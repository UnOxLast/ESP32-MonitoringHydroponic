// ==================== FIREBASE CONFIGURATION ====================

// Firebase configuration
#define API_KEY "-"
#define DATABASE_URL "-"

// Firebase objects
FirebaseData fbdo; // For data upload only
FirebaseAuth auth;
FirebaseJson json;
FirebaseConfig config;

// Additional Firebase objects for real-time streaming
FirebaseData fbdoModeStream; // For real-time mode streaming
FirebaseData fbdoPPMStream;  // For real-time TDS setpoint streaming

// Firebase Variables
int firebaseRecordCount = 0;
// currentMode, firebaseReady, streamSetup moved to Global.h

// Firebase paths
String modePath = "/devices/1234567890/configs/mode";
String ppmAPath = "/devices/1234567890/configs/setPPM_A/value";
String ppmMPath = "/devices/1234567890/configs/setPPM_M/value"; // Single manual setpoint

// Relay paths for Firebase control
const char *relayPaths[5] = {
    "/devices/1234567890/configs/relays/manual/nutrient_a",
    "/devices/1234567890/configs/relays/manual/nutrient_b",
    "/devices/1234567890/configs/relays/manual/ph_buffer",
    "/devices/1234567890/configs/relays/manual/aerator",
    "/devices/1234567890/configs/relays/manual/water"};

const char *autoRelayPaths[5] = {
    "/devices/1234567890/configs/relays/auto/nutrient_a",
    "/devices/1234567890/configs/relays/auto/nutrient_b",
    "/devices/1234567890/configs/relays/auto/ph_buffer",
    "/devices/1234567890/configs/relays/auto/aerator",
    "/devices/1234567890/configs/relays/auto/water"};

// Relay state array for Firebase sync
bool relayState[5] = {false, false, false, false, false};
// Manual setpoint variable (for real-time control)
float setpointManual = 100.0; // Default 100ml

const int NUM_RELAYS = 5;

// Firebase real-time streaming objects (declared after NUM_RELAYS)
FirebaseData fbdoRelayStream[NUM_RELAYS]; // For real-time relay streaming
FirebaseData fbdoManualSetpointStream;    // Single manual setpoint stream

// ==================== FIREBASE CALLBACK FUNCTIONS ====================

// Note: tokenStatusCallback is provided by Firebase library

// ==================== FIREBASE SETUP ====================

// Initialize Firebase connection
void initFirebase()
{
    Serial.println("FBASE : Initializing Firebase...");

    // Configure Firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Anonymous sign up
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("FBASE : ✅ Firebase Connected");
        firebaseReady = true;
    }
    else
    {
        Serial.println("FBASE : ❌ Firebase Connection Failed");
        firebaseReady = false;
        return;
    }

    // Set token status callback
    config.token_status_callback = tokenStatusCallback;

    // Begin Firebase with config
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Get initial record count
    if (Firebase.RTDB.getInt(&fbdo, "/devices/1234567890/record_count"))
    {
        firebaseRecordCount = fbdo.intData();
        Serial.printf("FBASE : Record Count: %d\n", firebaseRecordCount);
    }
    else
    {
        Serial.println("FBASE : Failed to get record count, using default 0");
        firebaseRecordCount = 0;
    }

    // Get mode from Firebase (don't overwrite Firebase with local default)
    if (Firebase.RTDB.getString(&fbdo, modePath))
    {
        String firebaseMode = fbdo.stringData();
        if (firebaseMode == "auto" || firebaseMode == "manual")
        {
            currentMode = firebaseMode;
            Serial.printf("FBASE : Mode loaded from Firebase: %s\n", currentMode.c_str());
            // Apply the mode immediately
            changeMode(currentMode);
        }
        else
        {
            Serial.printf("FBASE : Invalid mode in Firebase (%s), using default: %s\n",
                          firebaseMode.c_str(), currentMode.c_str());
            Firebase.RTDB.setString(&fbdo, modePath, currentMode);
        }
    }
    else
    {
        Serial.printf("FBASE : Mode not found in Firebase, setting default: %s\n", currentMode.c_str());
        Firebase.RTDB.setString(&fbdo, modePath, currentMode);
    }

    // Initialize setpoint values and sync from Firebase
    Serial.printf("FBASE : Initial setpoint values - Auto: %d ppm, Manual: %.1f mL\n",
                  getTdsSetpoint(), setpointManual);

    // Try to get manual setpoint from Firebase
    if (Firebase.RTDB.getFloat(&fbdo, ppmMPath))
    {
        setpointManual = fbdo.floatData();
        Serial.printf("FBASE : Manual setpoint loaded from Firebase: %.1f mL\n", setpointManual);
    }
    else
    {
        Serial.printf("FBASE : Failed to load manual setpoint, using default: %.1f mL\n", setpointManual);
        Firebase.RTDB.setFloat(&fbdo, ppmMPath, setpointManual);
    }

    // Try to get auto setpoint from Firebase
    if (Firebase.RTDB.getInt(&fbdo, ppmAPath))
    {
        int autoSetpoint = fbdo.intData();
        setTdsSetpoint(autoSetpoint);
        Serial.printf("FBASE : Auto setpoint loaded from Firebase: %d ppm\n", autoSetpoint);
    }
    else
    {
        Serial.printf("FBASE : Failed to load auto setpoint, using default: %d ppm\n", getTdsSetpoint());
        Firebase.RTDB.setInt(&fbdo, ppmAPath, getTdsSetpoint());
    }

    Serial.println("FBASE : Firebase system ready");

    // Setup real-time streams for instant control
    setupFirebaseStreams();
}

// ==================== REAL-TIME FIREBASE STREAMS ====================

// End all active streams for cleanup
void endAllStreams()
{
    Serial.println("FBASE : 🔄 Ending all active streams...");

    // End mode stream
    Firebase.RTDB.endStream(&fbdoModeStream);

    // End TDS setpoint stream
    Firebase.RTDB.endStream(&fbdoPPMStream);

    // End all manual setpoint streams
    Firebase.RTDB.endStream(&fbdoManualSetpointStream);

    // End all relay streams
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        Firebase.RTDB.endStream(&fbdoRelayStream[i]);
    }

    Serial.println("FBASE : ✅ All streams ended");
}

// Setup Firebase streams based on current mode
void setupFirebaseStreams()
{
    if (!firebaseReady || !Firebase.ready())
    {
        Serial.println("FBASE : Cannot setup streams - Firebase not ready");
        return;
    }

    Serial.printf("FBASE : 🎯 Setting up mode-based streams for %s mode...\n", currentMode.c_str());

    // Always setup mode stream for mode change detection
    setupModeStream();

    // Setup streams based on current mode
    if (currentMode == "manual")
    {
        setupManualModeStreams();
    }
    else if (currentMode == "auto")
    {
        setupAutoModeStreams();
    }

    streamSetup = true;
    Serial.printf("FBASE : 🚀 %s mode streams active - Optimized for current operation!\n", currentMode.c_str());
}

// Setup mode stream (always needed for mode change detection)
void setupModeStream()
{
    if (Firebase.RTDB.beginStream(&fbdoModeStream, modePath))
    {
        Serial.println("FBASE : ✅ Mode stream initialized");
    }
    else
    {
        Serial.printf("FBASE : ❌ Failed to setup mode stream: %s\n", fbdoModeStream.errorReason().c_str());
    }
}

// Setup streams for manual mode
void setupManualModeStreams()
{
    Serial.println("FBASE : 🎯 Setting up manual mode streams...");

    // Setup manual relay streams
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        if (Firebase.RTDB.beginStream(&fbdoRelayStream[i], relayPaths[i]))
        {
            Serial.printf("FBASE : ✅ Manual relay %d stream initialized\n", i + 1);
        }
        else
        {
            Serial.printf("FBASE : ❌ Failed to setup manual relay %d stream: %s\n", i + 1, fbdoRelayStream[i].errorReason().c_str());
        }
    }

    // Setup single manual setpoint stream
    if (Firebase.RTDB.beginStream(&fbdoManualSetpointStream, ppmMPath))
    {
        Serial.println("FBASE : ✅ Manual setpoint stream initialized");
    }
    else
    {
        Serial.printf("FBASE : ❌ Failed to setup manual setpoint stream: %s\n", fbdoManualSetpointStream.errorReason().c_str());
    }

    Serial.printf("FBASE : 🎯 Manual mode: %d streams active (1 mode + 5 relays + 1 setpoint)\n", 1 + NUM_RELAYS + 1);
}

// Setup streams for auto mode
void setupAutoModeStreams()
{
    Serial.println("FBASE : 🎯 Setting up auto mode streams...");

    // Setup TDS setpoint stream for auto mode
    if (Firebase.RTDB.beginStream(&fbdoPPMStream, ppmAPath))
    {
        Serial.println("FBASE : ✅ TDS setpoint stream initialized");
    }
    else
    {
        Serial.printf("FBASE : ❌ Failed to setup TDS stream: %s\n", fbdoPPMStream.errorReason().c_str());
    }

    // Setup auto relay streams
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        if (Firebase.RTDB.beginStream(&fbdoRelayStream[i], autoRelayPaths[i]))
        {
            Serial.printf("FBASE : ✅ Auto relay %d stream initialized\n", i + 1);
        }
        else
        {
            Serial.printf("FBASE : ❌ Failed to setup auto relay %d stream: %s\n", i + 1, fbdoRelayStream[i].errorReason().c_str());
        }
    }

    Serial.printf("FBASE : 🎯 Auto mode: %d streams active (1 mode + 1 TDS + 5 relays)\n", 1 + 1 + NUM_RELAYS);
}

// Handle real-time Firebase streams (call this frequently for instant response)
void handleFirebaseStreams()
{
    if (!firebaseReady || !Firebase.ready() || !streamSetup)
    {
        return;
    }

    // Handle mode stream changes
    if (Firebase.RTDB.readStream(&fbdoModeStream))
    {
        if (fbdoModeStream.streamAvailable())
        {
            if (fbdoModeStream.dataType() == "string")
            {
                String newMode = fbdoModeStream.stringData();
                if (newMode != currentMode)
                {
                    Serial.printf("FBASE : 🔥 INSTANT Mode change detected: %s -> %s\n", currentMode.c_str(), newMode.c_str());
                    currentMode = newMode;
                    changeMode(currentMode);

                    // Reinitialize all streams with new mode configuration
                    reinitializeStreamsForMode();
                }
            }
        }
    }

    // Handle TDS setpoint stream changes (only in auto mode)
    if (currentMode == "auto" && Firebase.RTDB.readStream(&fbdoPPMStream))
    {
        if (fbdoPPMStream.streamAvailable())
        {
            if (fbdoPPMStream.dataType() == "int")
            {
                int newPPM = fbdoPPMStream.intData();
                if (getTdsSetpoint() != newPPM)
                {
                    Serial.printf("FBASE : 🎯 INSTANT TDS setpoint change: %d -> %d ppm\n", getTdsSetpoint(), newPPM);
                    setTdsSetpoint(newPPM);
                    Serial.printf("FBASE : 🎯 Auto setpoint applied: %d -> %d ppm\n", getTdsSetpoint(), newPPM);
                }
            }
        }
    }

    // Handle relay stream changes
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        if (Firebase.RTDB.readStream(&fbdoRelayStream[i]))
        {
            if (fbdoRelayStream[i].streamAvailable())
            {
                if (fbdoRelayStream[i].dataType() == "boolean")
                {
                    bool newState = fbdoRelayStream[i].boolData();
                    if (relayState[i] != newState)
                    {
                        Serial.printf("FBASE : ⚡ INSTANT Relay %d change: %s -> %s\n",
                                      i + 1,
                                      relayState[i] ? "ON" : "OFF",
                                      newState ? "ON" : "OFF");
                        controlRelays(i, newState);
                    }
                }
            }
        }
    }

    // Handle single manual setpoint stream changes (only in manual mode)
    if (currentMode == "manual")
    {
        if (Firebase.RTDB.readStream(&fbdoManualSetpointStream))
        {
            if (fbdoManualSetpointStream.streamAvailable())
            {
                if (fbdoManualSetpointStream.dataType() == "float" || fbdoManualSetpointStream.dataType() == "int")
                {
                    float newSetpoint = fbdoManualSetpointStream.floatData();
                    if (setpointManual != newSetpoint)
                    {
                        Serial.printf("FBASE : 🎯 INSTANT Manual setpoint change: %.1f -> %.1f\n",
                                      setpointManual, newSetpoint);
                        setpointManual = newSetpoint;
                        Serial.printf("FBASE : 🎯 Manual setpoint applied: %.1f -> %.1f ppm\n", setpointManual, newSetpoint);
                    }
                }
            }
        }
    }
}

// Reinitialize all streams when mode changes (mode-based approach)
void reinitializeStreamsForMode()
{
    Serial.printf("FBASE : 🔄 Reinitializing streams for %s mode...\n", currentMode.c_str());

    // End all current streams (except mode stream which is always active)
    Firebase.RTDB.endStream(&fbdoPPMStream);
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        Firebase.RTDB.endStream(&fbdoRelayStream[i]);
    }
    Firebase.RTDB.endStream(&fbdoManualSetpointStream);

    // Setup streams based on new mode
    if (currentMode == "manual")
    {
        setupManualModeStreams();
    }
    else if (currentMode == "auto")
    {
        setupAutoModeStreams();
    }

    Serial.printf("FBASE : ✅ Streams reinitialized for %s mode\n", currentMode.c_str());
}

// ==================== DATA UPLOAD FUNCTIONS ====================

// Update record device data to Firebase
void updateRecordDeviceData()
{
    if (!firebaseReady || !Firebase.ready())
    {
        Serial.println("FBASE : Firebase not ready, skipping data upload");
        return;
    }

    String path = "/devices/1234567890/records/";
    if (firebaseRecordCount >= 0)
    {
        int nextRecordIndex = firebaseRecordCount;
        json.clear();

        // Validasi sebelum menambahkan ke JSON
        String currentDateTime = getFirebaseDateTime();
        if (currentDateTime.length() > 0)
        {
            json.set("datetime", currentDateTime);
        }
        if (phValue >= 0)
        {
            json.set("ph", phValue);
        }
        if (tdsValue >= 0)
        {
            json.set("tank_tds", tdsValue);
        }
        if (temperatureValue >= 0)
        {
            json.set("water_temp", temperatureValue);
        }

        String recordPath = path + String(nextRecordIndex);

        if (Firebase.RTDB.setJSON(&fbdo, recordPath.c_str(), &json))
        {
            Serial.println("FBASE : ✅ Data uploaded successfully");
            Serial.printf("FBASE : %s\n", json.raw());
            Firebase.RTDB.setInt(&fbdo, "/devices/1234567890/record_count", nextRecordIndex + 1);
            firebaseRecordCount += 1;
        }
        else
        {
            Serial.println("FBASE : ❌ Failed to upload data:");
            Serial.printf("FBASE : %s\n", fbdo.errorReason().c_str());
        }
    }
    else
    {
        Serial.println("FBASE : Starting from record index 0");
        Firebase.RTDB.setInt(&fbdo, "/devices/1234567890/record_count", 1);
        firebaseRecordCount = 1;
    }
}

// Send sensor data to Firebase
void sendSensorDataToFirebase()
{
    if (!firebaseReady || !Firebase.ready())
    {
        Serial.println("FBASE : Firebase not ready, skipping data upload");
        return;
    }

    // Upload record data
    updateRecordDeviceData();
}

// ==================== RELAY CONTROL FUNCTIONS ====================

// Control relay based on Firebase command
void controlRelays(int relayIndex, bool state)
{
    if (relayIndex < 0 || relayIndex >= NUM_RELAYS)
    {
        Serial.printf("FBASE : Invalid relay index: %d\n", relayIndex);
        return;
    }

    // Update local state
    relayState[relayIndex] = state;

    // Control physical relay (mapping Firebase relay index to our relay functions)
    switch (relayIndex)
    {
    case 0: // Relay 1 - Pump A
        setRelayPumpA(state);
        break;
    case 1: // Relay 2 - Pump B
        setRelayPumpB(state);
        break;
    case 2: // Relay 3 - pH UP
        setRelayPumpPH_UP(state);
        break;
    case 3: // Relay 4 - pH DOWN
        setRelayPumpPH_DOWN(state);
        break;
    case 4: // Relay 5 - Mixer
        setRelayMixerPump(state);
        break;
    }

    Serial.printf("FBASE : Relay %d set to %s\n", relayIndex + 1, state ? "ON" : "OFF");
}

// Change system mode
void changeMode(String mode)
{
    Serial.printf("FBASE : Changing mode to: %s\n", mode.c_str());

    if (mode == "auto")
    {
        Serial.println("FBASE : 🤖 Automatic mode activated");
        // Automatic mode - fuzzy logic will control relays

        // Update local flags
        autoModeEnabled = true;
        manualModeEnabled = false;

        // Trigger immediate fuzzy execution
        triggerFuzzyExecution = true;
        Serial.println("FBASE : 🚀 Fuzzy execution trigger set for immediate run");

        // Initialize fuzzy system if needed
        initializeFuzzySystem();
    }
    else if (mode == "manual")
    {
        Serial.println("FBASE : 👨‍💻 Manual mode activated");
        // Manual mode - Firebase controls relays directly

        // Update local flags
        manualModeEnabled = true;
        autoModeEnabled = false;
    }

    currentMode = mode;
}

// ==================== MANUAL SETPOINT FUNCTIONS ====================

// Get current manual setpoint value
float getManualSetpoint()
{
    return setpointManual;
}

// ==================== FUZZY LOGGING ====================

// Send fuzzy log to Firebase
void sendFuzzyLogToFirebase()
{
    if (!firebaseReady || !Firebase.ready())
    {
        Serial.println("FBASE : Firebase not ready, skipping fuzzy log upload");
        return;
    }

    // Prepare JSON for fuzzy log
    FirebaseJson fuzzyLog;
    fuzzyLog.clear();

    String currentDateTime = getFirebaseDateTime();
    fuzzyLog.set("datetime", currentDateTime);
    fuzzyLog.set("current_tds", tdsValue);
    fuzzyLog.set("tds_setpoint", getTdsSetpoint());
    fuzzyLog.set("fuzzy_output", runFuzzy(tdsValue, getTdsSetpoint()));

    String path = "/devices/1234567890/logs/" + String(firebaseRecordCount);

    if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &fuzzyLog))
    {
        Serial.println("FBASE : ✅ Fuzzy log sent to Firebase");
        Serial.printf("FBASE : %s\n", fuzzyLog.raw());
    }
    else
    {
        Serial.println("FBASE : ❌ Failed to send fuzzy log");
        Serial.printf("FBASE : %s\n", fbdo.errorReason().c_str());
    }
}

// ==================== STATUS AND UTILITY FUNCTIONS ====================

// Check if Firebase is ready
bool isFirebaseReady()
{
    return firebaseReady && Firebase.ready();
}

// Get current Firebase status
void getFirebaseStatus()
{
    Serial.println("FBASE : 📊 Firebase system status:");
    Serial.printf("FBASE :   Connection: %s\n", firebaseReady ? "Ready" : "Not Ready");
    Serial.printf("FBASE :   Firebase Ready: %s\n", Firebase.ready() ? "Yes" : "No");
    Serial.printf("FBASE :   Current Mode: %s\n", currentMode.c_str());
    Serial.printf("FBASE :   Record Count: %d\n", firebaseRecordCount);
    String currentDateTime = getFirebaseDateTime();
    Serial.printf("FBASE :   Last DateTime: %s\n", currentDateTime.c_str());

    Serial.println("FBASE :   Relay States:");
    for (int i = 0; i < NUM_RELAYS; i++)
    {
        Serial.printf("FBASE :     Relay %d: %s\n", i + 1, relayState[i] ? "ON" : "OFF");
    }
}

// ==================== REAL-TIME CONTROL FUNCTIONS ====================

// Fast real-time control check (call this as frequently as possible)
void handleRealTimeControl()
{
    // Handle Firebase streams for instant response
    handleFirebaseStreams();

    // Optional: Add any other real-time processing here
}

// Check if real-time streams are active
bool isRealTimeActive()
{
    return firebaseReady && Firebase.ready() && streamSetup;
}

// Get real-time status
void getRealTimeStatus()
{
    Serial.println("FBASE : 🚀 Real-time control status:");
    Serial.printf("FBASE :   Firebase Ready: %s\n", firebaseReady ? "Yes" : "No");
    Serial.printf("FBASE :   Streams Setup: %s\n", streamSetup ? "Yes" : "No");
    Serial.printf("FBASE :   Real-time Active: %s\n", isRealTimeActive() ? "🟢 ACTIVE" : "🔴 INACTIVE");
    Serial.printf("FBASE :   Current Mode: %s\n", currentMode.c_str());

    if (isRealTimeActive())
    {
        Serial.println("FBASE :   ⚡ Zero-delay control enabled!");
        Serial.println("FBASE :   📱 Flutter app commands will be instant");
    }
    else
    {
        Serial.println("FBASE :   ⚠️  Fallback to polling mode");
    }
}
