// ==================== SERIAL VARIABLES ====================

String serialBuffer = "";
bool serialComplete = false;

// ==================== SERIAL IMPLEMENTATIONS ====================

void initSerial()
{
    // Serial sudah diinit di setup(), jadi hanya print info
    Serial.println("SERIAL: ✅ Serial communication ready");
    Serial.println("SERIAL: Type 'help' for available commands");
}

void processSerialInput()
{
    while (Serial.available())
    {
        char inChar = (char)Serial.read();

        if (inChar == '\n' || inChar == '\r')
        {
            if (serialBuffer.length() > 0)
            {
                serialComplete = true;
                break;
            }
        }
        else
        {
            serialBuffer += inChar;
        }
    }

    if (serialComplete)
    {
        parseSerialCommand(serialBuffer);
        serialBuffer = "";
        serialComplete = false;
    }
}

void parseSerialCommand(String command)
{
    command.trim();
    // command.toLowerCase();

    Serial.println("SERIAL: Received command: " + command);

    if (command.startsWith("set ssid "))
    {
        String newSSID = command.substring(9);
        newSSID.trim();

        if (newSSID.length() > 0)
        {
            ssidWIFI = newSSID;
            Serial.println("SERIAL: ✅ SSID updated to: " + ssidWIFI);
            Serial.println("SERIAL: Please reconnect WiFi to apply changes");
        }
        else
        {
            Serial.println("SERIAL: ❌ SSID cannot be empty");
        }
    }
    else if (command.startsWith("set pass "))
    {
        String newPass = command.substring(9);
        newPass.trim();

        if (newPass.length() > 0)
        {
            passWIFI = newPass;
            Serial.println("SERIAL: ✅ Password updated");
            Serial.println("SERIAL: Please reconnect WiFi to apply changes");
        }
        else
        {
            Serial.println("SERIAL: ❌ Password cannot be empty");
        }
    }
    else if (command == "help")
    {
        Serial.println("SERIAL: 📖 Available Commands:");
        Serial.println("SERIAL: ==========================================");
        Serial.println("SERIAL: WiFi Commands:");
        Serial.println("SERIAL: - set ssid <WIFI_NAME>");
        Serial.println("SERIAL: - set pass <WIFI_PASSWORD>");
        Serial.println("SERIAL: - connect (connect to WiFi)");
        Serial.println("SERIAL: - disconnect (disconnect from WiFi)");
        Serial.println("SERIAL: - show wifi (show current WiFi settings)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: Sensor Commands:");
        Serial.println("SERIAL: - show sensors (show all sensor readings)");
        Serial.println("SERIAL: - reset flow (reset flow counters)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: Relay Commands:");
        Serial.println("SERIAL: - relay <1-5> on/off (control individual relay)");
        Serial.println("SERIAL: - pump a on/off (control Nutrisi A pump)");
        Serial.println("SERIAL: - pump b on/off (control Nutrisi B pump)");
        Serial.println("SERIAL: - pump ph-up on/off (control pH UP pump)");
        Serial.println("SERIAL: - pump ph-down on/off (control pH DOWN pump)");
        Serial.println("SERIAL: - pump mixer on/off (control mixer pump)");
        Serial.println("SERIAL: - show relays (show all relay states)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: Fuzzy Logic Commands:");
        Serial.println("SERIAL: - set tds <ppm> (set TDS setpoint)");
        Serial.println("SERIAL: - show fuzzy (show fuzzy logic status)");
        Serial.println("SERIAL: - test fuzzy (run fuzzy with current TDS)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: Firebase Commands:");
        Serial.println("SERIAL: - show firebase (show Firebase status)");
        Serial.println("SERIAL: - show realtime (show real-time control status)");
        Serial.println("SERIAL: - show handler (show handler stream optimization)");
        Serial.println("SERIAL: - stream status (show comprehensive stream status)");
        Serial.println("SERIAL: - upload data (upload sensor data to Firebase)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: System Commands:");
        Serial.println("SERIAL: - refresh mode (force mode refresh from Firebase)");
        Serial.println("SERIAL: - emergency stop (emergency stop all operations)");
        Serial.println("SERIAL: - resume ops (resume normal operations)");
        Serial.println("SERIAL: - show memory (show memory status)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: LCD Commands:");
        Serial.println("SERIAL: - lcd mode <0|1> (set LCD mode: 0=normal, 1=relay)");
        Serial.println("SERIAL: - lcd refresh (force LCD refresh)");
        Serial.println("SERIAL: - lcd reset (emergency LCD reset)");
        Serial.println("SERIAL: - lcd status (show LCD status)");
        Serial.println("SERIAL: - lcd diag (show LCD diagnostics)");
        Serial.println("SERIAL: ");
        Serial.println("SERIAL: General Commands:");
        Serial.println("SERIAL: - help");
        Serial.println("SERIAL: ==========================================");
    }
    else if (command == "connect")
    {
        Serial.println("SERIAL: 🔌 Connecting to WiFi...");
        connectWiFi();
    }
    else if (command == "disconnect")
    {
        Serial.println("SERIAL: 🔌 Disconnecting from WiFi...");
        disconnectWiFi();
    }
    else if (command == "show wifi")
    {
        Serial.println("SERIAL: Current WiFi settings:");
        Serial.println("SERIAL: SSID: " + ssidWIFI);
        Serial.println("SERIAL: Status: " + statusTextWIFI);
        if (statusTextWIFI == "Connected")
        {
            Serial.println("SERIAL: IP: " + ipWIFI);
            Serial.println("SERIAL: Signal: " + signalWIFI + " (" + rssiWIFI + ")");
        }
    }
    else if (command == "show sensors")
    {
        Serial.println("SERIAL: 🌡️ Current sensor readings:");
        Serial.printf("SERIAL:   Temperature: %.2f°C\n", temperatureValue);
        Serial.printf("SERIAL:   pH: %.2f\n", phValue);
        Serial.printf("SERIAL:   TDS: %.0f ppm\n", tdsValue);
        Serial.println("SERIAL: 💧 Flow rates (L/min):");
        Serial.printf("SERIAL:   Nutrisi A: %.3f (Total: %.3f L)\n", flowRateA, totalVolumeA);
        Serial.printf("SERIAL:   Nutrisi B: %.3f (Total: %.3f L)\n", flowRateB, totalVolumeB);
        Serial.printf("SERIAL:   pH UP: %.3f (Total: %.3f L)\n", flowRatePH_UP, totalVolumePH_UP);
        Serial.printf("SERIAL:   pH DOWN: %.3f (Total: %.3f L)\n", flowRatePH_DOWN, totalVolumePH_DOWN);
    }
    else if (command == "reset flow")
    {
        Serial.println("SERIAL: 🔄 Resetting flow counters...");
        resetFlowCounters();
    }
    else if (command.startsWith("relay "))
    {
        // Parse relay command: "relay 1 on" or "relay 3 off"
        String relayCmd = command.substring(6); // Remove "relay "
        int spaceIndex = relayCmd.indexOf(' ');

        if (spaceIndex > 0)
        {
            int relayNum = relayCmd.substring(0, spaceIndex).toInt();
            String state = relayCmd.substring(spaceIndex + 1);
            state.trim();

            if (relayNum >= 1 && relayNum <= 5)
            {
                if (state == "on")
                {
                    Serial.printf("SERIAL: 🔄 Turning relay %d ON\n", relayNum);
                    setRelay(relayNum, true);
                }
                else if (state == "off")
                {
                    Serial.printf("SERIAL: 🔄 Turning relay %d OFF\n", relayNum);
                    setRelay(relayNum, false);
                }
                else
                {
                    Serial.println("SERIAL: ❌ Invalid state. Use 'on' or 'off'");
                }
            }
            else
            {
                Serial.println("SERIAL: ❌ Invalid relay number. Use 1-5");
            }
        }
        else
        {
            Serial.println("SERIAL: ❌ Invalid format. Use: relay <1-5> on/off");
        }
    }
    else if (command.startsWith("pump "))
    {
        // Parse pump command: "pump a on", "pump ph-up off", etc.
        String pumpCmd = command.substring(5); // Remove "pump "
        int spaceIndex = pumpCmd.indexOf(' ');

        if (spaceIndex > 0)
        {
            String pumpName = pumpCmd.substring(0, spaceIndex);
            String state = pumpCmd.substring(spaceIndex + 1);
            state.trim();

            bool turnOn = (state == "on");
            bool validState = (state == "on" || state == "off");

            if (!validState)
            {
                Serial.println("SERIAL: ❌ Invalid state. Use 'on' or 'off'");
            }
            else if (pumpName == "a")
            {
                Serial.printf("SERIAL: 🔄 Turning Nutrisi A pump %s\n", state.c_str());
                setRelayPumpA(turnOn);
            }
            else if (pumpName == "b")
            {
                Serial.printf("SERIAL: 🔄 Turning Nutrisi B pump %s\n", state.c_str());
                setRelayPumpB(turnOn);
            }
            else if (pumpName == "ph-up")
            {
                Serial.printf("SERIAL: 🔄 Turning pH UP pump %s\n", state.c_str());
                setRelayPumpPH_UP(turnOn);
            }
            else if (pumpName == "ph-down")
            {
                Serial.printf("SERIAL: 🔄 Turning pH DOWN pump %s\n", state.c_str());
                setRelayPumpPH_DOWN(turnOn);
            }
            else if (pumpName == "mixer")
            {
                Serial.printf("SERIAL: 🔄 Turning mixer pump %s\n", state.c_str());
                setRelayMixerPump(turnOn);
            }
            else
            {
                Serial.println("SERIAL: ❌ Invalid pump name. Use: a, b, ph-up, ph-down, mixer");
            }
        }
        else
        {
            Serial.println("SERIAL: ❌ Invalid format. Use: pump <name> on/off");
        }
    }
    else if (command == "show relays")
    {
        Serial.println("SERIAL: ⚡ Current relay states:");
        Serial.printf("SERIAL:   Relay 1 (Pump A): %s\n", relayPumpA_State ? "ON" : "OFF");
        Serial.printf("SERIAL:   Relay 2 (Pump B): %s\n", relayPumpB_State ? "ON" : "OFF");
        Serial.printf("SERIAL:   Relay 3 (pH UP): %s\n", relayPumpPH_UP_State ? "ON" : "OFF");
        Serial.printf("SERIAL:   Relay 4 (pH DOWN): %s\n", relayPumpPH_DOWN_State ? "ON" : "OFF");
        Serial.printf("SERIAL:   Relay 5 (Mixer): %s\n", relayMixerPump_State ? "ON" : "OFF");
    }
    else if (command.startsWith("set tds "))
    {
        // Parse TDS setpoint command: "set tds 1200"
        String tdsCmd = command.substring(8); // Remove "set tds "
        tdsCmd.trim();

        int newSetpoint = tdsCmd.toInt();
        if (newSetpoint > 0 && newSetpoint <= 2000)
        {
            Serial.printf("SERIAL: 🎯 Setting TDS setpoint to %d ppm\n", newSetpoint);
            setTdsSetpoint(newSetpoint);
        }
        else
        {
            Serial.println("SERIAL: ❌ Invalid TDS setpoint. Use range: 1-2000 ppm");
        }
    }
    else if (command == "show fuzzy")
    {
        Serial.println("SERIAL: 🧠 Fuzzy logic status:");
        getFuzzyStatus();
    }
    else if (command == "test fuzzy")
    {
        Serial.println("SERIAL: 🧪 Testing fuzzy logic with current TDS...");
        float output = runFuzzyWithDebug(tdsValue, getTdsSetpoint(), true);
        Serial.printf("SERIAL: Recommended pump output: %.1f%%\n", output);
    }
    else if (command == "show firebase")
    {
        Serial.println("SERIAL: 🔥 Firebase status:");
        getFirebaseStatus();
    }
    else if (command == "show realtime")
    {
        Serial.println("SERIAL: ⚡ Real-time control status:");
        getRealTimeStatus();
    }
    else if (command == "show handler")
    {
        Serial.println("SERIAL: 🔧 Handler stream optimization:");
        getHandlerStreamStatus();
    }
    else if (command == "stream status")
    {
        Serial.println("SERIAL: 📊 Comprehensive real-time stream status:");
        Serial.println("SERIAL: ==========================================");
        getRealTimeStatus();
        Serial.println("SERIAL: ");
        getHandlerStreamStatus();
        Serial.println("SERIAL: ==========================================");
    }
    else if (command == "upload data")
    {
        Serial.println("SERIAL: ⬆️ Uploading sensor data to Firebase...");
        sendSensorDataToFirebase();
    }
    else if (command == "refresh mode")
    {
        Serial.println("SERIAL: 🔄 Refreshing mode from Firebase...");
        refreshMode();
    }
    else if (command == "emergency stop")
    {
        Serial.println("SERIAL: 🚨 ACTIVATING EMERGENCY STOP...");
        emergencyStop();
    }
    else if (command == "resume ops")
    {
        Serial.println("SERIAL: ▶️ Resuming normal operations...");
        resumeOperations();
    }
    else if (command == "show memory")
    {
        Serial.println("SERIAL: 🧠 Memory status:");
        checkMemoryStatus();
    }
    else if (command.startsWith("lcd mode "))
    {
        String modeStr = command.substring(9);
        int mode = modeStr.toInt();
        if (mode == 0 || mode == 1)
        {
            Serial.printf("SERIAL: 📺 Setting LCD mode to %d\n", mode);
            setLcdMode(mode);
        }
        else
        {
            Serial.println("SERIAL: ❌ Invalid LCD mode. Use 0 (normal) or 1 (relay)");
        }
    }
    else if (command == "lcd refresh")
    {
        Serial.println("SERIAL: 📺 Forcing LCD refresh...");
        forceLcdRefresh();
    }
    else if (command == "lcd reset")
    {
        Serial.println("SERIAL: 📺 Performing emergency LCD reset...");
        emergencyLcdReset();
    }
    else if (command == "lcd status")
    {
        Serial.println("SERIAL: 📺 LCD status:");
        Serial.printf("SERIAL: Status: %s\n", getLcdStatusString().c_str());
        Serial.printf("SERIAL: Ready: %s\n", getLcdReadiness() ? "YES" : "NO");
        Serial.printf("SERIAL: Available: %s\n", isLcdAvailable() ? "YES" : "NO");
        Serial.printf("SERIAL: Mode: %d (%s)\n", lcdMode, lcdMode == 0 ? "Normal" : "Relay");
    }
    else if (command == "lcd diag")
    {
        Serial.println("SERIAL: 📺 LCD diagnostics:");
        displayLcdDiagnostics();
    }
    else
    {
        Serial.println("SERIAL: ❌ Unknown command: " + command);
        Serial.println("SERIAL: Type 'help' for available commands");
    }
}