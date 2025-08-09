// ==================== RELAY/ACTUATOR IMPLEMENTATIONS ====================

// Initialize all relays
void initRelays()
{
    Serial.println("RELAY : Initializing relay system...");

    // Configure relay pins as outputs
    pinMode(relayPumpA_Pin, OUTPUT);
    pinMode(relayPumpB_Pin, OUTPUT);
    pinMode(relayPumpPH_UP_Pin, OUTPUT);
    pinMode(relayPumpPH_DOWN_Pin, OUTPUT);
    pinMode(relayMixerPump_Pin, OUTPUT);

    // Set all relays to OFF state (LOW = OFF, HIGH = ON)
    digitalWrite(relayPumpA_Pin, LOW);
    digitalWrite(relayPumpB_Pin, LOW);
    digitalWrite(relayPumpPH_UP_Pin, LOW);
    digitalWrite(relayPumpPH_DOWN_Pin, LOW);
    digitalWrite(relayMixerPump_Pin, LOW);

    // Reset state variables
    relayPumpA_State = false;
    relayPumpB_State = false;
    relayPumpPH_UP_State = false;
    relayPumpPH_DOWN_State = false;
    relayMixerPump_State = false;

    relaysReady = true;

    Serial.println("RELAY : ✅ Relay system initialized");
    Serial.println("RELAY : Pump A (GPIO 12), Pump B (GPIO 13)");
    Serial.println("RELAY : pH UP (GPIO 14), pH DOWN (GPIO 27)");
    Serial.println("RELAY : Mixer (GPIO 26)");
    Serial.println("RELAY : All relays set to OFF state");
}

// Generic relay control function
void setRelay(int relayNumber, bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    switch (relayNumber)
    {
    case 1:
        setRelayPumpA(state);
        break;
    case 2:
        setRelayPumpB(state);
        break;
    case 3:
        setRelayPumpPH_UP(state); 
        break;
    case 4:
        setRelayPumpPH_DOWN(state); //aerator di firebase
        break;
    case 5:
        setRelayMixerPump(state); 
        break;
    default:
        Serial.printf("RELAY : ❌ Invalid relay number: %d (valid: 1-5)\n", relayNumber);
        break;
    }
}

// Individual relay control functions
void setRelayPumpA(bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    relayPumpA_State = state;
    digitalWrite(relayPumpA_Pin, state ? HIGH : LOW); // HIGH = ON, LOW = OFF
    Serial.printf("RELAY : 🔄 Pump A %s\n", state ? "ON" : "OFF");
}

void setRelayPumpB(bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    relayPumpB_State = state;
    digitalWrite(relayPumpB_Pin, state ? HIGH : LOW);
    Serial.printf("RELAY : 🔄 Pump B %s\n", state ? "ON" : "OFF");
}

void setRelayPumpPH_UP(bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    relayPumpPH_UP_State = state;
    digitalWrite(relayPumpPH_UP_Pin, state ? HIGH : LOW);
    Serial.printf("RELAY : 🔄 pH UP Pump %s\n", state ? "ON" : "OFF");
}

void setRelayPumpPH_DOWN(bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    relayPumpPH_DOWN_State = state;
    digitalWrite(relayPumpPH_DOWN_Pin, state ? HIGH : LOW);
    Serial.printf("RELAY : 🔄 pH DOWN Pump %s\n", state ? "ON" : "OFF");
}

void setRelayMixerPump(bool state)
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    relayMixerPump_State = state;
    digitalWrite(relayMixerPump_Pin, state ? HIGH : LOW);
    Serial.printf("RELAY : 🔄 Mixer Pump %s\n", state ? "ON" : "OFF");
}

// Turn off all relays (emergency stop)
void turnOffAllRelays()
{
    if (!relaysReady)
    {
        Serial.println("RELAY : ❌ Relay system not ready");
        return;
    }

    Serial.println("RELAY : 🛑 Emergency stop - Turning OFF all relays");

    setRelayPumpA(false);
    setRelayPumpB(false);
    setRelayPumpPH_UP(false);
    setRelayPumpPH_DOWN(false);
    setRelayMixerPump(false);

    Serial.println("RELAY : ✅ All relays turned OFF");
}

// Check if relay system is ready
bool areRelaysReady()
{
    return relaysReady;
}
