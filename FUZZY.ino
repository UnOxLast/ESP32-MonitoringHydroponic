// ==================== FUZZY LOGIC CONTROLLER FOR TDS ====================

// Fuzzy variables
float currentPPM = 0;
float setPPM = 1000; // Default setpoint 1000 ppm
float errPPM = 0;
float deltaErrPPM = 0;
float prevPpmError = 0;

// Waterflow control variables
float maxMLPerCycle = 100.0; // Maksimal mL per siklus fuzzy
float currentTargetML_A = 0; // Target mL nutrisi A dari fuzzy output
float currentTargetML_B = 0; // Target mL nutrisi B dari fuzzy output
float actualML_A = 0;        // Aktual mL dari flow sensor A
float actualML_B = 0;        // Aktual mL dari flow sensor B
int fuzzyIterationCount = 0; // Counter iterasi fuzzy
int maxFuzzyIterations = 5;  // Maksimal iterasi fuzzy (mutable)
float tdsTolerance = 10.0;   // Tolerance TDS ±10 ppm

// Rentang untuk fungsi keanggotaan Error TDS (TDS_Error)
float errTDS_SN[] = {-1500, -1000, -800, -500};
float errTDS_N[] = {-1000, -500, -100};
float errTDS_Z[] = {-50, 0, 50};
float errTDS_P[] = {100, 500, 1000};
float errTDS_SP[] = {500, 1000, 1500, 1500};

// Rentang untuk fungsi keanggotaan Delta Error TDS (Delta_TDS_Error)
float deltaErrTDS_EN[] = {-1500, -1500, -600, 0};
float deltaErrTDS_EZ[] = {-1000, 0, 1000};
float deltaErrTDS_EP[] = {0, 600, 1500, 1500};

// Rentang untuk fungsi keanggotaan Output
const float OUTPUT_MIN = 0;
const float OUTPUT_MAX = 100;

// Struktur untuk fungsi keanggotaan output
struct OutputMF
{
    float a, b, c, d;
};

// Definisi fungsi keanggotaan output
const OutputMF OUTPUT_OFF = {0, 0, 10, 20};
const OutputMF OUTPUT_SE = {10, 20, 30, 40};
const OutputMF OUTPUT_M = {30, 40, 60, 70};
const OutputMF OUTPUT_B = {60, 70, 80, 90};
const OutputMF OUTPUT_SB = {80, 90, 100, 100};

// --- Fungsi Keanggotaan menggunakan Rentang Trapezoid ---
float trapezMF(float x, float leftShoulder, float leftTop, float rightTop, float rightShoulder)
{
    if (x <= leftShoulder || x >= rightShoulder)
        return 0;
    else if (x >= leftTop && x <= rightTop)
        return 1;
    else if (x > leftShoulder && x < leftTop)
        return (x - leftShoulder) / (leftTop - leftShoulder);
    else if (x > rightTop && x < rightShoulder)
        return (rightShoulder - x) / (rightShoulder - rightTop);
    return 0;
}

// Fungsi Keanggotaan untuk Error TDS
float mfErrTDS_SN(float error) { return trapezMF(error, errTDS_SN[0], errTDS_SN[1], errTDS_SN[2], errTDS_SN[3]); }
float mfErrTDS_N(float error) { return trapezMF(error, errTDS_N[0], errTDS_N[1], errTDS_N[2], errTDS_N[2]); }
float mfErrTDS_Z(float error) { return trapezMF(error, errTDS_Z[0], errTDS_Z[1], errTDS_Z[2], errTDS_Z[2]); }
float mfErrTDS_P(float error) { return trapezMF(error, errTDS_P[0], errTDS_P[1], errTDS_P[2], errTDS_P[2]); }
float mfErrTDS_SP(float error) { return trapezMF(error, errTDS_SP[0], errTDS_SP[1], errTDS_SP[2], errTDS_SP[3]); }

// Fungsi Keanggotaan untuk Delta Error TDS
float mfDeltaErrTDS_EN(float deltaError) { return trapezMF(deltaError, deltaErrTDS_EN[0], deltaErrTDS_EN[1], deltaErrTDS_EN[2], deltaErrTDS_EN[3]); }
float mfDeltaErrTDS_EZ(float deltaError) { return trapezMF(deltaError, deltaErrTDS_EZ[0], deltaErrTDS_EZ[1], deltaErrTDS_EZ[2], deltaErrTDS_EZ[2]); }
float mfDeltaErrTDS_EP(float deltaError) { return trapezMF(deltaError, deltaErrTDS_EP[0], deltaErrTDS_EP[1], deltaErrTDS_EP[2], deltaErrTDS_EP[3]); }

// Fungsi Keanggotaan untuk Output
float mfOutput_OFF(float x) { return trapezMF(x, OUTPUT_OFF.a, OUTPUT_OFF.b, OUTPUT_OFF.c, OUTPUT_OFF.d); }
float mfOutput_SE(float x) { return trapezMF(x, OUTPUT_SE.a, OUTPUT_SE.b, OUTPUT_SE.c, OUTPUT_SE.d); }
float mfOutput_M(float x) { return trapezMF(x, OUTPUT_M.a, OUTPUT_M.b, OUTPUT_M.c, OUTPUT_M.d); }
float mfOutput_B(float x) { return trapezMF(x, OUTPUT_B.a, OUTPUT_B.b, OUTPUT_B.c, OUTPUT_B.d); }
float mfOutput_SB(float x) { return trapezMF(x, OUTPUT_SB.a, OUTPUT_SB.b, OUTPUT_SB.c, OUTPUT_SB.d); }

// Struktur untuk menyimpan hasil rule
struct RuleOutput
{
    float strength;
    const OutputMF *outputMF;
};

// --- Proses Fuzzifikasi dan Inferensi Fuzzy ---
float inferenceTDS(float errPPM, float deltaErrPPM)
{
    RuleOutput ruleOutputs[15];
    ruleOutputs[0] = {mfErrTDS_SP(errPPM) * mfDeltaErrTDS_EN(deltaErrPPM), &OUTPUT_SB};
    ruleOutputs[1] = {mfErrTDS_SP(errPPM) * mfDeltaErrTDS_EZ(deltaErrPPM), &OUTPUT_SB};
    ruleOutputs[2] = {mfErrTDS_SP(errPPM) * mfDeltaErrTDS_EP(deltaErrPPM), &OUTPUT_B};
    ruleOutputs[3] = {mfErrTDS_P(errPPM) * mfDeltaErrTDS_EN(deltaErrPPM), &OUTPUT_M};
    ruleOutputs[4] = {mfErrTDS_P(errPPM) * mfDeltaErrTDS_EZ(deltaErrPPM), &OUTPUT_M};
    ruleOutputs[5] = {mfErrTDS_P(errPPM) * mfDeltaErrTDS_EP(deltaErrPPM), &OUTPUT_SE};
    ruleOutputs[6] = {mfErrTDS_Z(errPPM) * mfDeltaErrTDS_EN(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[7] = {mfErrTDS_Z(errPPM) * mfDeltaErrTDS_EZ(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[8] = {mfErrTDS_Z(errPPM) * mfDeltaErrTDS_EP(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[9] = {mfErrTDS_N(errPPM) * mfDeltaErrTDS_EN(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[10] = {mfErrTDS_N(errPPM) * mfDeltaErrTDS_EZ(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[11] = {mfErrTDS_N(errPPM) * mfDeltaErrTDS_EP(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[12] = {mfErrTDS_SN(errPPM) * mfDeltaErrTDS_EN(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[13] = {mfErrTDS_SN(errPPM) * mfDeltaErrTDS_EZ(deltaErrPPM), &OUTPUT_OFF};
    ruleOutputs[14] = {mfErrTDS_SN(errPPM) * mfDeltaErrTDS_EP(deltaErrPPM), &OUTPUT_OFF};

    float dx = 0.1;
    float centroidNumerator = 0;
    float centroidDenominator = 0;

    for (float x = OUTPUT_MIN; x <= OUTPUT_MAX; x += dx)
    {
        float maxMembership = 0;
        for (int i = 0; i < 15; i++)
        {
            float ruleMembership = 0;
            if (ruleOutputs[i].outputMF == &OUTPUT_OFF)
            {
                ruleMembership = min(ruleOutputs[i].strength, mfOutput_OFF(x));
            }
            else if (ruleOutputs[i].outputMF == &OUTPUT_SE)
            {
                ruleMembership = min(ruleOutputs[i].strength, mfOutput_SE(x));
            }
            else if (ruleOutputs[i].outputMF == &OUTPUT_M)
            {
                ruleMembership = min(ruleOutputs[i].strength, mfOutput_M(x));
            }
            else if (ruleOutputs[i].outputMF == &OUTPUT_B)
            {
                ruleMembership = min(ruleOutputs[i].strength, mfOutput_B(x));
            }
            else if (ruleOutputs[i].outputMF == &OUTPUT_SB)
            {
                ruleMembership = min(ruleOutputs[i].strength, mfOutput_SB(x));
            }
            maxMembership = max(maxMembership, ruleMembership);
        }
        centroidNumerator += x * maxMembership * dx;
        centroidDenominator += maxMembership * dx;
    }

    // Calculate result with protection
    float result = (centroidDenominator == 0) ? 0 : centroidNumerator / centroidDenominator;

    // Clamp result to valid range (0-100%)
    if (result < 0)
    {
        result = 0;
    }
    else if (result > 100)
    {
        result = 100;
    }

    return result;
}

// Fungsi untuk Menentukan Kategori Output
String getOutputCategory(float output)
{
    if (output >= OUTPUT_SB.a)
        return "Sangat Banyak (SB)";
    else if (output >= OUTPUT_B.a)
        return "Banyak (B)";
    else if (output >= OUTPUT_M.a)
        return "Sedang (M)";
    else if (output >= OUTPUT_SE.a)
        return "Sedikit (SE)";
    else
        return "Mati (OFF)";
}

// ==================== WATERFLOW CONTROL FUNCTIONS ====================

// Konversi fuzzy output (0-100%) ke target mL nutrisi
void calculateNutrientML(float fuzzyOutput)
{
    // Konversi persentase ke mL
    currentTargetML_A = (fuzzyOutput / 100.0) * maxMLPerCycle;
    currentTargetML_B = (fuzzyOutput / 100.0) * maxMLPerCycle;

    Serial.printf("FUZZY : Target nutrisi A: %.1f mL\n", currentTargetML_A);
    Serial.printf("FUZZY : Target nutrisi B: %.1f mL\n", currentTargetML_B);
    Serial.printf("FUZZY : Fuzzy output: %.1f%% → %.1f mL each\n",
                  fuzzyOutput, currentTargetML_A);
}

// Get current target mL untuk nutrisi A
float getTargetML_A()
{
    return currentTargetML_A;
}

// Get current target mL untuk nutrisi B
float getTargetML_B()
{
    return currentTargetML_B;
}

// Update actual mL dari flow sensors
void updateActualML(float mlA, float mlB)
{
    actualML_A = mlA;
    actualML_B = mlB;
    Serial.printf("FUZZY : Actual flow - A: %.1f mL, B: %.1f mL\n", mlA, mlB);
}

// Cek apakah target mL sudah tercapai
bool isTargetMLReached()
{
    float toleranceML = 2.0; // Tolerance ±2 mL
    bool targetA_reached = abs(actualML_A - currentTargetML_A) <= toleranceML;
    bool targetB_reached = abs(actualML_B - currentTargetML_B) <= toleranceML;

    return (targetA_reached && targetB_reached);
}

// Cek apakah TDS sudah dalam target
bool isTDSTargetReached(float currentTDS, float targetTDS)
{
    return abs(currentTDS - targetTDS) <= tdsTolerance;
}

// Initialize fuzzy logic system
void initFuzzyLogic()
{
    Serial.println("FUZZY : Initializing fuzzy logic controller...");

    // Reset fuzzy variables
    currentPPM = 0;
    setPPM = 1000; // Default setpoint 1000 ppm
    errPPM = 0;
    deltaErrPPM = 0;
    prevPpmError = 0;

    // Reset waterflow variables
    currentTargetML_A = 0;
    currentTargetML_B = 0;
    actualML_A = 0;
    actualML_B = 0;
    fuzzyProcessActive = false;
    fuzzyIterationCount = 0;

    Serial.println("FUZZY : ✅ Fuzzy logic controller initialized");
    Serial.printf("FUZZY : Default setpoint: %.0f ppm\n", setPPM);
    Serial.printf("FUZZY : Max mL per cycle: %.0f mL\n", maxMLPerCycle);
    Serial.println("FUZZY : TDS control ready with 15 fuzzy rules");
}

// ==================== MAIN FUZZY TDS CONTROL FUNCTIONS ====================

// Start fuzzy TDS control process with immediate execution
bool startFuzzyTDSControl(float targetTDS, float currentTDS)
{
    if (fuzzyProcessActive)
    {
        Serial.println("FUZZY : ⚠️ Fuzzy process already active");
        return false;
    }

    Serial.printf("FUZZY : 🚀 Starting fuzzy TDS control - Target: %.0f ppm\n", targetTDS);

    fuzzyProcessActive = true;
    fuzzyIterationCount = 0;
    setPPM = targetTDS;

    // IMMEDIATE EXECUTION: Run first fuzzy iteration right away
    Serial.printf("FUZZY : 🔄 Iterasi %d/%d (immediate execution)\n", fuzzyIterationCount + 1, maxFuzzyIterations);

    // Check if already in target (unlikely but possible)
    if (isTDSTargetReached(currentTDS, setPPM))
    {
        Serial.printf("FUZZY : ✅ Target TDS already reached!\n");
        Serial.printf("FUZZY : Current: %.1f ppm, Target: %.0f ppm\n", currentTDS, setPPM);
        fuzzyProcessActive = false;
        return true; // Target already reached
    }

    // Execute fuzzy calculation immediately
    fuzzyIterationCount++; // Now becomes 1
    float fuzzyOutput = runFuzzy(currentTDS, setPPM);

    // Convert to target mL immediately
    calculateNutrientML(fuzzyOutput);

    Serial.printf("FUZZY : 🎯 Immediate fuzzy result: %.1f%% → A: %.1f mL, B: %.1f mL\n",
                  fuzzyOutput, currentTargetML_A, currentTargetML_B);

    return false; // Continue with pump execution
}

// Execute single fuzzy iteration
bool executeFuzzyIteration(float currentTDS)
{
    if (!fuzzyProcessActive)
    {
        Serial.println("FUZZY : ❌ Fuzzy process not active");
        return false;
    }

    fuzzyIterationCount++;
    Serial.printf("FUZZY : 🔄 Iterasi %d/%d\n", fuzzyIterationCount, maxFuzzyIterations);

    // 1. Cek apakah sudah dalam tolerance
    if (isTDSTargetReached(currentTDS, setPPM))
    {
        Serial.printf("FUZZY : ✅ Target TDS tercapai dalam %d iterasi!\n", fuzzyIterationCount);
        Serial.printf("FUZZY : Current: %.1f ppm, Target: %.0f ppm\n", currentTDS, setPPM);
        fuzzyProcessActive = false;
        return true; // Target tercapai
    }

    // 2. Cek maksimal iterasi
    if (fuzzyIterationCount >= maxFuzzyIterations)
    {
        Serial.printf("FUZZY : ⚠️ Maksimal iterasi (%d) tercapai\n", maxFuzzyIterations);
        Serial.printf("FUZZY : Current: %.1f ppm, Target: %.0f ppm\n", currentTDS, setPPM);
        fuzzyProcessActive = false;
        return true; // Stop karena max iterasi
    }

    // 3. Hitung fuzzy output
    float fuzzyOutput = runFuzzy(currentTDS, setPPM);

    // 4. Konversi ke target mL
    calculateNutrientML(fuzzyOutput);

    // 5. Return false untuk indikasi perlu eksekusi pompa
    return false;
}

// Stop fuzzy process
void stopFuzzyProcess()
{
    if (fuzzyProcessActive)
    {
        Serial.printf("FUZZY : ⏹️ Stopping fuzzy process at iteration %d\n", fuzzyIterationCount);
        fuzzyProcessActive = false;
    }
}

// Get fuzzy process status
bool isFuzzyProcessActive()
{
    return fuzzyProcessActive;
}

// Get current fuzzy iteration count
int getFuzzyIterationCount()
{
    return fuzzyIterationCount;
}

// Debug output untuk monitoring fuzzy logic
void writeOutput(float pumpPPMOutput)
{
    Serial.print("FUZZY : Output (0-100): ");
    Serial.println(pumpPPMOutput);
    Serial.print("FUZZY : Category: ");
    Serial.println(getOutputCategory(pumpPPMOutput));

    if (pumpPPMOutput > OUTPUT_OFF.d)
    {
        Serial.println("FUZZY : 💡 Pump should be ON");
    }
    else
    {
        Serial.println("FUZZY : 💡 Pump should be OFF");
    }
}

// Main fuzzy logic function
float runFuzzy(float ppm, int setppm)
{
    currentPPM = ppm;
    setPPM = setppm;
    errPPM = setPPM - currentPPM;
    deltaErrPPM = errPPM - prevPpmError;
    prevPpmError = errPPM;

    float pumpPPMOutput = inferenceTDS(errPPM, deltaErrPPM);
    return pumpPPMOutput;
}

// Enhanced fuzzy logic function with debugging
float runFuzzyWithDebug(float ppm, int setppm, bool enableDebug)
{
    currentPPM = ppm;
    setPPM = setppm;
    errPPM = setPPM - currentPPM;
    deltaErrPPM = errPPM - prevPpmError;
    prevPpmError = errPPM;

    float pumpPPMOutput = inferenceTDS(errPPM, deltaErrPPM);

    if (enableDebug)
    {
        Serial.printf("FUZZY : Current TDS: %.1f ppm, Setpoint: %d ppm\n", ppm, setppm);
        Serial.printf("FUZZY : Error: %.1f ppm, Delta Error: %.1f ppm\n", errPPM, deltaErrPPM);
        writeOutput(pumpPPMOutput);
    }

    return pumpPPMOutput;
}

// Get current fuzzy system status
void getFuzzyStatus()
{
    Serial.println("FUZZY : 📊 Current fuzzy system status:");
    Serial.printf("FUZZY :   Current TDS: %.1f ppm\n", currentPPM);
    Serial.printf("FUZZY :   Setpoint: %.0f ppm\n", setPPM);
    Serial.printf("FUZZY :   Error: %.1f ppm\n", errPPM);
    Serial.printf("FUZZY :   Delta Error: %.1f ppm\n", deltaErrPPM);
    Serial.printf("FUZZY :   Process Active: %s\n", fuzzyProcessActive ? "YES" : "NO");
    Serial.printf("FUZZY :   Iteration: %d/%d\n", fuzzyIterationCount, maxFuzzyIterations);

    float output = runFuzzy(currentPPM, setPPM);
    Serial.printf("FUZZY :   Current Output: %.1f%%\n", output);
    Serial.printf("FUZZY :   Category: %s\n", getOutputCategory(output).c_str());
    Serial.printf("FUZZY :   Target mL A: %.1f mL\n", currentTargetML_A);
    Serial.printf("FUZZY :   Target mL B: %.1f mL\n", currentTargetML_B);
    Serial.printf("FUZZY :   Actual mL A: %.1f mL\n", actualML_A);
    Serial.printf("FUZZY :   Actual mL B: %.1f mL\n", actualML_B);
}

// Enhanced status dengan waterflow info
void getFuzzyWaterflowStatus()
{
    Serial.println("FUZZY : 💧 Waterflow Status:");
    Serial.printf("FUZZY :   Max mL per cycle: %.0f mL\n", maxMLPerCycle);
    Serial.printf("FUZZY :   Target A: %.1f mL | Actual A: %.1f mL\n",
                  currentTargetML_A, actualML_A);
    Serial.printf("FUZZY :   Target B: %.1f mL | Actual B: %.1f mL\n",
                  currentTargetML_B, actualML_B);
    Serial.printf("FUZZY :   ML Target Reached: %s\n",
                  isTargetMLReached() ? "YES" : "NO");
    Serial.printf("FUZZY :   TDS Target Reached: %s\n",
                  isTDSTargetReached(currentPPM, setPPM) ? "YES" : "NO");
}

// Set new TDS setpoint
void setTdsSetpoint(int newSetpoint)
{
    if (newSetpoint >= 0 && newSetpoint <= 2000)
    {
        setPPM = newSetpoint;
        // Print moved to Firebase.ino for consistency
    }
    else
    {
        Serial.printf("FUZZY : ❌ Invalid setpoint: %d (valid range: 0-2000 ppm)\n", newSetpoint);
    }
}

// Get current TDS setpoint
int getTdsSetpoint()
{
    return (int)setPPM;
}

// ==================== FUZZY SYSTEM CONFIGURATION ====================

// Set maximum mL per cycle
void setMaxMLPerCycle(float maxML)
{
    if (maxML > 0 && maxML <= 200)
    {
        maxMLPerCycle = maxML;
        Serial.printf("FUZZY : Max mL per cycle set to: %.0f mL\n", maxMLPerCycle);
    }
    else
    {
        Serial.printf("FUZZY : ❌ Invalid max mL: %.0f (valid range: 1-200 mL)\n", maxML);
    }
}

// Get maximum mL per cycle
float getMaxMLPerCycle()
{
    return maxMLPerCycle;
}

// Set TDS tolerance
void setTdsTolerance(float tolerance)
{
    if (tolerance > 0 && tolerance <= 50)
    {
        tdsTolerance = tolerance;
        Serial.printf("FUZZY : TDS tolerance set to: ±%.1f ppm\n", tdsTolerance);
    }
    else
    {
        Serial.printf("FUZZY : ❌ Invalid tolerance: %.1f (valid range: 0.1-50 ppm)\n", tolerance);
    }
}

// Get TDS tolerance
float getTdsTolerance()
{
    return tdsTolerance;
}

// Set maximum fuzzy iterations
void setMaxFuzzyIterations(int maxIterations)
{
    if (maxIterations > 0 && maxIterations <= 10)
    {
        maxFuzzyIterations = maxIterations;
        Serial.printf("FUZZY : Max iterations set to: %d\n", maxFuzzyIterations);
    }
    else
    {
        Serial.printf("FUZZY : ❌ Invalid iterations: %d (valid range: 1-10)\n", maxIterations);
    }
}

// Get maximum fuzzy iterations
int getMaxFuzzyIterations()
{
    return maxFuzzyIterations;
}

// ==================== INTEGRATION FUNCTIONS ====================

// Initialize fuzzy system for integration with other modules
void initializeFuzzySystem()
{
    initFuzzyLogic();

    // Set default parameters for hydroponics
    setMaxMLPerCycle(100.0);  // 100 mL max per cycle
    setTdsTolerance(10.0);    // ±10 ppm tolerance
    setMaxFuzzyIterations(5); // Max 5 iterations

    Serial.println("FUZZY : 🌱 Fuzzy system ready for hydroponics integration");
}
