// ==================== SYSTEM HANDLER ====================
// Handler utility functions untuk mode management dan operasi tingkat tinggi
// Fungsi-fungsi ini dipanggil dari main task untuk kontrol real-time

// ==================== HANDLER CONFIGURATION ====================

// ==================== TIME MANAGEMENT FUNCTIONS ====================

// Initialize NTP time synchronization
void initNTP()
{
    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 7 * 3600; // UTC+7 (WIB)
    const int daylightOffset_sec = 0;

    Serial.println("TIME  : Initializing NTP time synchronization...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    while (!time(nullptr))
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("TIME  : ✅ NTP Time synced");
    updateTime(); // Initial time update
}

// Main time update function (called from handleNtpUpdate)
void updateTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        parseCompileTime(); // fallback
        return;
    }

    // Update individual time components
    currentYear = timeinfo.tm_year + 1900;
    currentMonth = timeinfo.tm_mon + 1;
    currentDay = timeinfo.tm_mday;
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;

    // Update day name
    const char *dayNames[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    currentDayName = dayNames[timeinfo.tm_wday];

    // Update month name
    const char *monthNames[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    currentMonthName = monthNames[timeinfo.tm_mon];

    // Format full time string for LCD (HH:MM:SS)
    char timeBuf[16];
    sprintf(timeBuf, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    fullTimeStr = String(timeBuf);

    // Format full date string for LCD (YYYY-MM-DD)
    char dateBuf[16];
    sprintf(dateBuf, "%04d-%02d-%02d", currentYear, currentMonth, currentDay);
    fullDateStr = String(dateBuf);
}

// Fallback time parsing from compile time
void parseCompileTime()
{
    Serial.println("TIME  : ⚠️ Using compile time as fallback");

    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    char monthStr[4];
    int year, day, hour, min, sec;
    sscanf(__DATE__, "%s %d %d", monthStr, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    int month = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (strcmp(monthStr, months[i]) == 0)
        {
            month = i + 1;
            break;
        }
    }

    currentYear = year;
    currentMonth = month;
    currentDay = day;
    currentHour = hour;
    currentMinute = min;
    currentSecond = sec;

    currentDayName = "CompileTime";
    currentMonthName = String(monthStr);

    // Format strings
    char timeBuf[16];
    sprintf(timeBuf, "%02d:%02d:%02d", currentHour, currentMinute, currentSecond);
    fullTimeStr = String(timeBuf);

    char dateBuf[16];
    sprintf(dateBuf, "%04d-%02d-%02d", currentYear, currentMonth, currentDay);
    fullDateStr = String(dateBuf);
}

// Get formatted date-time string for Firebase (YYYY-MM-DD HH:MM:SS)
String getFirebaseDateTime()
{
    char dateTimeBuf[32];
    sprintf(dateTimeBuf, "%04d-%02d-%02d %02d:%02d:%02d",
            currentYear, currentMonth, currentDay,
            currentHour, currentMinute, currentSecond);
    return String(dateTimeBuf);
}

// Format time string for LCD display (HH:MM:SS)
String formatTimeString()
{
    if (fullTimeStr.length() > 0 && fullTimeStr != "00:00:00")
    {
        return fullTimeStr;
    }
    return "00:00:00";
}

// Format date string for LCD display (DD/MM/YYYY)
String formatDateString()
{
    if (fullDateStr.length() > 0 && fullDateStr != "1970-01-01")
    {
        // Convert YYYY-MM-DD to DD/MM/YYYY for LCD display
        if (fullDateStr.length() == 10)
        {
            String year = fullDateStr.substring(0, 4);
            String month = fullDateStr.substring(5, 7);
            String day = fullDateStr.substring(8, 10);
            return day + "/" + month + "/" + year;
        }
    }
    return "01/01/1970";
}

// ==================== DATA HANDLING FUNCTIONS ====================

// Handle sensor reading (every 1 second)
void handleSensorReading()
{
    // Read all sensors individually
    readTemperature();
    readPh();
    readTds();
    updateFlowRates();

    // Serial.printf("HANDLE: Sensors read - T:%.1f°C pH:%.2f TDS:%.1f\n", temperatureValue, phValue, tdsValue);
}

// Handle NTP time update (every 500ms)
void handleNtpUpdate()
{
    // Update Firebase time for accurate timestamping
    updateTime();
    // updateFirebaseTime();
}

// Handle data sending (every 1 minute)
void handleDataSend()
{
    if (!isFirebaseReady())
    {
        Serial.println("HANDLE: Firebase not ready, skipping data send");
        return;
    }

    // Upload sensor data to Firebase
    sendSensorDataToFirebase();

    Serial.println("HANDLE: 📊 Data send completed");
}

// Handle real-time control processing (call as frequently as possible)
void handleRealTimeProcessing()
{
    // Process real-time Firebase streams for instant control
    handleRealTimeControl();
}

// ==================== FUZZY LOGIC HANDLING ====================

// Handle fuzzy logic processing and relay activation (auto mode only)
void handleFuzzyExecution()
{
    if (!autoModeEnabled)
    {
        return;
    }

    // Read current sensor values and Firebase TDS setpoint
    float currentTds = tdsValue;
    float targetTds = getTdsSetpoint(); // Ambil dari Firebase setPPM_A

    if (currentTds < 0 || targetTds <= 0)
    {
        Serial.println("HANDLE: Invalid sensor data for fuzzy logic");
        return;
    }

    // Log TDS status only when values change significantly
    static float lastCurrentTds = 0.0;
    static float lastTargetTds = 0.0;
    static unsigned long lastTdsLogTime = 0;

    if ((currentTds - lastCurrentTds > 10.0 || lastCurrentTds - currentTds > 10.0) ||
        (targetTds - lastTargetTds > 10.0 || lastTargetTds - targetTds > 10.0) ||
        (millis() - lastTdsLogTime > 30000)) // Log every 30 seconds at minimum
    {
        Serial.printf("HANDLE: Auto mode - Current TDS: %.1f ppm, Target TDS: %.1f ppm (from Firebase)\n",
                      currentTds, targetTds);
        lastCurrentTds = currentTds;
        lastTargetTds = targetTds;
        lastTdsLogTime = millis();
    }

    // Check if fuzzy process is already active
    if (isFuzzyProcessActive())
    {
        // Continue existing fuzzy process iteration
        bool finished = executeFuzzyIteration(currentTds);

        if (!finished)
        {
            // Fuzzy iteration menghasilkan target mL - ambil hasilnya
            float targetML_A = getTargetML_A();
            float targetML_B = getTargetML_B();

            if (targetML_A > 0 || targetML_B > 0)
            {
                Serial.printf("HANDLE: Fuzzy processed TDS setpoint → Target A: %.1f mL, B: %.1f mL\n",
                              targetML_A, targetML_B);

                // Start pumps via Firebase relay control (unified approach)
                if (targetML_A > 0 && !relayState[0])
                {
                    updateRelayState(0, true); // Start Nutrisi A pump
                    Serial.printf("HANDLE: Auto Pump A started via Firebase - chasing %.1f mL (from fuzzy)\n", targetML_A);
                }

                if (targetML_B > 0 && !relayState[1])
                {
                    updateRelayState(1, true); // Start Nutrisi B pump
                    Serial.printf("HANDLE: Auto Pump B started via Firebase - chasing %.1f mL (from fuzzy)\n", targetML_B);
                }
            }
            else
            {
                Serial.println("HANDLE: Fuzzy logic determined no pumping needed");
            }
        }
        else
        {
            Serial.println("HANDLE: Fuzzy TDS control process completed");
        }
    }
    else
    {
        // Start new fuzzy process if TDS is not in target
        if (!isTDSTargetReached(currentTds, targetTds))
        {
            Serial.printf("HANDLE: TDS not in target - Starting fuzzy control (Firebase TDS: %.1f → fuzzy mL conversion)\n",
                          targetTds);
            Serial.printf("HANDLE: Starting fuzzy TDS control - Current: %.1f, Target: %.1f\n",
                          currentTds, targetTds);
            startFuzzyTDSControl(targetTds, currentTds); // Now with immediate execution
        }
        else
        {
            Serial.printf("HANDLE: TDS in target range (%.1f ≈ %.1f) - no fuzzy action needed\n",
                          currentTds, targetTds);
        }
    }

    // Log current status (only when status changes)
    static bool lastFuzzyActive = false;
    static int lastIteration = 0;
    bool currentFuzzyActive = isFuzzyProcessActive();
    int currentIteration = getFuzzyIterationCount();

    if (currentFuzzyActive != lastFuzzyActive || currentIteration != lastIteration)
    {
        // Get current fuzzy output and category for status display
        float fuzzyOutput = runFuzzy(currentTds, targetTds);
        String outputCategory = getOutputCategory(fuzzyOutput);

        Serial.printf("HANDLE: Fuzzy status - TDS: %.1f/%.1f, Active: %s, Iteration: %d, Output: %.1f%% (%s)\n",
                      currentTds, targetTds,
                      currentFuzzyActive ? "YES" : "NO",
                      currentIteration,
                      fuzzyOutput, outputCategory.c_str());
        lastFuzzyActive = currentFuzzyActive;
        lastIteration = currentIteration;
    }
}

// Update relay state in Firebase (works for both manual and auto mode)
void updateRelayState(int relayIndex, bool state)
{
    if (relayIndex < 0 || relayIndex >= NUM_RELAYS)
    {
        return;
    }

    // Choose the correct Firebase path based on current mode
    const char *targetPath;
    if (autoModeEnabled)
    {
        targetPath = autoRelayPaths[relayIndex];
    }
    else
    {
        targetPath = relayPaths[relayIndex]; // Manual relay paths
    }

    // Update Firebase relay path
    // The stream will detect this change and apply it instantly
    if (Firebase.RTDB.setBool(&fbdo, targetPath, state))
    {
        Serial.printf("HANDLE: %s relay %d command sent: %s (Stream will apply)\n",
                      autoModeEnabled ? "Auto" : "Manual", relayIndex + 1, state ? "ON" : "OFF");
    }
    else
    {
        Serial.printf("HANDLE: Failed to send %s relay %d command\n",
                      autoModeEnabled ? "auto" : "manual", relayIndex + 1);

        // Fallback: apply directly if Firebase update fails
        controlRelays(relayIndex, state);
        relayState[relayIndex] = state;
        Serial.printf("HANDLE: Fallback: Applied relay %d directly\n", relayIndex + 1);
    }
}

// Update auto relay state in Firebase (stream-optimized) - DEPRECATED, use updateRelayState instead
void updateAutoRelayState(int relayIndex, bool state)
{
    updateRelayState(relayIndex, state); // Redirect to new function
}

// ==================== EMERGENCY FUNCTIONS ====================

// Emergency stop all operations
void emergencyStop()
{
    Serial.println("HANDLE: 🚨 EMERGENCY STOP ACTIVATED");

    // Stop all relays
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        controlRelays(i, false);
        relayState[i] = false;
    }

    // Disable both modes
    autoModeEnabled = false;
    manualModeEnabled = false;

    Serial.println("HANDLE: ✅ Emergency stop completed");
}

// Resume normal operations after emergency stop
void resumeOperations()
{
    Serial.println("HANDLE: 🔄 Resuming normal operations...");

    // Re-enable the current mode from Firebase
    if (currentMode == "auto")
    {
        autoModeEnabled = true;
        manualModeEnabled = false;
        Serial.println("HANDLE: ✅ Auto mode re-enabled");

        // Trigger immediate fuzzy execution
        triggerFuzzyExecution = true;
        Serial.println("HANDLE: 🚀 Fuzzy execution trigger set for immediate run");

        // Reinitialize fuzzy system for auto mode
        initializeFuzzySystem();
    }
    else if (currentMode == "manual")
    {
        manualModeEnabled = true;
        autoModeEnabled = false;
        Serial.println("HANDLE: ✅ Manual mode re-enabled");
    }
    else
    {
        // Default to manual mode if currentMode is unknown
        manualModeEnabled = true;
        autoModeEnabled = false;
        Serial.println("HANDLE: ⚠️  Unknown mode, defaulting to manual");
    }

    // Reinitialize streams for the current mode to ensure proper control
    if (isFirebaseReady())
    {
        reinitializeStreamsForMode();
        Serial.println("HANDLE: 🔄 Firebase streams reinitialized");
    }
    else
    {
        Serial.println("HANDLE: ⚠️  Firebase not ready, streams will initialize when ready");
    }

    Serial.println("HANDLE: ✅ Operations fully resumed - System ready for normal operation");
}

// ==================== UTILITY FUNCTIONS ====================

// Check if auto mode is active
bool isAutoModeActive()
{
    return autoModeEnabled;
}

// Force mode refresh from Firebase (handled by real-time streams)
void refreshMode()
{
    Serial.println("HANDLE: 🔄 Mode managed by real-time streams - no manual refresh needed");
    Serial.printf("HANDLE: Current mode: %s\n", currentMode.c_str());
    Serial.printf("HANDLE: Auto mode active: %s\n", autoModeEnabled ? "Yes" : "No");
    Serial.printf("HANDLE: Manual mode active: %s\n", manualModeEnabled ? "Yes" : "No");
}

// ==================== STREAM STATUS FUNCTIONS ====================

// Get handler stream status
void getHandlerStreamStatus()
{
    Serial.println("HANDLE: 📊 Handler stream integration status:");
    Serial.printf("HANDLE:   Auto Mode: %s\n", autoModeEnabled ? "🟢 ACTIVE" : "🔴 INACTIVE");
    Serial.printf("HANDLE:   Manual Mode: %s\n", manualModeEnabled ? "🟢 ACTIVE" : "🔴 INACTIVE");
    Serial.printf("HANDLE:   Current Mode: %s\n", currentMode.c_str());

    if (isRealTimeActive())
    {
        Serial.println("HANDLE:   Stream Control: 🟢 REAL-TIME ACTIVE");
        Serial.println("HANDLE:   ⚡ Mode changes: Instant via stream");
        Serial.println("HANDLE:   ⚡ Relay control: Zero-delay via stream");
        Serial.println("HANDLE:   ⚡ TDS setpoint: Real-time via stream");
    }
    else
    {
        Serial.println("HANDLE:   Stream Control: 🔴 FALLBACK MODE");
        Serial.println("HANDLE:   ⚠️  Using periodic polling");
    }
}

// ==================== MEMORY MONITORING FUNCTIONS ====================

// Monitor ESP32 memory usage
void checkMemoryStatus()
{
    // Get heap memory info
    size_t freeHeap = ESP.getFreeHeap();
    size_t totalHeap = ESP.getHeapSize();
    size_t usedHeap = totalHeap - freeHeap;

    // Get PSRAM info (if available)
    size_t freePsram = ESP.getFreePsram();
    size_t totalPsram = ESP.getPsramSize();

    // Calculate percentages
    float heapUsage = (float)usedHeap / totalHeap * 100.0;
    float psramUsage = totalPsram > 0 ? (float)(totalPsram - freePsram) / totalPsram * 100.0 : 0;

    Serial.println("HANDLE: 💾 Memory Status:");
    Serial.printf("HANDLE:   Heap: %d/%d bytes (%.1f%% used)\n", usedHeap, totalHeap, heapUsage);
    Serial.printf("HANDLE:   Free Heap: %d bytes\n", freeHeap);

    if (totalPsram > 0)
    {
        Serial.printf("HANDLE:   PSRAM: %d/%d bytes (%.1f%% used)\n",
                      totalPsram - freePsram, totalPsram, psramUsage);
    }
    else
    {
        Serial.println("HANDLE:   PSRAM: Not available");
    }

    // Memory warnings
    if (freeHeap < 20000)
    {
        Serial.println("HANDLE:   ⚠️  WARNING: Low heap memory!");
    }
    if (heapUsage > 85.0)
    {
        Serial.println("HANDLE:   🚨 CRITICAL: High memory usage!");
    }

    // Stack info
    Serial.printf("HANDLE:   Stack High Water Mark: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
}

// ==================== UNIFIED PUMP EXECUTION FUNCTIONS ====================

// Handle unified pump execution - both manual and auto mode using same chasing logic
void handleUnifiedPumpExecution()
{
    // Static variables to track targets and states (works for both manual and auto)
    static bool pumpA_chasing = false;
    static bool pumpB_chasing = false;
    static bool pumpPH_UP_chasing = false;
    static bool pumpPH_DOWN_chasing = false;

    static float targetML_A = 0.0;
    static float targetML_B = 0.0;
    static float targetML_PH_UP = 0.0;
    static float targetML_PH_DOWN = 0.0;

    // Get setpoint based on current mode
    float setpointML = 0.0;
    bool isAutoMode = autoModeEnabled;

    if (isAutoMode)
    {
        // Auto mode: get targets from fuzzy logic (yang sudah diproses dari Firebase TDS setpoint)
        float fuzzyTargetA = getTargetML_A();
        float fuzzyTargetB = getTargetML_B();

        // Check for new fuzzy targets (hasil konversi TDS setpoint → mL)
        static float lastFuzzyTargetA = 0.0;
        static float lastFuzzyTargetB = 0.0;

        if (fuzzyTargetA > 0 && !pumpA_chasing)
        {
            targetML_A = fuzzyTargetA;
            // Only log when target changes
            if (fuzzyTargetA != lastFuzzyTargetA)
            {
                Serial.printf("HANDLE: Auto mode - New fuzzy target A: %.1f mL (converted from TDS setpoint)\n", targetML_A);
                lastFuzzyTargetA = fuzzyTargetA;
            }
        }
        if (fuzzyTargetB > 0 && !pumpB_chasing)
        {
            targetML_B = fuzzyTargetB;
            // Only log when target changes
            if (fuzzyTargetB != lastFuzzyTargetB)
            {
                Serial.printf("HANDLE: Auto mode - New fuzzy target B: %.1f mL (converted from TDS setpoint)\n", targetML_B);
                lastFuzzyTargetB = fuzzyTargetB;
            }
        }
    }
    else
    {
        // Manual mode: get manual setpoint directly from Firebase
        setpointML = getManualSetpoint();
        // Only log when setpoint changes or is first detected
        static float lastSetpointML = 0.0;
        if (setpointML > 0 && setpointML != lastSetpointML)
        {
            Serial.printf("HANDLE: Manual mode - Using direct mL setpoint: %.1f mL (from Firebase)\n", setpointML);
            lastSetpointML = setpointML;
        }
    }

    // Check if any pump relay is active and should start chasing
    if (relayState[0] && !pumpA_chasing) // Nutrisi A pump
    {
        if (isAutoMode && targetML_A > 0)
        {
            resetFlowSensors(); // Reset before starting
            pumpA_chasing = true;
            Serial.printf("HANDLE: Auto Pump A started - chasing %.1f mL\n", targetML_A);
        }
        else if (!isAutoMode && setpointML > 0)
        {
            resetFlowSensors(); // Reset before starting
            targetML_A = setpointML;
            pumpA_chasing = true;
            Serial.printf("HANDLE: Manual Pump A started - chasing %.1f mL\n", targetML_A);
        }
    }

    if (relayState[1] && !pumpB_chasing) // Nutrisi B pump
    {
        if (isAutoMode && targetML_B > 0)
        {
            resetFlowSensors(); // Reset before starting
            pumpB_chasing = true;
            Serial.printf("HANDLE: Auto Pump B started - chasing %.1f mL\n", targetML_B);
        }
        else if (!isAutoMode && setpointML > 0)
        {
            resetFlowSensors(); // Reset before starting
            targetML_B = setpointML;
            pumpB_chasing = true;
            Serial.printf("HANDLE: Manual Pump B started - chasing %.1f mL\n", targetML_B);
        }
    }

    if (relayState[2] && !pumpPH_UP_chasing) // pH UP pump
    {
        if (!isAutoMode && setpointML > 0)
        {
            resetFlowSensors(); // Reset before starting
            targetML_PH_UP = setpointML;
            pumpPH_UP_chasing = true;
            Serial.printf("HANDLE: Manual pH UP pump started - chasing %.1f mL\n", targetML_PH_UP);
        }
    }

    if (relayState[3] && !pumpPH_DOWN_chasing) // pH DOWN pump
    {
        if (!isAutoMode && setpointML > 0)
        {
            resetFlowSensors(); // Reset before starting
            targetML_PH_DOWN = setpointML;
            pumpPH_DOWN_chasing = true;
            Serial.printf("HANDLE: Manual pH DOWN pump started - chasing %.1f mL\n", targetML_PH_DOWN);
        }
    }

    // Monitor and stop pumps when targets are reached
    if (pumpA_chasing)
    {
        float currentML_A = getFlowSensorA_mL();
        if (currentML_A >= targetML_A)
        {
            setRelayPumpA(false);
            relayState[0] = false;      // Update local state immediately
            updateRelayState(0, false); // Update Firebase
            pumpA_chasing = false;

            if (isAutoMode)
            {
                Serial.printf("HANDLE: Auto Pump A stopped - achieved %.1f mL (target %.1f mL)\n", currentML_A, targetML_A);
                // Update actual mL for fuzzy iteration check
                updateActualML(currentML_A, getFlowSensorB_mL());
            }
            else
            {
                Serial.printf("HANDLE: Manual Pump A stopped - achieved %.1f mL (target %.1f mL)\n", currentML_A, targetML_A);
            }
        }
    }

    if (pumpB_chasing)
    {
        float currentML_B = getFlowSensorB_mL();
        if (currentML_B >= targetML_B)
        {
            setRelayPumpB(false);
            relayState[1] = false;      // Update local state immediately
            updateRelayState(1, false); // Update Firebase
            pumpB_chasing = false;

            if (isAutoMode)
            {
                Serial.printf("HANDLE: Auto Pump B stopped - achieved %.1f mL (target %.1f mL)\n", currentML_B, targetML_B);
                // Update actual mL for fuzzy iteration check
                updateActualML(getFlowSensorA_mL(), currentML_B);
            }
            else
            {
                Serial.printf("HANDLE: Manual Pump B stopped - achieved %.1f mL (target %.1f mL)\n", currentML_B, targetML_B);
            }
        }
    }

    if (pumpPH_UP_chasing)
    {
        float currentML_PH_UP = getFlowSensorPH_UP_mL();
        if (currentML_PH_UP >= targetML_PH_UP)
        {
            setRelayPumpPH_UP(false);
            relayState[2] = false;      // Update local state immediately
            updateRelayState(2, false); // Update Firebase
            pumpPH_UP_chasing = false;
            Serial.printf("HANDLE: Manual pH UP pump stopped - achieved %.1f mL (target %.1f mL)\n", currentML_PH_UP, targetML_PH_UP);
        }
    }

    if (pumpPH_DOWN_chasing)
    {
        float currentML_PH_DOWN = getFlowSensorPH_DOWN_mL();
        if (currentML_PH_DOWN >= targetML_PH_DOWN)
        {
            setRelayPumpPH_DOWN(false);
            relayState[3] = false;      // Update local state immediately
            updateRelayState(3, false); // Update Firebase
            pumpPH_DOWN_chasing = false;
            Serial.printf("HANDLE: Manual pH DOWN pump stopped - achieved %.1f mL (target %.1f mL)\n", currentML_PH_DOWN, targetML_PH_DOWN);
        }
    }

    // Reset chasing flags if relay is manually turned off from Firebase
    if (!relayState[0])
        pumpA_chasing = false;
    if (!relayState[1])
        pumpB_chasing = false;
    if (!relayState[2])
        pumpPH_UP_chasing = false;
    if (!relayState[3])
        pumpPH_DOWN_chasing = false;
}

// Execute specific pump with mL target (legacy function for direct pump control)
bool executePumpWithTarget(int pumpIndex, float targetML)
{
    if (targetML <= 0)
    {
        Serial.printf("HANDLE: Invalid mL target for pump %d: %.1f\n", pumpIndex + 1, targetML);
        return false;
    }

    // Reset flow sensors before starting
    resetFlowSensors();

    unsigned long pumpStartTime = millis();
    const unsigned long maxPumpTime = 60000; // 60 second timeout for manual operation

    // Start the appropriate pump
    switch (pumpIndex)
    {
    case 0: // Nutrisi A
        setRelayPumpA(true);
        Serial.printf("HANDLE: Manual Pump A started - target %.1f mL\n", targetML);
        break;
    case 1: // Nutrisi B
        setRelayPumpB(true);
        Serial.printf("HANDLE: Manual Pump B started - target %.1f mL\n", targetML);
        break;
    case 2: // pH UP
        setRelayPumpPH_UP(true);
        Serial.printf("HANDLE: Manual pH UP pump started - target %.1f mL\n", targetML);
        break;
    case 3: // pH DOWN
        setRelayPumpPH_DOWN(true);
        Serial.printf("HANDLE: Manual pH DOWN pump started - target %.1f mL\n", targetML);
        break;
    default:
        Serial.printf("HANDLE: Invalid pump index: %d\n", pumpIndex);
        return false;
    }

    // Monitor until target reached or timeout
    while (millis() - pumpStartTime < maxPumpTime)
    {
        float currentML = 0.0;

        // Get current mL reading for the specific pump
        switch (pumpIndex)
        {
        case 0:
            currentML = getFlowSensorA_mL();
            break;
        case 1:
            currentML = getFlowSensorB_mL();
            break;
        case 2:
            currentML = getFlowSensorPH_UP_mL();
            break;
        case 3:
            currentML = getFlowSensorPH_DOWN_mL();
            break;
        }

        // Check if target reached
        if (currentML >= targetML)
        {
            // Stop the pump
            switch (pumpIndex)
            {
            case 0:
                setRelayPumpA(false);
                break;
            case 1:
                setRelayPumpB(false);
                break;
            case 2:
                setRelayPumpPH_UP(false);
                break;
            case 3:
                setRelayPumpPH_DOWN(false);
                break;
            }

            // Update Firebase relay state
            updateAutoRelayState(pumpIndex, false);

            Serial.printf("HANDLE: Manual pump %d stopped - achieved %.1f mL (target %.1f mL)\n",
                          pumpIndex + 1, currentML, targetML);
            return true;
        }

        delay(100); // Check every 100ms
    }

    // Emergency stop if timeout
    switch (pumpIndex)
    {
    case 0:
        setRelayPumpA(false);
        break;
    case 1:
        setRelayPumpB(false);
        break;
    case 2:
        setRelayPumpPH_UP(false);
        break;
    case 3:
        setRelayPumpPH_DOWN(false);
        break;
    }

    updateAutoRelayState(pumpIndex, false);
    Serial.printf("HANDLE: ⚠️ Manual pump %d timeout - emergency stop\n", pumpIndex + 1);
    return false;
}
