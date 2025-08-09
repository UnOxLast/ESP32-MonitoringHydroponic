// ==================== SENSOR VARIABLES ====================

// Temperature sensor objects
OneWire oneWire(oneWireBus);
DallasTemperature DS18B20(&oneWire);

// ==================== SENSOR IMPLEMENTATIONS ====================

// Initialize temperature sensor DS18B20
void initTemperatureSensor()
{
    DS18B20.begin();

    Serial.println("SENSOR: Initializing DS18B20 temperature sensor...");

    // Check if sensor is connected
    int deviceCount = DS18B20.getDeviceCount();

    if (deviceCount > 0)
    {
        temperatureSensorReady = true;
        Serial.printf("SENSOR: ✅ DS18B20 found - %d device(s) detected\n", deviceCount);
        Serial.printf("SENSOR: Resolution: %d bits\n", DS18B20.getResolution());
    }
    else
    {
        temperatureSensorReady = false;
        Serial.println("SENSOR: ❌ DS18B20 not found - Check wiring!");
    }
}

// Read temperature from DS18B20
float readTemperature()
{
    if (!temperatureSensorReady)
    {
        // Serial.println("SENSOR: ❌ Temperature sensor not ready");
        return -999.0; // Error value
    }

    DS18B20.requestTemperatures();

    // Wait for conversion (750ms for 12-bit resolution)
    vTaskDelay(pdMS_TO_TICKS(750));

    float tempC = DS18B20.getTempCByIndex(0);

    // Check if reading is valid
    if (tempC == DEVICE_DISCONNECTED_C)
    {
        Serial.println("SENSOR: ❌ Temperature sensor disconnected");
        return -999.0; // Error value
    }

    // Validate temperature range (reasonable for water systems)
    if (tempC < -10.0 || tempC > 60.0)
    {
        Serial.printf("SENSOR: ⚠️ Temperature out of range: %.2f°C\n", tempC);
        return -999.0; // Error value
    }

    temperatureValue = tempC;
    // Serial.printf("SENSOR: 🌡️ Temperature: %.2f°C\n", tempC);

    return tempC;
}

// ==================== pH SENSOR IMPLEMENTATIONS ====================

// Initialize pH sensor
void initPhSensor()
{
    Serial.println("SENSOR: Initializing pH sensor...");

    // Set analog reference and resolution for ESP32
    analogReadResolution(12); // 12-bit resolution (0-4095)

    // Test initial reading
    int rawValue = analogRead(phSensorPin);

    if (rawValue > 0 && rawValue < 4095)
    {
        phSensorReady = true;
        Serial.printf("SENSOR: ✅ pH sensor initialized - Initial raw value: %d\n", rawValue);

        // Initial reading
        readPh();
    }
    else
    {
        phSensorReady = false;
        Serial.println("SENSOR: ❌ pH sensor initialization failed - Check wiring!");
    }
}

// Read pH sensor value
float readPh()
{
    if (!phSensorReady)
    {
        // Serial.println("SENSOR: ❌ pH sensor not ready");
        return -1.0;
    }

    // Take multiple readings for stability
    const int numReadings = 10;
    int totalRaw = 0;

    for (int i = 0; i < numReadings; i++)
    {
        totalRaw += analogRead(phSensorPin);
        delay(10); // Small delay between readings
    }

    // Calculate average
    int averageRaw = totalRaw / numReadings;

    // Convert to voltage (ESP32 ADC: 0-4095 = 0-3.3V)
    phVoltage = (averageRaw * 3.3) / 4095.0;

    // Convert voltage to pH value
    // Standard pH probe: pH 7.0 = 2.5V (middle point)
    // pH decreases ~0.18V per pH unit (acidic side)
    // pH increases ~0.18V per pH unit (basic side)
    // Formula: pH = 7.0 - ((voltage - 2.5) / 0.18)
    float calculatedPh = 7.0 - ((phVoltage - 2.5) / 0.18);

    // Validate pH range (0-14)
    if (calculatedPh < 0.0)
    {
        calculatedPh = 0.0;
    }
    else if (calculatedPh > 14.0)
    {
        calculatedPh = 14.0;
    }

    phValue = calculatedPh;

    // Serial.printf("SENSOR: 🧪 pH - Raw: %d, Voltage: %.3fV, pH: %.2f\n", averageRaw, phVoltage, phValue);

    return phValue;
}

// Check if pH sensor is ready
bool isPhSensorReady()
{
    return phSensorReady;
}

// ==================== TDS SENSOR IMPLEMENTATIONS ====================

// Initialize TDS sensor
void initTdsSensor()
{
    Serial.println("SENSOR: Initializing TDS sensor...");

    // Set analog resolution for ESP32 (already set by pH sensor, but ensure it's set)
    analogReadResolution(12); // 12-bit resolution (0-4095)

    // Test initial reading
    int rawValue = analogRead(tdsSensorPin);

    if (rawValue > 0 && rawValue < 4095)
    {
        tdsSensorReady = true;
        Serial.printf("SENSOR: ✅ TDS sensor initialized - Initial raw value: %d\n", rawValue);

        // Initial reading
        readTds();
    }
    else
    {
        tdsSensorReady = false;
        Serial.println("SENSOR: ❌ TDS sensor initialization failed - Check wiring!");
    }
}

// Read TDS sensor value
float readTds()
{
    if (!tdsSensorReady)
    {
        // Serial.println("SENSOR: ❌ TDS sensor not ready");
        return -1.0;
    }

    // Take multiple readings for stability
    const int numReadings = 10;
    int totalRaw = 0;

    for (int i = 0; i < numReadings; i++)
    {
        totalRaw += analogRead(tdsSensorPin);
        delay(10); // Small delay between readings
    }

    // Calculate average
    int averageRaw = totalRaw / numReadings;

    // Convert to voltage (ESP32 ADC: 0-4095 = 0-3.3V)
    tdsVoltage = (averageRaw * 3.3) / 4095.0;

    // Convert voltage to TDS value using TDS Gravity V1 formula
    // Formula for TDS Gravity V1: TDS = (133.42 * V^3 - 255.86 * V^2 + 857.39 * V) * 0.5
    // This formula is calibrated for the specific sensor characteristics
    float voltage = tdsVoltage;
    float tdsRaw = (133.42 * voltage * voltage * voltage - 255.86 * voltage * voltage + 857.39 * voltage) * 0.5;

    // Temperature compensation (optional, using current temperature)
    // TDS increases ~2% per degree Celsius above 25°C
    float compensatedTds = tdsRaw;
    if (temperatureSensorReady && temperatureValue > 0)
    {
        float tempCoeff = 1.0 + 0.02 * (temperatureValue - 25.0);
        compensatedTds = tdsRaw / tempCoeff;
    }

    // Validate TDS range (0-2000 ppm for typical applications)
    if (compensatedTds < 0.0)
    {
        compensatedTds = 0.0;
    }
    else if (compensatedTds > 2000.0)
    {
        compensatedTds = 2000.0;
    }

    tdsValue = compensatedTds;

    // Serial.printf("SENSOR: 💧 TDS - Raw: %d, Voltage: %.3fV, TDS: %.1f ppm (Temp: %.1f°C)\n", averageRaw, tdsVoltage, tdsValue, temperatureValue);

    return tdsValue;
}

// Check if TDS sensor is ready
bool isTdsSensorReady()
{
    return tdsSensorReady;
}

// ==================== WATER FLOW SENSOR IMPLEMENTATIONS ====================

// Interrupt service routines for flow sensors
void IRAM_ATTR flowSensorA_ISR()
{
    flowCountA++;
}

void IRAM_ATTR flowSensorB_ISR()
{
    flowCountB++;
}

void IRAM_ATTR flowSensorPH_UP_ISR()
{
    flowCountPH_UP++;
}

void IRAM_ATTR flowSensorPH_DOWN_ISR()
{
    flowCountPH_DOWN++;
}

// Initialize water flow sensors
void initFlowSensors()
{
    Serial.println("SENSOR: Initializing water flow sensors...");

    // Configure pins as input with pull-up
    pinMode(flowSensorA_Pin, INPUT_PULLUP);
    pinMode(flowSensorB_Pin, INPUT_PULLUP);
    pinMode(flowSensorPH_UP_Pin, INPUT_PULLUP);
    pinMode(flowSensorPH_DOWN_Pin, INPUT_PULLUP);

    // Reset counters
    flowCountA = 0;
    flowCountB = 0;
    flowCountPH_UP = 0;
    flowCountPH_DOWN = 0;

    // Reset flow rates and volumes
    flowRateA = 0.0;
    flowRateB = 0.0;
    flowRatePH_UP = 0.0;
    flowRatePH_DOWN = 0.0;

    totalVolumeA = 0.0;
    totalVolumeB = 0.0;
    totalVolumePH_UP = 0.0;
    totalVolumePH_DOWN = 0.0;

    // Attach interrupts (falling edge detection)
    attachInterrupt(digitalPinToInterrupt(flowSensorA_Pin), flowSensorA_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorB_Pin), flowSensorB_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPH_UP_Pin), flowSensorPH_UP_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPH_DOWN_Pin), flowSensorPH_DOWN_ISR, FALLING);

    flowSensorsReady = true;

    Serial.println("SENSOR: ✅ Water flow sensors initialized");
    Serial.println("SENSOR: Flow A (GPIO 4), Flow B (GPIO 18)");
    Serial.println("SENSOR: Flow pH UP (GPIO 2), Flow pH DOWN (GPIO 19)");
}

// Update flow rates and calculate volumes
void updateFlowRates()
{
    if (!flowSensorsReady)
    {
        Serial.println("SENSOR: ❌ Flow sensors not ready");
        return;
    }

    // Static variables to track time and previous counts
    static unsigned long lastUpdateTime = 0;
    static unsigned long prevCountA = 0;
    static unsigned long prevCountB = 0;
    static unsigned long prevCountPH_UP = 0;
    static unsigned long prevCountPH_DOWN = 0;

    unsigned long currentTime = millis();

    // Calculate time difference (minimum 1 second for stable readings)
    if (currentTime - lastUpdateTime >= 1000)
    {
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0; // Convert to seconds

        // Calculate pulse differences
        unsigned long deltaCountA = flowCountA - prevCountA;
        unsigned long deltaCountB = flowCountB - prevCountB;
        unsigned long deltaCountPH_UP = flowCountPH_UP - prevCountPH_UP;
        unsigned long deltaCountPH_DOWN = flowCountPH_DOWN - prevCountPH_DOWN;

        // Sanity check: maximum reasonable pulse count per second (prevent sensor noise/overflow)
        // Max realistic flow ~10 L/min = ~980 pulses/sec = ~1000 pulses/sec safety margin
        const unsigned long MAX_PULSES_PER_SECOND = 1000;
        const unsigned long MAX_DELTA_PULSES = MAX_PULSES_PER_SECOND * (unsigned long)deltaTime;

        // Filter out unreasonable pulse counts (sensor malfunction/noise)
        if (deltaCountA > MAX_DELTA_PULSES)
        {
            Serial.printf("SENSOR: ⚠️  Abnormal pulse count A: %lu (max: %lu) - filtering out\n",
                          deltaCountA, MAX_DELTA_PULSES);
            deltaCountA = 0;         // Ignore this reading
            flowCountA = prevCountA; // Reset counter to prevent accumulation
        }

        if (deltaCountB > MAX_DELTA_PULSES)
        {
            Serial.printf("SENSOR: ⚠️  Abnormal pulse count B: %lu (max: %lu) - filtering out\n",
                          deltaCountB, MAX_DELTA_PULSES);
            deltaCountB = 0;
            flowCountB = prevCountB;
        }

        if (deltaCountPH_UP > MAX_DELTA_PULSES / 10) // pH solutions: much smaller expected flow
        {
            Serial.printf("SENSOR: ⚠️  Abnormal pulse count pH UP: %lu - filtering out\n", deltaCountPH_UP);
            deltaCountPH_UP = 0;
            flowCountPH_UP = prevCountPH_UP;
        }

        if (deltaCountPH_DOWN > MAX_DELTA_PULSES / 10) // pH solutions: much smaller expected flow
        {
            Serial.printf("SENSOR: ⚠️  Abnormal pulse count pH DOWN: %lu - filtering out\n", deltaCountPH_DOWN);
            deltaCountPH_DOWN = 0;
            flowCountPH_DOWN = prevCountPH_DOWN;
        }

        // Convert pulses to flow rate (L/min)
        // YF-S401 flow sensor: ~5880 pulses per liter (Hall effect sensor)
        // Formula: Flow Rate (L/min) = (pulses / time_in_minutes) / pulses_per_liter
        const float pulsesPerLiter = 5880.0; // YF-S401 specification
        float timeInMinutes = deltaTime / 60.0;

        if (timeInMinutes > 0)
        {
            flowRateA = (deltaCountA / timeInMinutes) / pulsesPerLiter;
            flowRateB = (deltaCountB / timeInMinutes) / pulsesPerLiter;
            flowRatePH_UP = (deltaCountPH_UP / timeInMinutes) / pulsesPerLiter;
            flowRatePH_DOWN = (deltaCountPH_DOWN / timeInMinutes) / pulsesPerLiter;

            // Calculate total volumes (cumulative) - only if values are reasonable
            float deltaVolumeA = deltaCountA / pulsesPerLiter;
            float deltaVolumeB = deltaCountB / pulsesPerLiter;
            float deltaVolumePH_UP = deltaCountPH_UP / pulsesPerLiter;
            float deltaVolumePH_DOWN = deltaCountPH_DOWN / pulsesPerLiter;

            // Additional volume sanity check
            const float MAX_DELTA_VOLUME = 1.0; // Max 1L per reading cycle
            if (deltaVolumeA <= MAX_DELTA_VOLUME)
                totalVolumeA += deltaVolumeA;
            if (deltaVolumeB <= MAX_DELTA_VOLUME)
                totalVolumeB += deltaVolumeB;
            if (deltaVolumePH_UP <= 0.1)
                totalVolumePH_UP += deltaVolumePH_UP; // pH: max 100mL
            if (deltaVolumePH_DOWN <= 0.1)
                totalVolumePH_DOWN += deltaVolumePH_DOWN; // pH: max 100mL
        }

        // Debug output (only show active flows to reduce spam)
        bool hasActiveFlow = false;
        if (flowRateA > 0.01 || flowRateB > 0.01 || flowRatePH_UP > 0.01 || flowRatePH_DOWN > 0.01)
        {
            hasActiveFlow = true;
            Serial.println("SENSOR: 💧 Flow Rates:");
            if (flowRateA > 0.01)
                Serial.printf("SENSOR:   Nutrisi A: %.3f L/min (Total: %.3f L)\n", flowRateA, totalVolumeA);
            if (flowRateB > 0.01)
                Serial.printf("SENSOR:   Nutrisi B: %.3f L/min (Total: %.3f L)\n", flowRateB, totalVolumeB);
            if (flowRatePH_UP > 0.01)
                Serial.printf("SENSOR:   pH UP: %.3f L/min (Total: %.3f L)\n", flowRatePH_UP, totalVolumePH_UP);
            if (flowRatePH_DOWN > 0.01)
                Serial.printf("SENSOR:   pH DOWN: %.3f L/min (Total: %.3f L)\n", flowRatePH_DOWN, totalVolumePH_DOWN);
        }

        // Update previous values
        prevCountA = flowCountA;
        prevCountB = flowCountB;
        prevCountPH_UP = flowCountPH_UP;
        prevCountPH_DOWN = flowCountPH_DOWN;
        lastUpdateTime = currentTime;
    }
}

// Reset flow counters and volumes
void resetFlowCounters()
{
    if (!flowSensorsReady)
    {
        Serial.println("SENSOR: ❌ Flow sensors not ready");
        return;
    }

    // Disable interrupts temporarily
    detachInterrupt(digitalPinToInterrupt(flowSensorA_Pin));
    detachInterrupt(digitalPinToInterrupt(flowSensorB_Pin));
    detachInterrupt(digitalPinToInterrupt(flowSensorPH_UP_Pin));
    detachInterrupt(digitalPinToInterrupt(flowSensorPH_DOWN_Pin));

    // Reset all counters and volumes
    flowCountA = 0;
    flowCountB = 0;
    flowCountPH_UP = 0;
    flowCountPH_DOWN = 0;

    flowRateA = 0.0;
    flowRateB = 0.0;
    flowRatePH_UP = 0.0;
    flowRatePH_DOWN = 0.0;

    totalVolumeA = 0.0;
    totalVolumeB = 0.0;
    totalVolumePH_UP = 0.0;
    totalVolumePH_DOWN = 0.0;

    // Re-attach interrupts
    attachInterrupt(digitalPinToInterrupt(flowSensorA_Pin), flowSensorA_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorB_Pin), flowSensorB_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPH_UP_Pin), flowSensorPH_UP_ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(flowSensorPH_DOWN_Pin), flowSensorPH_DOWN_ISR, FALLING);

    Serial.println("SENSOR: ✅ Flow counters and volumes reset");
}

// Check if flow sensors are ready
bool areFlowSensorsReady()
{
    return flowSensorsReady;
}

// Get flow sensor readings in mL with overflow protection
float getFlowSensorA_mL()
{
    // Sanity check: maximum reasonable value (10 liters = 10,000 mL)
    const float MAX_REASONABLE_ML = 10000.0;
    float result = totalVolumeA * 1000.0; // Convert L to mL

    if (result > MAX_REASONABLE_ML || result < 0 || isnan(result) || isinf(result))
    {
        Serial.printf("SENSOR: ⚠️  Flow A overflow detected: %.0f mL - resetting to 0\n", result);
        totalVolumeA = 0.0; // Reset to prevent future issues
        return 0.0;
    }

    return result;
}

float getFlowSensorB_mL()
{
    // Sanity check: maximum reasonable value (10 liters = 10,000 mL)
    const float MAX_REASONABLE_ML = 10000.0;
    float result = totalVolumeB * 1000.0; // Convert L to mL

    if (result > MAX_REASONABLE_ML || result < 0 || isnan(result) || isinf(result))
    {
        Serial.printf("SENSOR: ⚠️  Flow B overflow detected: %.0f mL - resetting to 0\n", result);
        totalVolumeB = 0.0; // Reset to prevent future issues
        return 0.0;
    }

    return result;
}

float getFlowSensorPH_UP_mL()
{
    // Sanity check: maximum reasonable value (1 liter = 1,000 mL for pH solutions)
    const float MAX_REASONABLE_ML = 1000.0;
    float result = totalVolumePH_UP * 1000.0; // Convert L to mL

    if (result > MAX_REASONABLE_ML || result < 0 || isnan(result) || isinf(result))
    {
        Serial.printf("SENSOR: ⚠️  Flow pH UP overflow detected: %.0f mL - resetting to 0\n", result);
        totalVolumePH_UP = 0.0; // Reset to prevent future issues
        return 0.0;
    }

    return result;
}

float getFlowSensorPH_DOWN_mL()
{
    // Sanity check: maximum reasonable value (1 liter = 1,000 mL for pH solutions)
    const float MAX_REASONABLE_ML = 1000.0;
    float result = totalVolumePH_DOWN * 1000.0; // Convert L to mL

    if (result > MAX_REASONABLE_ML || result < 0 || isnan(result) || isinf(result))
    {
        Serial.printf("SENSOR: ⚠️  Flow pH DOWN overflow detected: %.0f mL - resetting to 0\n", result);
        totalVolumePH_DOWN = 0.0; // Reset to prevent future issues
        return 0.0;
    }

    return result;
}

// Simulate flow sensor for testing (temporary function)
void simulateFlowSensor(char sensor, float mL)
{
    float liters = mL / 1000.0;

    if (sensor == 'A' || sensor == 'a')
    {
        totalVolumeA += liters;
        Serial.printf("SENSOR: 🧪 Simulated flow A: +%.1f mL (Total: %.1f mL)\n", mL, totalVolumeA * 1000.0);
    }
    else if (sensor == 'B' || sensor == 'b')
    {
        totalVolumeB += liters;
        Serial.printf("SENSOR: 🧪 Simulated flow B: +%.1f mL (Total: %.1f mL)\n", mL, totalVolumeB * 1000.0);
    }
}

// Reset flow sensors for waterflow control
void resetFlowSensors()
{
    resetFlowCounters();
}

// Emergency flow sensor reset - use when sensors malfunction or overflow
void emergencyFlowReset()
{
    Serial.println("SENSOR: 🚨 Emergency flow sensor reset initiated");

    // Reset all pulse counters
    flowCountA = 0;
    flowCountB = 0;
    flowCountPH_UP = 0;
    flowCountPH_DOWN = 0;

    // Reset all accumulated volumes
    totalVolumeA = 0.0;
    totalVolumeB = 0.0;
    totalVolumePH_UP = 0.0;
    totalVolumePH_DOWN = 0.0;

    // Reset all flow rates
    flowRateA = 0.0;
    flowRateB = 0.0;
    flowRatePH_UP = 0.0;
    flowRatePH_DOWN = 0.0;

    Serial.println("SENSOR: ✅ All flow sensors reset to zero");
    Serial.println("SENSOR: 💡 Use this command if flow values become unreasonable due to sensor malfunction");
}

// Check if any flow sensor has unreasonable values
bool checkFlowSensorHealth()
{
    const float MAX_TOTAL_VOLUME = 10.0; // 10 liters maximum
    const float MAX_PH_VOLUME = 1.0;     // 1 liter maximum for pH

    bool healthy = true;

    if (totalVolumeA > MAX_TOTAL_VOLUME || totalVolumeB > MAX_TOTAL_VOLUME)
    {
        Serial.printf("SENSOR: ⚠️  Nutrient volume overflow - A:%.1fL B:%.1fL (max: %.1fL)\n",
                      totalVolumeA, totalVolumeB, MAX_TOTAL_VOLUME);
        healthy = false;
    }

    if (totalVolumePH_UP > MAX_PH_VOLUME || totalVolumePH_DOWN > MAX_PH_VOLUME)
    {
        Serial.printf("SENSOR: ⚠️  pH volume overflow - UP:%.3fL DOWN:%.3fL (max: %.1fL)\n",
                      totalVolumePH_UP, totalVolumePH_DOWN, MAX_PH_VOLUME);
        healthy = false;
    }

    if (!healthy)
    {
        Serial.println("SENSOR: 🚨 Flow sensor health check FAILED - consider emergency reset");
    }

    return healthy;
}
