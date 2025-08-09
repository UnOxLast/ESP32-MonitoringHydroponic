/*
  LCD.ino - ESP32 Hydroponics LCD Display Module

  FEATURES:
  =========
  ✅ Selective Line Updates - Only updates changed lines to reduce I2C traffic
  ✅ Robust Error Handling - Automatic detection and recovery from LCD disconnection
  ✅ Health Monitoring - Periodic I2C health checks with fallback to Serial output
  ✅ Multiple Display Modes - Normal sensor view and relay status view
  ✅ Status Header - Real-time Firebase/WiFi/Mode status display
  ✅ Auto Recovery - Attempts to reconnect and restore LCD functionality
  ✅ Diagnostic Tools - Built-in diagnostics and emergency reset functions

  DISPLAY LAYOUT:
  ===============
  Line 0: =MELON PENS=[F]=[M]=  (Status header)
  Line 1: Content based on mode (sensors/relay)
  Line 2: Content based on mode (sensors/relay)
  Line 3: Content based on mode (sensors/relay)

  ERROR HANDLING:
  ===============
  - I2C device availability check before each operation
  - Automatic recovery attempts with exponential backoff
  - Fallback to Serial output when LCD is unavailable
  - Periodic health checks to detect disconnections
  - Emergency reset capability for manual intervention

  INTEGRATION:
  ============
  Call updateLCD() from main loop for automatic operation.
  Use setLcdMode() to switch between display modes.
  Monitor isLcdAvailable() for system status reporting.

  Dependencies: LiquidCrystal_I2C, Wire, Global.h variables
*/

// ==================== LCD DISPLAY MODULE FOR ESP32 HYDROPONICS ====================

// LCD display management for ESP32 Melon Hydroponics System
// New implementation with selective update and optimized layout
// Features:
// - Selective refresh (only update changed content)
// - New MELON PENS layout with status indicators
// - Firebase/WiFi/Mode status indicators
// - Optimized for 20x4 LCD display

// LCD Configuration
#define LCD_ADDRESS 0x27 // Default I2C address for LCD
#define LCD_COLUMNS 20   // Number of columns
#define LCD_ROWS 4       // Number of rows

// ==================== LCD VARIABLES ====================

// LCD display object
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// LCD status and timing
unsigned long lastLcdUpdate = 0;
const unsigned long LCD_UPDATE_INTERVAL = 1000; // Update every 1 second

// Project information - UPDATED
const String PROJECT_NAME = "MELON PENS";
const String PROJECT_VERSION = "v3.0";

// Selective update tracking variables
static String lastLine0 = "";
static String lastLine1 = "";
static String lastLine2 = "";
static String lastLine3 = "";
static bool forceFullRefresh = false;

// LCD Error Handling and Recovery System
unsigned long lastRecoveryAttempt = 0;
int recoveryAttemptCount = 0;
unsigned long lastHealthCheck = 0;
bool lcdConnectionLost = false;

// ==================== UTILITY FUNCTIONS ====================

// Get Firebase connection status indicator
String getFirebaseStatusIndicator()
{
    if (isFirebaseReady())
    {
        return "F"; // Firebase connected
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
        return "W"; // WiFi only
    }
    else
    {
        return "-"; // No connection
    }
}

// Get mode indicator
String getModeIndicator()
{
    return autoModeEnabled ? "A" : "M";
}

// Format sensor value with validation
String formatSensorValue(float value, float minVal, float maxVal, int decimals)
{
    if (isnan(value) || value < minVal || value > maxVal)
    {
        return "--";
    }
    return String(value, decimals);
}

// ==================== LCD ERROR HANDLING FUNCTIONS ====================

// Check if I2C device is connected with timeout
bool checkI2CDevice(uint8_t address, uint16_t timeout_ms)
{
    unsigned long startTime = millis();

    // Clear any pending I2C operations
    Wire.clearWriteError();

    Wire.beginTransmission(address);

    // Check for timeout during transmission
    while (millis() - startTime < timeout_ms)
    {
        byte error = Wire.endTransmission();
        if (error == 0)
        {
            return true; // Device found
        }
        else if (error == 2)
        {
            // Address not acknowledged - device not connected
            Serial.printf("LCD   : 📍 Device at 0x%02X not acknowledged\n", address);
            return false;
        }
        else if (error == 3)
        {
            // Data not acknowledged - possible hardware issue
            Serial.printf("LCD   : 📡 Data not acknowledged for 0x%02X\n", address);
            return false;
        }
        else if (error == 4)
        {
            // Unknown error, retry after brief delay
            delay(50);
            Wire.beginTransmission(address);
            continue;
        }
        else if (error == 5)
        {
            // Hardware timeout - critical error
            Serial.printf("LCD   : ⚡ Hardware timeout for 0x%02X\n", address);
            return false;
        }
        else
        {
            // Other errors
            Serial.printf("LCD   : ❌ I2C error %d for 0x%02X\n", error, address);
            return false;
        }
    }

    // Timeout occurred
    Serial.printf("LCD   : ⏰ I2C timeout for address 0x%02X\n", address);
    return false;
}

// LCD Recovery System
bool recoverLCD()
{
    Serial.println("LCD   : 🔧 Starting LCD recovery process...");

    // Step 1: Reset I2C bus
    Wire.end();
    delay(100);
    Wire.begin();
    delay(100);

    // Step 2: Check if device is back
    if (!checkI2CDevice(LCD_ADDRESS, 500))
    {
        Serial.println("LCD   : ❌ Device still not detected during recovery");
        return false;
    }

    // Step 3: Re-initialize LCD
    try
    {
        lcd.init();
        lcd.backlight();
        lcd.clear();

        // Test write
        lcd.setCursor(0, 0);
        lcd.print("Recovery OK");
        delay(1000);
        lcd.clear();

        Serial.println("LCD   : ✅ LCD recovery successful!");
        return true;
    }
    catch (...)
    {
        Serial.println("LCD   : ❌ LCD re-initialization failed");
        return false;
    }
}

// Attempt LCD recovery with rate limiting
void attemptLcdRecovery()
{
    // Skip if LCD is already ready
    if (lcdReady)
    {
        return;
    }

    // Rate limiting: don't attempt too frequently unless it's a critical failure
    if (millis() - lastRecoveryAttempt < RECOVERY_INTERVAL)
    {
        return; // Too soon, wait longer
    }

    if (recoveryAttemptCount >= MAX_RECOVERY_ATTEMPTS)
    {
        // Give up after max attempts
        if (!lcdConnectionLost)
        {
            Serial.printf("LCD   : ❌ Giving up after %d recovery attempts\n", MAX_RECOVERY_ATTEMPTS);
            Serial.println("LCD   : 💡 Check connections: SDA->GPIO21, SCL->GPIO22, VCC->5V, GND->GND");
            lcdConnectionLost = true;
        }
        return;
    }

    Serial.printf("LCD   : 🔧 Attempting LCD recovery #%d/%d\n",
                  recoveryAttemptCount + 1, MAX_RECOVERY_ATTEMPTS);
    lastRecoveryAttempt = millis();
    recoveryAttemptCount++;

    // Try to re-initialize LCD
    if (recoverLCD())
    {
        lcdReady = true;
        lcdConnectionLost = false;
        recoveryAttemptCount = 0; // Reset counter on success
        forceFullRefresh = true;  // Force full refresh after recovery
        Serial.println("LCD   : 🎉 LCD connection restored!");
    }
    else
    {
        Serial.printf("LCD   : ❌ LCD recovery failed (attempt %d/%d)\n",
                      recoveryAttemptCount, MAX_RECOVERY_ATTEMPTS);
    }
}

// Immediate recovery attempt for critical failures (bypasses rate limiting)
void immediateRecoveryAttempt()
{
    Serial.println("LCD   : 🚨 Critical LCD failure detected - immediate recovery attempt");

    // Reset rate limiting for immediate recovery
    lastRecoveryAttempt = 0;

    // Attempt recovery immediately
    attemptLcdRecovery();
}

// LCD Health Check
bool checkLcdHealth()
{
    if (!lcdReady)
    {
        return false;
    }

    // Quick health check
    if (!checkI2CDevice(LCD_ADDRESS, 100))
    {
        Serial.println("LCD   : ⚠️  Health check failed - device not responding");
        lcdReady = false;
        return false;
    }

    return true;
}

// Handle LCD connection loss - provide fallback behavior
void handleLcdLoss()
{
    if (!lcdReady && !lcdConnectionLost)
    {
        // First time detecting loss
        Serial.println("LCD   : ⚠️  LCD connection lost - attempting recovery");
        lcdConnectionLost = true;
        attemptLcdRecovery();
    }

    // Fallback to Serial output for critical information
    static unsigned long lastSerialUpdate = 0;
    if (millis() - lastSerialUpdate > 10000) // Every 10 seconds
    {
        Serial.println("LCD   : 📺 LCD unavailable - displaying status via Serial");
        Serial.printf("STATUS: pH:%.1f T:%.1f°C TDS:%.0fppm Mode:%s Firebase:%s\n",
                      phValue, temperatureValue, tdsValue,
                      autoModeEnabled ? "AUTO" : "MANUAL",
                      isFirebaseReady() ? "OK" : (WiFi.status() == WL_CONNECTED ? "WiFi" : "OFF"));

        // Show relay states if any active
        bool anyRelay = (relayPumpA_State || relayPumpB_State || relayPumpPH_UP_State ||
                         relayPumpPH_DOWN_State || relayMixerPump_State);
        if (anyRelay)
        {
            Serial.printf("RELAYS: NutA:%s NutB:%s pH+:%s pH-:%s Mix:%s\n",
                          relayPumpA_State ? "ON" : "OFF",
                          relayPumpB_State ? "ON" : "OFF",
                          relayPumpPH_UP_State ? "ON" : "OFF",
                          relayPumpPH_DOWN_State ? "ON" : "OFF",
                          relayMixerPump_State ? "ON" : "OFF");
        }

        lastSerialUpdate = millis();
    }
}

// ==================== LCD DISPLAY FUNCTIONS ====================

// Initialize LCD with comprehensive error handling
void initLCD()
{
    Serial.println("LCD   : Initializing MELON PENS LCD display...");

    // Initialize I2C with error checking
    Wire.begin();
    delay(100);

    // Check if LCD is connected
    if (!checkI2CDevice(LCD_ADDRESS, 200))
    {
        Serial.printf("LCD   : ⚠️  LCD not found at address 0x%02X\n", LCD_ADDRESS);
        Serial.println("LCD   : 🔄 Trying alternative addresses...");

        // Try common LCD addresses
        uint8_t commonAddresses[] = {0x3F, 0x27, 0x20, 0x26};
        bool found = false;

        for (int i = 0; i < 4; i++)
        {
            if (checkI2CDevice(commonAddresses[i], 200))
            {
                Serial.printf("LCD   : ✅ LCD found at address 0x%02X\n", commonAddresses[i]);
                found = true;
                break;
            }
        }

        if (!found)
        {
            Serial.println("LCD   : ❌ LCD not detected - will attempt recovery during operation");
            Serial.println("LCD   : 💡 Check connections: SDA->GPIO21, SCL->GPIO22, VCC->5V, GND->GND");
            lcdReady = false;
            lcdConnectionLost = true;
            return;
        }
    }

    // Try to initialize LCD with error handling
    try
    {
        lcd.init();
        lcd.backlight();
        lcd.clear();

        // Display startup message
        lcd.setCursor(0, 0);
        lcd.print("====INITIALIZING====");
        lcd.setCursor(0, 1);
        lcd.print("====PLEASE WAIT====");
        lcd.setCursor(0, 2);
        lcd.print("====================");
        lcd.setCursor(0, 3);
        lcd.print("====================");

        delay(2000);
        lcd.clear();

        // Set ready flag
        lcdReady = true;
        lcdConnectionLost = false;
        forceFullRefresh = true;
        recoveryAttemptCount = 0; // Reset recovery counter

        Serial.println("LCD   : ✅ MELON PENS LCD initialized successfully");
    }
    catch (...)
    {
        Serial.println("LCD   : ❌ Failed to initialize LCD - will retry during operation");
        lcdReady = false;
        lcdConnectionLost = true;
    }
}

// Enhanced safe LCD print with comprehensive error handling
void safeLcdPrint(int col, int row, String text)
{
    if (!lcdReady)
    {
        attemptLcdRecovery(); // Try recovery if not ready
        return;
    }

    try
    {
        // Pre-check: Verify I2C connection before writing
        if (!checkI2CDevice(LCD_ADDRESS, 50))
        {
            Serial.println("LCD   : ⚠️  I2C device lost during print operation");
            lcdReady = false;
            // Use immediate recovery for hardware disconnection
            immediateRecoveryAttempt();
            return;
        }

        lcd.setCursor(col, row);
        lcd.print(text);
    }
    catch (...)
    {
        Serial.println("LCD   : ⚠️  Error writing to LCD - marking for recovery");
        lcdReady = false;
        // Use immediate recovery for critical errors
        immediateRecoveryAttempt();
    }
} // Selective update function - only updates changed lines
void selectiveUpdateLine(int row, String newContent, String &lastContent)
{
    if (forceFullRefresh || newContent != lastContent)
    {
        // Clear the line first (pad with spaces)
        String paddedContent = newContent;
        while (paddedContent.length() < LCD_COLUMNS)
        {
            paddedContent += " ";
        }
        if (paddedContent.length() > LCD_COLUMNS)
        {
            paddedContent = paddedContent.substring(0, LCD_COLUMNS);
        }

        safeLcdPrint(0, row, paddedContent);
        lastContent = newContent;
    }
}

// ==================== MAIN DISPLAY FUNCTIONS ====================

// Main system display with new MELON PENS layout
// =MELON PENS=[F]=[M]=
// HH:MM:SS--DD/MM/YYYY
// pH:XX.X T:XX TD:XXXX
// A:XXXX/XXXX B:XXXX/XXXX (Auto: CurrentFlow/FuzzySetpointML, Manual: CurrentFlow/ManualSetpointML)
void updateLcdNormal()
{
    // Line 0: Header with status indicators
    String line0 = "=" + PROJECT_NAME + "=[" + getFirebaseStatusIndicator() + "]=[" + getModeIndicator() + "]=";

    // Line 1: Time and Date
    String line1 = formatTimeString() + "--" + formatDateString();

    // Line 2: pH, Temperature, and TDS in one line
    String line2 = "pH:" + formatSensorValue(phValue, 0, 14, 1) +
                   " T:" + formatSensorValue(temperatureValue, -50, 150, 0) +
                   " TD:" + formatSensorValue(tdsValue, 0, 5000, 0);

    // Line 3: NutA and NutB flow values with labels
    String line3 = "";

    // Get flow values for NutA and NutB
    float flowA = getFlowSensorA_mL();
    float flowB = getFlowSensorB_mL();

    String nutADisplay = "";
    String nutBDisplay = "";

    if (autoModeEnabled)
    {
        // Auto mode: current flow / fuzzy setpoint mL
        float targetML_A = getTargetML_A();
        float targetML_B = getTargetML_B();
        nutADisplay = "A:" + String((int)flowA) + "/" + String((int)targetML_A);
        nutBDisplay = "B:" + String((int)flowB) + "/" + String((int)targetML_B);
    }
    else
    {
        // Manual mode: current flow / manual setpoint mL
        nutADisplay = "A:" + String((int)flowA) + "/" + String((int)setpointManual);
        nutBDisplay = "B:" + String((int)flowB) + "/" + String((int)setpointManual);
    }

    // Format: A:XXXX/XXXX  B:XXXX/XXXX
    line3 = nutADisplay;

    // Calculate spacing between NutA and NutB
    int spacesNeeded = LCD_COLUMNS - nutADisplay.length() - nutBDisplay.length();
    if (spacesNeeded < 2)
        spacesNeeded = 2; // Minimum 2 spaces

    for (int i = 0; i < spacesNeeded; i++)
    {
        line3 += " ";
    }
    line3 += nutBDisplay;

    // Selective updates
    selectiveUpdateLine(0, line0, lastLine0);
    selectiveUpdateLine(1, line1, lastLine1);
    selectiveUpdateLine(2, line2, lastLine2);
    selectiveUpdateLine(3, line3, lastLine3);
}

// Relay display with new layout
// Auto mode: A:SSS XXXX A:XXX/XXX (with TDS current/setpoint in middle)
// Manual mode: NutA:SSS   A:XXXX/XXXX (original format)
// ======RELAY=[F]=[M]=
// A:SSS XXXX A:XXX/XXX (auto) or NutA:SSS   A:XXXX/XXXX (manual)
// B:SSS XXXX B:XXX/XXX (auto) or NutB:SSS   B:XXXX/XXXX (manual)
// Mx:SSS p+:SSS p-:SSS
void updateLcdRelay()
{
    // Line 0: Header
    String line0 = "======RELAY=[" + getFirebaseStatusIndicator() + "]=[" + getModeIndicator() + "]=";

    // Get flow values for NutA and NutB separately
    float flowA = getFlowSensorA_mL();
    float flowB = getFlowSensorB_mL();

    String nutAFlow = "";
    String nutBFlow = "";
    String line1 = "";
    String line2 = "";

    if (autoModeEnabled)
    {
        // Auto mode: individual flow / individual fuzzy setpoint mL
        float targetML_A = getTargetML_A();
        float targetML_B = getTargetML_B();
        nutAFlow = "A:" + String((int)flowA) + "/" + String((int)targetML_A);
        nutBFlow = "B:" + String((int)flowB) + "/" + String((int)targetML_B);

        // Get TDS values for auto mode
        int currentTDS = (int)tdsValue;
        int setpointTDS = (int)setPPM;
        String tdsInfo = String(currentTDS) + "/" + String(setpointTDS);

        // Line 1: A:SSS XXXX A:XXX/XXX (auto mode format)
        line1 = "A:" + String(relayPumpA_State ? "ON " : "OFF") + " " + tdsInfo;
        // Calculate spacing to right-align the nutA flow
        int spacesNeeded1 = LCD_COLUMNS - line1.length() - nutAFlow.length();
        if (spacesNeeded1 < 1)
            spacesNeeded1 = 1;
        for (int i = 0; i < spacesNeeded1; i++)
            line1 += " ";
        line1 += nutAFlow;

        // Line 2: B:SSS XXXX B:XXX/XXX (auto mode format)
        line2 = "B:" + String(relayPumpB_State ? "ON " : "OFF") + " " + tdsInfo;
        // Calculate spacing to right-align the nutB flow
        int spacesNeeded2 = LCD_COLUMNS - line2.length() - nutBFlow.length();
        if (spacesNeeded2 < 1)
            spacesNeeded2 = 1;
        for (int i = 0; i < spacesNeeded2; i++)
            line2 += " ";
        line2 += nutBFlow;
    }
    else
    {
        // Manual mode: original format without TDS info
        nutAFlow = String((int)flowA) + "/" + String((int)setpointManual);
        nutBFlow = String((int)flowB) + "/" + String((int)setpointManual);

        // Line 1: NutA:SSS   A:XXXX/XXXX (manual mode format)
        line1 = "NutA:" + String(relayPumpA_State ? "ON " : "OFF");
        int spacesNeeded1 = LCD_COLUMNS - line1.length() - nutAFlow.length();
        if (spacesNeeded1 < 1)
            spacesNeeded1 = 1;
        for (int i = 0; i < spacesNeeded1; i++)
            line1 += " ";
        line1 += nutAFlow;

        // Line 2: NutB:SSS   B:XXXX/XXXX (manual mode format)
        line2 = "NutB:" + String(relayPumpB_State ? "ON " : "OFF");
        int spacesNeeded2 = LCD_COLUMNS - line2.length() - nutBFlow.length();
        if (spacesNeeded2 < 1)
            spacesNeeded2 = 1;
        for (int i = 0; i < spacesNeeded2; i++)
            line2 += " ";
        line2 += nutBFlow;
    }

    // Line 3: Mx:SSS p+:SSS p-:SSS (same for both modes)
    String line3 = "Mx:" + String(relayMixerPump_State ? "ON " : "OFF") +
                   " p+:" + String(relayPumpPH_UP_State ? "ON " : "OFF") +
                   " p-:" + String(relayPumpPH_DOWN_State ? "ON " : "OFF");

    // Selective updates
    selectiveUpdateLine(0, line0, lastLine0);
    selectiveUpdateLine(1, line1, lastLine1);
    selectiveUpdateLine(2, line2, lastLine2);
    selectiveUpdateLine(3, line3, lastLine3);
}

// ==================== MAIN UPDATE FUNCTION ====================

// Main LCD update function with comprehensive error handling
void updateLcdStatus()
{
    // Handle LCD connection issues
    if (!lcdReady)
    {
        handleLcdLoss();
        attemptLcdRecovery();
        return;
    }

    // Periodic health check (every 30 seconds)
    if (millis() - lastHealthCheck > LCD_HEALTH_CHECK_INTERVAL)
    {
        if (!checkLcdHealth())
        {
            Serial.println("LCD   : 🚨 Periodic health check failed - immediate recovery");
            // Use immediate recovery for health check failures
            immediateRecoveryAttempt();
        }
        lastHealthCheck = millis();
    }

    // Only update if enough time has passed
    if (millis() - lastLcdUpdate < LCD_UPDATE_INTERVAL)
    {
        return;
    }

    try
    {
        // Check if any relay is active
        bool anyRelay = (relayPumpA_State || relayPumpB_State || relayPumpPH_UP_State ||
                         relayPumpPH_DOWN_State || relayMixerPump_State);

        if (anyRelay)
        {
            updateLcdRelay();
        }
        else
        {
            updateLcdNormal();
        }

        // Reset force refresh flag after first update
        forceFullRefresh = false;
        lastLcdUpdate = millis();

        // Reset connection lost flag if we successfully updated
        if (lcdConnectionLost)
        {
            lcdConnectionLost = false;
            Serial.println("LCD   : 🎉 LCD connection confirmed working");
        }
    }
    catch (...)
    {
        Serial.println("LCD   : ⚠️  Error during LCD update operation - immediate recovery");
        lcdReady = false;
        // Use immediate recovery for critical update failures
        immediateRecoveryAttempt();
    }
} // Force full refresh (call after LCD recovery or major state changes)
void forceLcdRefresh()
{
    forceFullRefresh = true;
    lastLine0 = "";
    lastLine1 = "";
    lastLine2 = "";
    lastLine3 = "";
    Serial.println("LCD   : 🔄 Full refresh forced");
}

// ==================== LCD DIAGNOSTIC FUNCTIONS ====================

// Get LCD connection status for system monitoring
String getLcdStatusString()
{
    if (lcdReady)
    {
        return "OK";
    }
    else if (lcdConnectionLost)
    {
        return "LOST";
    }
    else
    {
        return "INIT";
    }
}

// Display LCD diagnostic information
void displayLcdDiagnostics()
{
    Serial.println("LCD   : 📊 LCD System Diagnostics:");
    Serial.printf("LCD   :   Status: %s\n", getLcdStatusString().c_str());
    Serial.printf("LCD   :   Ready: %s\n", lcdReady ? "YES" : "NO");
    Serial.printf("LCD   :   Connection Lost: %s\n", lcdConnectionLost ? "YES" : "NO");
    Serial.printf("LCD   :   Recovery Attempts: %d/%d\n", recoveryAttemptCount, MAX_RECOVERY_ATTEMPTS);
    Serial.printf("LCD   :   Last Recovery: %lu ms ago\n", millis() - lastRecoveryAttempt);
    Serial.printf("LCD   :   Last Health Check: %lu ms ago\n", millis() - lastHealthCheck);
    Serial.printf("LCD   :   I2C Address: 0x%02X\n", LCD_ADDRESS);

    // Test I2C connection
    bool i2cOk = checkI2CDevice(LCD_ADDRESS, 100);
    Serial.printf("LCD   :   I2C Test: %s\n", i2cOk ? "PASS" : "FAIL");
}

// Emergency LCD reset - force complete reinitialization
void emergencyLcdReset()
{
    Serial.println("LCD   : 🚨 Emergency LCD reset initiated");

    // Reset all state variables
    lcdReady = false;
    lcdConnectionLost = false;
    recoveryAttemptCount = 0;
    lastRecoveryAttempt = 0;
    lastHealthCheck = 0;

    // Force full refresh
    forceLcdRefresh();

    // Attempt immediate reinitialization
    initLCD();

    Serial.println("LCD   : 🔄 Emergency reset completed");
}

// ==================== SYSTEM INTEGRATION FUNCTIONS ====================

// Check if LCD is available for display updates
bool isLcdAvailable()
{
    return lcdReady && !lcdConnectionLost;
}

// Get LCD readiness for system status reporting
bool getLcdReadiness()
{
    return lcdReady;
}

// Schedule LCD health check (can be called from main loop)
void scheduleLcdHealthCheck()
{
    if (millis() - lastHealthCheck >= LCD_HEALTH_CHECK_INTERVAL)
    {
        checkLcdHealth();
    }
}

// Main LCD update function - call this from main loop
void updateLCD()
{
    // Schedule health check
    scheduleLcdHealthCheck();

    // Only update if LCD is available
    if (!isLcdAvailable())
    {
        return;
    }

    // Update status line on every call
    updateLcdStatus();

    // Update content based on current mode
    if (lcdMode == 0)
    {
        updateLcdNormal();
    }
    else if (lcdMode == 1)
    {
        updateLcdRelay();
    }
}

// Set LCD display mode (0=normal, 1=relay)
void setLcdMode(int mode)
{
    if (mode != lcdMode)
    {
        lcdMode = mode;
        forceLcdRefresh(); // Force refresh when mode changes
        Serial.printf("LCD   : 🔄 Mode changed to %d\n", mode);
    }
}
