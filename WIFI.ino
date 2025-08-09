// ==================== WIFI VARIABLES ====================

// WiFi TIMEOUT
#define WIFI_TIMEOUT_MS 10000

// ESP32 auto-reconnect is enabled in initWiFiHardware()

// ==================== WIFI IMPLEMENTATIONS ====================

// Initialize WiFi hardware (safe to call multiple times)
void initWiFiHardware()
{
    if (!wifiInitialized)
    {
        Serial.println("WIFI  : 🔧 Initializing WiFi hardware...");
        WiFi.mode(WIFI_STA);         // Set WiFi mode to Station
        WiFi.setAutoReconnect(true); // aktifkan auto-reconnect ESP32
        WiFi.persistent(true);       // jangan simpan konfigurasi ke flash
        WiFi.disconnect(true);       // Disconnect and clear any stored credentials
        delay(100);                  // Small delay for stability
        wifiInitialized = true;
        Serial.println("WIFI  : ✅ WiFi hardware initialized with auto-reconnect");
    }
}

void connectWiFi()
{
    Serial.printf("WIFI  : 🔌 Connecting to WiFi: %s\n", ssidWIFI.c_str());
    WiFi.begin(ssidWIFI.c_str(), passWIFI.c_str());

    unsigned long startAttemptTime = millis();
    const unsigned long timeout = WIFI_TIMEOUT_MS;
    Serial.print("WIFI  : .");

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout)
    {
        Serial.print(".");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("WIFI  : ✅ Connected to %s\n", WiFi.SSID().c_str());
        Serial.printf("WIFI  : 📡 IP Address: %s\n", WiFi.localIP().toString().c_str());

        initNTP();

        // Initialize Firebase only once for memory stability
        if (!firebaseInitialized)
        {
            Serial.println("WIFI  : 🔥 Initializing Firebase (first time)...");
            initFirebase();
            firebaseInitialized = true;
        }
        else
        {
            Serial.println("WIFI  : 🔄 Firebase already initialized, setting up streams...");
            // Just setup streams on reconnect to preserve memory
            setupFirebaseStreams();
        }

        // statusWiFi() dihapus untuk mencegah circular calls
        // Status akan di-update otomatis oleh WiFi task
    }
    else
    {
        Serial.println("WIFI  : ❌ WiFi Not Connected.");
    }
}

void disconnectWiFi()
{
    // Force aggressive disconnect
    endAllStreams();
    WiFi.disconnect(); // true = erase stored credentials

    Serial.println("WIFI  : 🔌 Manually Disconnected (Aggressive)");
    Serial.println("WIFI  : ℹ️ Auto-reconnect disabled. Use 'connect' command to reconnect.");
}

void statusWiFi()
{
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED)
    {
        ipWIFI = WiFi.localIP().toString();
        int rssi = WiFi.RSSI();
        rssiWIFI = String(rssi) + " dBm";

        if (rssi > -50)
            signalWIFI = "Excellent";
        else if (rssi > -70)
            signalWIFI = "Good";
        else if (rssi > -80)
            signalWIFI = "Fair";
        else
            signalWIFI = "Weak";

        statusTextWIFI = "Connected";
    }
    else
    {
        ipWIFI = "-";
        rssiWIFI = "-";
        signalWIFI = "No Signal";
        statusTextWIFI = "Disconnected";
        // ESP32 auto-reconnect akan menangani reconnect
        Serial.println("WIFI  : ❌ WiFi disconnected - auto-reconnect enabled");
    }
}

// Function to get WiFi status as string
String getWiFiStatusString()
{
    return "IP:" + ipWIFI + " Signal:" + signalWIFI;
}
