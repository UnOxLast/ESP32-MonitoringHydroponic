# ESP32 Hidroponik System

## Deskripsi Proyek
Sistem kontrol hidroponik otomatis menggunakan ESP32 dengan monitoring pH, TDS, temperature, dan kontrol nutrisi melalui fuzzy logic. Sistem terintegrasi dengan Firebase untuk monitoring dan kontrol jarak jauh.

## Pin Mapping ESP32

### 📊 Sensor Pins
| Sensor | Pin | Keterangan |
|--------|-----|------------|
| pH Sensor | GPIO 35 | Sensor pH air nutrisi |
| Flow Sensor A (Nutrisi A)       | GPIO 4  | Digital Input | Interrupt-based pulse counting |
| Flow Sensor B (Nutrisi B)       | GPIO 18 | Digital Input | Interrupt-based pulse counting |
| Flow Sensor pH UP               | GPIO 2  | Digital Input | Interrupt-based pulse counting |
| Flow Sensor pH DOWN             | GPIO 19 | Digital Input | Interrupt-based pulse counting |
| DS18B20 Temperature | GPIO 32 | Sensor suhu air (OneWire) |

### 💧 Water Flow Sensors
| Flow Sensor | Pin | Fungsi |
|-------------|-----|--------|
| Flow A | GPIO 4 | Nutrisi A flow meter |
| Flow B | GPIO 18 | Nutrisi B flow meter |
| Flow pH UP | GPIO 2 | pH UP solution flow meter |
| Flow pH DOWN | GPIO 19 | pH DOWN solution flow meter |

### 🔌 Relay Controls (5 Relay)
| Relay | Pin | Fungsi |
|-------|-----|--------|
| Relay 1 (Pump A) | GPIO 12 | Nutrisi A pump |
| Relay 2 (Pump B) | GPIO 13 | Nutrisi B pump |
| Relay 3 (pH UP) | GPIO 14 | pH UP pump |
| Relay 4 (pH DOWN) | GPIO 27 | pH DOWN pump |
| Relay 5 (Mixer) | GPIO 26 | Water mixer pump |

### 📺 Display & Communication
| Device | Interface | Keterangan |
|--------|-----------|------------|
| LCD 20x4 | I2C (0x27) | Display status sistem |
| WiFi | Built-in | Koneksi internet untuk Firebase |
| Serial | USB | Komunikasi debug dan konfigurasi |

## Hardware Specifications

### Microcontroller
- **Board**: ESP32 DevKit
- **Operating Voltage**: 3.3V
- **Input Voltage**: 5V (via USB) / 7-12V (via Vin)

### Sensors
- **pH Sensor**: Analog sensor range 0-14 pH
- **TDS Sensor**: Gravity TDS sensor  
- **Temperature**: DS18B20 waterproof sensor
- **Flow Meters**: 4x Water flow sensors with pulse output

#### Detailed Sensor Specifications

**Temperature Sensor (DS18B20)**
- Resolution: 12-bit (0.0625°C precision)
- Range: -55°C to +125°C
- Protocol: OneWire digital communication

**pH Sensor**
- Type: Analog pH sensor
- Range: 0-14 pH
- Voltage: 0-3.3V (mapped to pH scale)
- Calibration: 2-point or 3-point calibration

**TDS Sensor (Gravity V1)**
- Type: Total Dissolved Solids sensor
- Range: 0-1000+ ppm
- Voltage: 0-3.3V (analog read)
- Temperature Compensation: Enabled
- Implementation: Manual analog reading with temperature coefficient

**Water Flow Sensors**
- Type: Hall effect flow sensors
- Pulse Rate: ~450 pulses per liter (configurable)
- Operating Voltage: 5V or 3.3V compatible
- Output: Digital pulse train (interrupt-based counting)
- Flow Rate Calculation: Real-time L/min calculation
- Volume Tracking: Cumulative volume measurement per sensor
- Channels: 4 independent flow meters
  - Nutrisi A (GPIO 4)
  - Nutrisi B (GPIO 18) 
  - pH UP (GPIO 2)
  - pH DOWN (GPIO 19)

### Actuators
- **Pumps**: 5x pumps controlled via relay module
- **Relay Module**: 5-channel relay board (5V trigger)

#### Detailed Actuator Specifications

**Relay Module**
- Type: 5-channel relay board
- Trigger Voltage: 5V (HIGH = ON, LOW = OFF)
- Contact Rating: 10A 250VAC / 10A 30VDC
- Isolation: Optocoupler isolation
- Control Logic: Active HIGH (GPIO HIGH turns relay ON)
- Channels:
  - Relay 1 (GPIO 12): Nutrisi A pump
  - Relay 2 (GPIO 13): Nutrisi B pump  
  - Relay 3 (GPIO 14): pH UP pump
  - Relay 4 (GPIO 27): pH DOWN pump
  - Relay 5 (GPIO 26): Water mixer pump

**Pumps**
- Type: Peristaltic/Submersible pumps
- Operating Voltage: 12V DC (via relay switching)
- Flow Rate: Variable (depends on pump specifications)
- Control: ON/OFF via relay (no PWM speed control)
- Safety Features: Emergency stop function, individual control

## Firebase Configuration

### Database Structure
```
devices/
  └── 1234567890/
      ├── configs/
      │   ├── mode (auto/manual)
      │   ├── setPPM_A
      │   └── relays/
      │       ├── manual/ (relay1-5)
      │       └── auto/ (relay1-5)
      └── sensor_data/
          ├── ph
          ├── tds
          ├── temperature
          └── timestamp
```

### API Configuration
- **API Key**: -
- **Database URL**: -

## Features

### 🤖 Automation Modes
- **Manual Mode**: Manual control semua pump melalui serial commands
- **Auto Mode**: Kontrol otomatis dengan fuzzy logic (TDS control)

### 🧠 Fuzzy Logic Controller
- **Input Variables**: TDS Error dan Delta TDS Error
- **Output**: Pump intensity (0-100%)
- **Membership Functions**: Trapezoidal (5 input, 5 output)
- **Rules**: 15 fuzzy rules untuk TDS control
- **Method**: Mamdani inference dengan centroid defuzzification
- **Setpoint Range**: 1-2000 ppm (default: 1000 ppm)

### 📊 Monitoring
- Real-time sensor monitoring (pH, TDS, Temperature)
- Water flow measurement untuk 4 pump (Nutrisi A, B, pH UP, pH DOWN)
- Interrupt-based pulse counting dengan rate calculation (L/min)
- Total volume tracking untuk setiap flow sensor
- Status system melalui LCD dan Firebase

### 🌐 Connectivity
- **WiFi**: Koneksi internet untuk Firebase
- **NTP**: Sinkronisasi waktu otomatis (GMT+7)
- **Serial**: Konfigurasi WiFi via serial command
- **Firebase**: Real-time database untuk remote control dan monitoring

### 🔥 Firebase Integration
- **Realtime Database**: Bidirectional data sync
- **Authentication**: Anonymous authentication
- **Data Upload**: Automatic sensor data logging
- **Remote Control**: Cloud-based relay control
- **Mode Control**: Auto/Manual mode switching
- **Configuration Sync**: Setpoint updates from cloud

### ⚡ RTOS Tasks
- **Main Task**: Logic utama sistem
- **WiFi Task**: Manajemen koneksi WiFi dan NTP
- **Serial Task**: Handle komunikasi serial

## Serial Commands
```
# WiFi Commands
set ssid <WIFI_NAME>     # Set WiFi SSID
set pass <WIFI_PASSWORD> # Set WiFi password
connect                 # Connect to WiFi
disconnect              # Disconnect from WiFi
show wifi               # Show current WiFi status

# Sensor Commands
show sensors            # Show all sensor readings (temp, pH, TDS, flow rates)
reset flow              # Reset flow counters and volumes

# Relay Commands
relay <1-5> on/off      # Control individual relay (1=PumpA, 2=PumpB, 3=pH UP, 4=pH DOWN, 5=Mixer)
pump a on/off           # Control Nutrisi A pump
pump b on/off           # Control Nutrisi B pump  
pump ph-up on/off       # Control pH UP pump
pump ph-down on/off     # Control pH DOWN pump
pump mixer on/off       # Control mixer pump
show relays             # Show all relay states
stop all                # Emergency stop - turn off all relays

# Fuzzy Logic Commands
set tds <ppm>           # Set TDS setpoint (1-2000 ppm)
show fuzzy              # Show fuzzy logic status
test fuzzy              # Test fuzzy with current TDS values

# Firebase Commands
show firebase           # Show Firebase connection status
sync firebase           # Sync configuration with Firebase
upload data             # Upload sensor data to Firebase

# General Commands
help                    # Show available commands
```

## Intervals & Timing
- **Fuzzy Logic**: 20 detik
- **Firebase Upload**: 2 menit (sensor data)
- **Firebase Check**: 5 detik (control data)
- **Fuzzy Log**: 5 menit
- **WiFi Monitor**: 10 detik
- **NTP Update**: 1 menit

## Development Status
- ✅ Basic RTOS structure
- ✅ WiFi connectivity with NTP
- ✅ Serial communication with runtime commands
- ✅ DS18B20 temperature sensor integration
- ✅ pH sensor integration (analog reading)
- ✅ TDS sensor integration (manual gravity V1 implementation)
- ✅ Water flow sensor integration (4 channels, interrupt-based)
- ✅ Actuator/relay control system (5-channel relay with manual serial control)
- ✅ Fuzzy logic controller (TDS control with 15 rules)
- ✅ Firebase integration (realtime database, bidirectional sync)
- 🚧 LCD display (pending)

## File Structure
```
ProgramV3/
├── Global.h          # Header dengan pin definitions, global variables, function declarations
├── ProgramV3.ino     # Main program dengan RTOS tasks setup
├── WIFI.ino          # WiFi dan NTP management functions
├── SERIAL.ino        # Serial communication dan command handler
├── SENSOR.ino        # All sensor implementations (DS18B20, pH, TDS, Flow)
├── ACTUATOR.ino      # Relay/pump control functions
├── FUZZY.ino         # Fuzzy logic controller for TDS automation
├── FIREBASE.ino      # Firebase integration for cloud connectivity
├── HANDLER.ino       # System mode management and operation handler
└── README.md         # Documentation (this file)
```

## Next Steps
1. ✅ Implementasi sensor reading (pH, TDS, Temperature) - COMPLETED
2. ✅ Implementasi flow meter counting - COMPLETED  
3. ✅ Implementasi relay control - COMPLETED
4. ✅ Implementasi fuzzy logic controller - COMPLETED
5. ✅ Implementasi Firebase integration - COMPLETED
6. ✅ Implementasi system handler untuk mode management - COMPLETED
7. 🚧 Implementasi LCD display
8. 🚧 Testing dan kalibrasi sensor

## HANDLER.ino - System Mode Management

### Overview
`HANDLER.ino` adalah layer abstraksi yang mengelola operasi sistem tingkat tinggi tanpa mengotori program utama. File ini menangani mode switching, data synchronization, dan fuzzy logic execution.

### Key Features
- **Automatic Mode Management**: Deteksi dan switching mode otomatis
- **Timer-based Operations**: Operasi periodik menggunakan FreeRTOS timer
- **Fuzzy Logic Orchestration**: Koordinasi eksekusi fuzzy logic untuk mode auto
- **Data Synchronization**: Pengelolaan sync data dengan Firebase
- **Emergency Controls**: Fungsi emergency stop dan resume operations

### Handler Functions

#### Initialization
```cpp
void initHandler()                    // Initialize handler system
bool isHandlerReady()                // Check if handler is ready
```

#### Mode Management
```cpp
void handleModeManagement()          // Handle mode detection and switching
void enableAutoMode()                // Enable automatic fuzzy control
void enableManualMode()              // Enable manual Firebase control
void refreshMode()                   // Force mode refresh from Firebase
String getCurrentMode()              // Get current active mode
bool isAutoModeActive()             // Check if auto mode is active
bool isManualModeActive()           // Check if manual mode is active
```

#### Data Operations
```cpp
void handleDataSync()               // Handle periodic data sync
void forcedDataSync()              // Force immediate data sync
```

#### Fuzzy Logic Control (Auto Mode)
```cpp
void initializeFuzzySystem()       // Initialize fuzzy logic system
void handleFuzzyExecution()        // Execute fuzzy control logic
void applyFuzzyOutput(float output) // Apply fuzzy output to relays
void updateAutoRelayState(int relay, bool state) // Update auto relay states
```

#### Emergency Controls
```cpp
void emergencyStop()               // Emergency stop all operations
void resumeOperations()            // Resume normal operations
```

#### Status & Monitoring
```cpp
void getHandlerStatus()            // Display handler system status
```

### Operation Flow

#### Auto Mode Flow
1. **Mode Detection**: Handler detects "auto" mode from Firebase
2. **Fuzzy Initialization**: Initialize fuzzy logic system
3. **Setpoint Reading**: Read auto setpoint from Firebase (`setPPM_A`)
4. **Periodic Execution**: 
   - Read sensor values (TDS, pH, temperature)
   - Execute fuzzy logic algorithm
   - Apply fuzzy output to auto relay paths
   - Log results to Firebase
5. **State Update**: Update Firebase auto relay states

#### Manual Mode Flow
1. **Mode Detection**: Handler detects "manual" mode from Firebase
2. **Configuration Reading**: Read manual setpoints (`setPPM_M0-4`)
3. **Firebase Control**: 
   - Read manual relay states from Firebase
   - Apply relay states to physical hardware
   - Monitor setpoint changes
4. **Direct Control**: User/App controls relays directly via Firebase

### Timer Configuration
```cpp
MODE_CHECK_INTERVAL = 5000ms       // Mode check frequency
DATA_SYNC_INTERVAL = 30000ms       // Data upload frequency  
FUZZY_RUN_INTERVAL = 10000ms       // Fuzzy execution frequency
FIREBASE_SYNC_INTERVAL = 15000ms   // Firebase sync frequency
```

### Integration with Main Program
Handler terintegrasi seamlessly dengan program utama:
- **Zero Impact**: Tidak mengubah struktur `ProgramV3.ino`
- **Self-Contained**: Semua logic ada di `HANDLER.ino`
- **Timer-Based**: Operasi berjalan otomatis via FreeRTOS timer
- **Clean Architecture**: Layer abstraksi yang jelas

### Usage Example
```cpp
// In ProgramV3.ino setup() - hanya tambah 1 baris
void setup() {
    // ... existing setup code ...
    initHandler();  // Initialize handler system
}

// Handler akan otomatis mengelola:
// - Mode switching (manual/auto)
// - Fuzzy logic execution (auto mode)
// - Data synchronization
// - Firebase communication
// - Relay control
```
