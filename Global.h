#ifndef GLOBAL_H
#define GLOBAL_H

//========================= DEFINE ============================//
// Sensor pins
#define oneWireBus 15   // DS18B20 temperature sensor pin
#define phSensorPin 35  // pH sensor analog pin (ADC1_CH7)
#define tdsSensorPin 34 // TDS sensor analog pin (ADC1_CH6)

// Water flow sensor pins
#define flowSensorA_Pin 4        // Nutrisi A flow meter
#define flowSensorB_Pin 18       // Nutrisi B flow meter
#define flowSensorPH_UP_Pin 2    // pH UP solution flow meter
#define flowSensorPH_DOWN_Pin 19 // pH DOWN solution flow meter

// Relay/Actuator pins
#define relayPumpA_Pin 33       // Nutrisi A pump relay
#define relayPumpB_Pin 26       // Nutrisi B pump relay
#define relayPumpPH_UP_Pin 25   // pH UP pump relay
#define relayPumpPH_DOWN_Pin 27 // pH DOWN pump relay
#define relayMixerPump_Pin 14   // Water mixer pump relay

// Handler timing configuration (optimized for stream-based architecture)
const unsigned long SENSOR_READ_INTERVAL = 1000;  // Read sensors every 1 second
const unsigned long NTP_UPDATE_INTERVAL = 500;    // Update NTP time every 500ms
const unsigned long MODE_CHECK_INTERVAL = 30000;  // Check mode every 30 seconds (reduced, streams handle real-time)
const unsigned long CONFIG_SYNC_INTERVAL = 60000; // Sync config every 60 seconds (reduced, only non-critical data)
const unsigned long DATA_SEND_INTERVAL = 3600000; // Send data every 1 hour
const unsigned long FUZZY_RUN_INTERVAL = 60000;   // Run fuzzy every 1 minute

// LCD error handling configuration
const unsigned long LCD_HEALTH_CHECK_INTERVAL = 30000; // Health check every 30 seconds
const unsigned long RECOVERY_INTERVAL = 5000;          // Try recovery every 5 seconds
const int MAX_RECOVERY_ATTEMPTS = 3;                   // Maximum recovery attempts

// Temperature sensor variables
float temperatureValue = 0.0;
bool temperatureSensorReady = false;

// pH sensor variables
float phValue = 7.0;
float phVoltage = 0.0;
bool phSensorReady = false;

// TDS sensor variables
float tdsValue = 0.0;
float tdsVoltage = 0.0;
bool tdsSensorReady = false;

// Water flow sensor variables
volatile unsigned long flowCountA = 0;       // Nutrisi A pulse count
volatile unsigned long flowCountB = 0;       // Nutrisi B pulse count
volatile unsigned long flowCountPH_UP = 0;   // pH UP pulse count
volatile unsigned long flowCountPH_DOWN = 0; // pH DOWN pulse count

float flowRateA = 0.0;       // L/min
float flowRateB = 0.0;       // L/min
float flowRatePH_UP = 0.0;   // L/min
float flowRatePH_DOWN = 0.0; // L/min

float totalVolumeA = 0.0;       // Total volume in liters
float totalVolumeB = 0.0;       // Total volume in liters
float totalVolumePH_UP = 0.0;   // Total volume in liters
float totalVolumePH_DOWN = 0.0; // Total volume in liters

bool flowSensorsReady = false;

// Relay/Actuator variables
bool relayPumpA_State = false;       // Nutrisi A pump state (false=OFF, true=ON)
bool relayPumpB_State = false;       // Nutrisi B pump state
bool relayPumpPH_UP_State = false;   // pH UP pump state
bool relayPumpPH_DOWN_State = false; // pH DOWN pump state
bool relayMixerPump_State = false;   // Water mixer pump state
bool relaysReady = false;            // Relay system initialization status

// WiFi credentials (ganti dengan kredensial Anda)
String ssidWIFI = "Bangminta";
String passWIFI = "12345678";
// WiFi status variables
String ipWIFI = "-";
String rssiWIFI = "-";
String signalWIFI = "No Signal";
String statusTextWIFI = "Disconnected";

// Time variables (unified time management)
int currentYear = 1970;
int currentMonth = 1;
int currentDay = 1;
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;
String currentDayName = "";
String currentMonthName = "";
String fullTimeStr = "";
String fullDateStr = "";

// Handler state variables
bool autoModeEnabled = false;
bool manualModeEnabled = false;

// Fuzzy Logic tracking variables
bool fuzzyProcessActive = false;      // Track if fuzzy logic is currently processing
unsigned long lastFuzzyExecution = 0; // Last fuzzy execution timestamp
bool fuzzySystemReady = false;        // Fuzzy system initialization status
extern float setPPM;                  // TDS setpoint for auto mode (defined in FUZZY.ino)
extern float setpointManual;          // Manual setpoint for manual mode (defined in FIREBASE.ino)

// Firebase tracking variables
bool firebaseReady = false;    // Firebase initialization status
bool streamSetup = false;      // Track if streams are initialized
String currentMode = "manual"; // Current operation mode

// WiFi tracking variables
bool wifiInitialized = false; // WiFi initialization status

// Fuzzy logic trigger
bool triggerFuzzyExecution = false; // Trigger immediate fuzzy execution when switching to auto

// LCD system tracking variables
bool lcdReady = false;               // LCD hardware status
bool sensorsInitialized = false;     // All sensors initialization status
bool firebaseInitialized = false;    // Firebase system ready status
bool relaysInitialized = false;      // Relay system ready status
bool systemFullyInitialized = false; // Complete system initialization status
int lcdMode = 0;                     // LCD display mode (0=normal, 1=relay)

// LCD state machine variables
bool wifiEventTriggered = false; // WiFi event popup trigger
String lastWifiEvent = "";       // Last WiFi event message

// LCD timing and state variables
unsigned long stateStartTime = 0;       // State machine timing
unsigned long bootingDuration = 3000;   // Booting screen duration (3 seconds)
unsigned long wifiPopupDuration = 5000; // WiFi popup duration (5 seconds)

// LCD State Management System
enum LcdState
{
    LCD_BOOTING,     // Initial startup with project name and version
    LCD_MAIN_MENU,   // Normal operation (auto-cycle)
    LCD_WIFI_POPUP,  // WiFi status popup (5 seconds)
    LCD_RELAY_ACTIVE // When relays are active (auto/manual)
};

// LCD state variables
extern LcdState currentLcdState;
extern LcdState previousLcdState;

//====================== VARIABLE ========================//
// TODO: Tambahkan variabel global di sini jika diperlukan

// ====================== TIMING VARIABLES ========================//
// Timing variables untuk main task scheduling
extern unsigned long lastModeCheck;
extern unsigned long lastConfigSync;
extern unsigned long lastDataSend;
extern unsigned long lastFuzzyRun;
extern unsigned long lastSensorRead;
extern unsigned long lastNtpUpdate;

//====================== FUNCTION DECLARATION ========================//
// WiFi functions
void initWiFiHardware();
void connectWiFi();
void disconnectWiFi();
void reconnectWiFi();
void statusWiFi();

// NTP functions
void initNTP();
void updateTime();
void parseCompileTime();
String getFirebaseDateTime();
String formatTimeString();
String formatDateString();

// Utility functions
String getWiFiStatusString();

// Serial functions
void initSerial();
void processSerialInput();
void parseSerialCommand(String command);

// Sensor functions
void initTemperatureSensor();
float readTemperature();
bool isTemperatureSensorReady();

// pH sensor functions
void initPhSensor();
float readPhSensor();
bool isPhSensorReady();

// TDS sensor functions
void initTdsSensor();
float readTdsSensor();
bool isTdsSensorReady();

// Water flow sensor functions
void initFlowSensors();
void updateFlowRates();
void resetFlowCounters();
bool areFlowSensorsReady();
float getFlowSensorA_mL();
float getFlowSensorB_mL();
float getFlowSensorPH_UP_mL();
float getFlowSensorPH_DOWN_mL();
void resetFlowSensors();
void emergencyFlowReset();    // Emergency reset for flow sensor overflow
bool checkFlowSensorHealth(); // Health check for flow sensors

// Relay/Actuator functions
void initRelays();
void setRelay(int relayNumber, bool state);
void setRelayPumpA(bool state);
void setRelayPumpB(bool state);
void setRelayPumpPH_UP(bool state);
void setRelayPumpPH_DOWN(bool state);
void setRelayMixerPump(bool state);
void turnOffAllRelays();
bool areRelaysReady();

// Fuzzy Logic functions
void initFuzzyLogic();
float runFuzzy(float ppm, int setppm);
float runFuzzyWithDebug(float ppm, int setppm, bool enableDebug);
void getFuzzyStatus();
void setTdsSetpoint(int newSetpoint);
int getTdsSetpoint();
float getTargetML_A();
float getTargetML_B();
bool startFuzzyTDSControl(float targetTDS, float currentTDS);
bool executeFuzzyIteration(float currentTDS);
void stopFuzzyProcess();
String getOutputCategory(float output);

// Firebase functions
void initFirebase();
void updateFirebaseTime();
void sendSensorDataToFirebase();
void sendFuzzyLogToFirebase();
void controlRelays(int relayIndex, bool state);
void changeMode(String mode);
bool isFirebaseReady();
void getFirebaseStatus();

// Handler functions
void handleSensorReading();
void handleNtpUpdate();
void handleDataSend();
void initializeFuzzySystem();
void handleFuzzyExecution();
void updateAutoRelayState(int relayIndex, bool state);
void emergencyStop();
void resumeOperations();
bool isAutoModeActive();
void refreshMode();
void executeWaterflowControl();
bool executeNutrientPumping(float targetML_A, float targetML_B);
void handleManualExecution();
bool executeManualPump(int pumpIndex, float targetML);
void handleUnifiedPumpExecution(); // Unified pump execution for auto/manual

// Handler stream optimization functions
void getHandlerStreamStatus();

// Real-time Firebase functions
void setupFirebaseStreams();
void endAllStreams();
void setupModeStream();
void setupManualModeStreams();
void setupAutoModeStreams();
void reinitializeStreamsForMode();
void handleFirebaseStreams();
void handleRealTimeControl();
bool isRealTimeActive();
void getRealTimeStatus();
void handleRealTimeProcessing();

// Memory monitoring functions
void checkMemoryStatus();

// Manual setpoint access functions
float getManualSetpoint();

// LCD display functions
void initLCD();
void safeLcdPrint(int col, int row, String text);
void safeLcdClear();
void updateLcdStatus();
void displaySensorPage();
void displayRelayPage();
void updateLcdNormal();
void updateLcdFuzzy();
void updateLcdManual();
void cycleLcdPage();
String getSensorDisplay(float value, float minVal, float maxVal, int decimals, String unit);
bool anyRelayActive();
bool isFuzzyProcessActive();
void checkSystemInitialization();

// Enhanced LCD functions (new MELON PENS system)
void updateLCD();              // Main LCD update function
void setLcdMode(int mode);     // Set LCD mode (0=normal, 1=relay)
bool isLcdAvailable();         // Check if LCD is ready
bool getLcdReadiness();        // Get LCD readiness status
void forceLcdRefresh();        // Force full LCD refresh
void emergencyLcdReset();      // Emergency LCD reset
String getLcdStatusString();   // Get LCD status as string
void displayLcdDiagnostics();  // Display LCD diagnostics
void scheduleLcdHealthCheck(); // Schedule LCD health check

// LCD state machine functions
void displayBootingScreen();
void displayWifiPopupScreen();
bool isRelaySystemActive();
void transitionToState(LcdState newState);
String getStateName(LcdState state);
void handleBootingState();
void handleMainMenuState();
void handleWifiPopupState();
void handleRelayActiveState();

// LCD Recovery System functions
bool checkI2CDevice(uint8_t address, uint16_t timeout_ms = 100);
bool recoverLCD();
void attemptLcdRecovery();
bool checkLcdHealth();
void handleLcdLoss();

#endif
