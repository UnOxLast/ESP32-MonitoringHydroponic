// Include
#include <WiFi.h>
#include <time.h>
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include "Global.h"

// Task handles
TaskHandle_t mainTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;
TaskHandle_t lcdTaskHandle = NULL;

// Mutex untuk sinkronisasi
SemaphoreHandle_t dataMutex;

// Timing variables untuk main task scheduling
unsigned long lastDataSend = 0;
unsigned long lastFuzzyRun = 0;
unsigned long lastSensorRead = 0;
unsigned long lastNtpUpdate = 0;

// LCD State variables (defined here for global access)
LcdState currentLcdState = LCD_BOOTING;
LcdState previousLcdState = LCD_BOOTING;

// ==================== TASK IMPLEMENTATIONS ====================

// Main Task - Full real-time stream-optimized system logic with handler function calls
void mainTask(void *parameter)
{
    Serial.println("MAIN  : Main Task started (Full Real-time Stream Architecture)");

    // Initialize timing variables
    lastDataSend = millis();
    lastFuzzyRun = millis();
    lastSensorRead = millis();
    lastNtpUpdate = millis();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms loop for real-time stream processing

    for (;;)
    {
        unsigned long currentTime = millis();

        // Handle real-time Firebase streams (every loop for instant response < 100ms)
        // This processes ALL changes instantly: mode, relay, TDS setpoint, manual setpoints
        handleRealTimeProcessing();

        // Handle sensor reading (every 1 second)
        if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL)
        {
            handleSensorReading();
            // Serial.println(fullTimeStr);
            lastSensorRead = currentTime;
        }

        // Handle NTP time update (every 500ms)
        if (currentTime - lastNtpUpdate >= NTP_UPDATE_INTERVAL)
        {
            handleNtpUpdate();
            lastNtpUpdate = currentTime;
        }

        // Handle data sending (every 1 minute)
        if (currentTime - lastDataSend >= DATA_SEND_INTERVAL)
        {
            handleDataSend(); // used but commented because doesnt want to send data to firebase
            lastDataSend = currentTime;
        }

        // Handle fuzzy logic execution (auto mode only)
        // Trigger: 1) Every 1 minute, OR 2) Immediate trigger when switching to auto
        if (isAutoModeActive() &&
            ((currentTime - lastFuzzyRun >= FUZZY_RUN_INTERVAL) || triggerFuzzyExecution))
        {
            if (triggerFuzzyExecution)
            {
                Serial.println("FUZZY : 🚀 Immediate fuzzy execution triggered (mode switch)");
                triggerFuzzyExecution = false; // Reset trigger
            }

            handleFuzzyExecution();
            lastFuzzyRun = currentTime;
        }

        // Flow sensor health check (every 30 seconds)
        static unsigned long lastFlowHealthCheck = 0;
        if (currentTime - lastFlowHealthCheck >= 30000) // 30 seconds
        {
            if (!checkFlowSensorHealth())
            {
                Serial.println("MAIN  : ⚠️  Flow sensor health check failed - values may be corrupted");
                Serial.println("MAIN  : 💡 Use 'flow reset' command if needed");
            }
            lastFlowHealthCheck = currentTime;
        }

        // Handle unified pump execution (both manual and auto mode use same chasing logic)
        handleUnifiedPumpExecution();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// WiFi Task - Handle koneksi dan monitoring
void wifiTask(void *parameter)
{
    Serial.println("WIFI  : WiFi Task started");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(60000); // 60 detik

    for (;;)
    {
        // Panggil statusWiFi() yang akan menentukan logika apa yang harus dilakukan
        statusWiFi();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Serial Task - Handle komunikasi serial
void serialTask(void *parameter)
{
    Serial.println("SERIAL: Serial Task started");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms

    for (;;)
    {
        // Process serial input
        processSerialInput();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// LCD Task - Handle LCD display updates with new MELON PENS system
void lcdTask(void *parameter)
{
    Serial.println("LCD   : LCD Task started (MELON PENS Display System)");

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // 1 second updates

    for (;;)
    {
        // Use new comprehensive LCD update function
        // This handles error recovery, health checks, and display updates
        updateLCD();

        // Auto-switch to relay mode when any relay is active
        bool anyRelay = (relayPumpA_State || relayPumpB_State || relayPumpPH_UP_State ||
                         relayPumpPH_DOWN_State || relayMixerPump_State);

        if (anyRelay && lcdMode != 1)
        {
            setLcdMode(1); // Switch to relay view
        }
        else if (!anyRelay && lcdMode != 0)
        {
            setLcdMode(0); // Switch back to normal view
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ==================== SETUP & LOOP ====================

void setup()
{
    Serial.begin(115200);

    Serial.println("SETUP : Starting RTOS with Full Real-time Stream Architecture");
    Serial.println("SETUP : 🚀 Complete zero-delay Firebase control enabled");
    Serial.println("SETUP : 📱 All Flutter app commands will be instant");
    Serial.println("SETUP : 📺 MELON PENS LCD with robust error handling");
    delay(1000);

    initSerial();
    initLCD(); // Initialize new MELON PENS LCD system
    initTemperatureSensor();
    initPhSensor();
    initTdsSensor();
    initFlowSensors();       // Initialize flow sensors
    initRelays();            // Initialize relay system
    initializeFuzzySystem(); // Initialize enhanced fuzzy logic controller (waterflow-enabled)
    initWiFiHardware();
    connectWiFi();

    delay(1000);

    // Display LCD status after initialization
    Serial.printf("SETUP : LCD Status: %s (Mode: %d)\n", getLcdStatusString().c_str(), lcdMode);

    // Create mutex
    dataMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
        lcdTask,        // Task function
        "LCDTask",      // Task name
        4096,           // Stack size
        NULL,           // Parameters
        2,              // Priority (normal)
        &lcdTaskHandle, // Task handle
        1               // Core ID (core 1)
    );

    xTaskCreatePinnedToCore(
        serialTask,        // Task function
        "SerialTask",      // Task name
        8192,              // Stack size
        NULL,              // Parameters
        1,                 // Priority (lowest)
        &serialTaskHandle, // Task handle
        1                  // Core ID (core 1)
    );

    xTaskCreatePinnedToCore(
        wifiTask,        // Task function
        "WiFiTask",      // Task name
        4096,            // Stack size
        NULL,            // Parameters
        3,               // Priority (highest - critical for Firebase connection)
        &wifiTaskHandle, // Task handle
        0                // Core ID (core 0)
    );

    // Create tasks with optimized priority for stream processing
    xTaskCreatePinnedToCore(
        mainTask,        // Task function
        "MainTask",      // Task name
        20480,           // Stack size (increased for stream processing)
        NULL,            // Parameters
        2,               // Priority (increased for real-time streams)
        &mainTaskHandle, // Task handle
        1                // Core ID (core 1)
    );
}

void loop()
{
    // Empty loop - all processing handled by optimized RTOS tasks
    // Full real-time Firebase streams processed in mainTask every 100ms
    // ALL data (mode, relay, TDS setpoint, manual setpoints) now instant
    // WiFi management in wifiTask every 60s
    // Serial processing in serialTask every 100ms
    // LCD updates with MELON PENS system in lcdTask every 1s
    //   - Includes robust error handling and auto-recovery
    //   - Auto-switches between normal/relay modes
    //   - Serial fallback when LCD disconnected
}