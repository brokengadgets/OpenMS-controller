# OpenMS Gas Chromatograph Controller

A comprehensive system for controlling and monitoring a Gas Chromatograph using an Arduino Mega 2560 and desktop software. This project includes both the Arduino firmware and a Python-based desktop application.

## Features

- **Dual PID Control**: Precise temperature and pressure regulation with PID algorithms
- **Time-Based Profiles**: Define complex temperature and pressure profiles with time-based setpoints
- **Mass Spectrometer Interface**: Analog I/O for mass spectrometer integration
- **Data Logging**: Comprehensive data collection and storage
- **Configuration Management**: Load and save instrument settings
- **Command Line Interface**: Powerful CLI with interactive mode

## Hardware Requirements

- Arduino Mega 2560 board
- Temperature sensor (analog)
- Pressure sensor (analog)
- Heater control circuit (PWM)
- Pressure control valve/pump (PWM)
- Mass Spectrometer with analog input/output
- Serial connection to computer

## Pin Assignments

| Component | Arduino Pin |
|-----------|-------------|
| Temperature Sensor | A0 |
| Pressure Sensor | A1 |
| MS Input | A2 |
| MS Output | 3 (PWM) |
| Heater Output | 5 (PWM) |
| Pressure Output | 6 (PWM) |

## Building the Project

### Arduino Firmware Setup

1. Install [PlatformIO IDE](https://platformio.org/install/ide) (as a VSCode extension or standalone)
2. Clone this repository:
   ```
   git clone https://github.com/yourusername/OpenMS-controller.git
   cd OpenMS-controller
   ```
3. Open the project in PlatformIO IDE
4. Customize the firmware if needed:
   - Modify pin assignments in `src/main.cpp` if your hardware differs
   - Adjust PID parameters in `src/main.cpp` for your specific thermal and pressure characteristics
   - Update calibration values in `AnalogIO.cpp` for your sensors
5. Build the firmware:
   ```
   platformio run
   ```
6. Connect the Arduino Mega 2560 to your computer
7. Upload the firmware:
   ```
   platformio run --target upload
   ```
8. Monitor the serial output (optional):
   ```
   platformio device monitor
   ```

### Hardware Setup

1. **Temperature Sensor Connection**:
   - Connect the temperature sensor to analog pin A0
   - Use appropriate signal conditioning circuit if needed (op-amp, voltage divider, etc.)
   - Ensure the sensor output is within the 0-5V range of the Arduino's ADC

2. **Pressure Sensor Connection**:
   - Connect the pressure sensor to analog pin A1
   - Include any necessary signal conditioning

3. **Mass Spectrometer Interface**:
   - Connect the MS analog input to pin A2
   - Connect the MS control output to PWM pin 3
   - Implement any required buffer amplifiers or isolation circuits

4. **Heater Control**:
   - Connect the heater control circuit to PWM pin 5
   - Use appropriate power transistors/MOSFETs for driving high-current loads
   - Implement safety features like thermal fuses and overtemperature protection

5. **Pressure Control**:
   - Connect the pressure control valve/pump to PWM pin 6
   - Use appropriate driver circuits for the specific actuator

6. **Serial Connection**:
   - Connect the Arduino to the computer via USB for development
   - For permanent installation, consider using hardware serial pins (0/1) with an appropriate level shifter

### Desktop Software Setup

1. Install Python 3.6 or higher
2. Create a virtual environment (recommended):
   ```
   python -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```
3. Install required libraries:
   ```
   pip install pyserial
   ```
4. Make the controller script executable (Linux/macOS):
   ```
   chmod +x desktop/openms_controller.py
   ```
5. Run the controller software:
   ```
   cd desktop
   python openms_controller.py -i
   ```

## Using the System

### Quick Start Guide

1. **Connect Hardware**:
   - Ensure all hardware is properly connected as described in the Hardware Setup section
   - Power on the Arduino and other components
   
2. **Initial Test**:
   - Run the desktop software in interactive mode:
     ```
     cd desktop
     python openms_controller.py -i
     ```
   - Check if the controller responds by typing `:status`
   - You should see temperature and pressure readings
   
3. **Run a Basic Profile**:
   - Use the sample profile provided:
     ```
     python openms_controller.py --profile sample_profile.json --run all
     ```
   - This will load and run both temperature and pressure profiles
   
4. **Monitor the Run**:
   - Watch the status updates in the terminal
   - Data is automatically logged to the `logs` directory
   - After completion, the system will return to idle state

### Common Use Cases

#### 1. Temperature-Only Run with Custom Profile

```bash
# Create a simple temperature profile
cat > temp_only.json << EOF
{
  "temperature_profile": [
    {"time": 0, "value": 40.0},
    {"time": 120, "value": 100.0},
    {"time": 300, "value": 100.0},
    {"time": 360, "value": 40.0}
  ],
  "pressure_profile": []
}
EOF

# Run with just the temperature profile
python desktop/openms_controller.py --profile temp_only.json --run temp
```

#### 2. Configure and Save Settings

```bash
# Start interactive mode
python desktop/openms_controller.py -i

# Set MS gain and offset (at the prompt)
SET_MS:1.2:0.05

# Set sample rate to 500ms
SAMPLE_RATE:500

# Save the current configuration
:quit

# Save the configuration to a file
python desktop/openms_controller.py --save-config my_settings.json
```

#### 3. Automating Multiple Runs

Create a shell script to run multiple profiles in sequence:

```bash
#!/bin/bash
# run_sequence.sh

echo "Starting sequence of GC runs"

# Run with first profile
python desktop/openms_controller.py --profile profile1.json --run all
sleep 10  # Wait for cooldown

# Run with second profile
python desktop/openms_controller.py --profile profile2.json --run all
sleep 10  # Wait for cooldown

# Run with third profile
python desktop/openms_controller.py --profile profile3.json --run all

echo "Sequence completed"
```

### Troubleshooting

#### Arduino Connection Issues

- **No Serial Port Detected**:
  - Check USB cable connection
  - Try a different USB port
  - Verify the Arduino is powered (LED indicators)
  - Check if drivers are installed (especially for clone boards)

- **Communication Errors**:
  - Ensure the baud rate matches (default: 115200)
  - Check for conflicting software using the same port
  - Try resetting the Arduino

#### Temperature Control Issues

- **Unstable Temperature**:
  - Tune PID parameters in `src/main.cpp` (lower Kp for less oscillation)
  - Check sensor placement and thermal coupling
  - Verify heater power is adequate for the application

- **Slow Response**:
  - Increase Kp and Ki values
  - Check for thermal limitations in the system
  - Verify heater power output

#### Data Logging Issues

- **Missing Data Points**:
  - Decrease sample rate (increase time between samples)
  - Check for serial buffer overflows
  - Verify disk space and write permissions

### Calibration Procedure

For accurate measurements, calibrate the system as follows:

1. **Temperature Calibration**:
   - Use a reference thermometer
   - Set the system to several known temperatures
   - Record actual vs. measured values
   - Update calibration in `AnalogIO.cpp`:
     ```cpp
     analogIO.calibrateTemperature(offset, gain);
     ```

2. **Pressure Calibration**:
   - Use a reference pressure gauge
   - Apply several known pressures
   - Record actual vs. measured values
   - Update calibration in `AnalogIO.cpp`:
     ```cpp
     analogIO.calibratePressure(offset, gain);
     ```

### Configuration Files

The desktop software accepts JSON configuration files that define the system parameters and profiles. See `desktop/sample_config.json` for an example.

Example configuration:
```json
{
  "serial_port": "COM3",
  "baud_rate": 115200,
  "log_directory": "logs",
  "temperature_profile": [...],
  "pressure_profile": [...],
  "ms_gain": 1.0,
  "ms_offset": 0.0,
  "sample_rate": 1000,
  "logging_enabled": true
}
```

### Profile Files

Profiles define time-based temperature and pressure settings. See `desktop/sample_profile.json` for an example.

Example profile (simplified):
```json
{
  "name": "Standard GC Run",
  "temperature_profile": [
    {"time": 0, "value": 40.0},
    {"time": 360, "value": 200.0},
    {"time": 1200, "value": 40.0}
  ],
  "pressure_profile": [
    {"time": 0, "value": 15.0},
    {"time": 360, "value": 30.0},
    {"time": 1200, "value": 15.0}
  ]
}
```

### Command Line Options

The desktop software supports the following command line options:

```
usage: openms_controller.py [-h] [-p PORT] [-b BAUD] [-c CONFIG]
                         [-s SAVE_CONFIG] [--profile PROFILE]
                         [--run {temp,press,all}] [--command COMMAND]
                         [--log-dir LOG_DIR] [--no-log]
                         [--ms-gain MS_GAIN] [--ms-offset MS_OFFSET]
                         [--sample-rate SAMPLE_RATE] [-i]

OpenMS Gas Chromatograph Controller

options:
  -h, --help            show this help message and exit
  -p PORT, --port PORT  Serial port to connect to
  -b BAUD, --baud BAUD  Baud rate for serial connection
  -c CONFIG, --config CONFIG
                        Load configuration from file
  -s SAVE_CONFIG, --save-config SAVE_CONFIG
                        Save configuration to file
  --profile PROFILE     Load profile from file
  --run {temp,press,all}
                        Run profile(s)
  --command COMMAND     Send a command to the controller
  --log-dir LOG_DIR     Directory for data logs
  --no-log              Disable data logging
  --ms-gain MS_GAIN     MS gain value
  --ms-offset MS_OFFSET
                        MS offset value
  --sample-rate SAMPLE_RATE
                        Sample rate in milliseconds
  -i, --interactive     Run in interactive mode
```

### Interactive Mode

In interactive mode, you can enter commands directly. Special commands begin with a colon:

- `:help` - Show help
- `:quit` - Exit the program
- `:start` - Start logging data
- `:stop` - Stop logging data
- `:run temp` - Start temperature profile
- `:run press` - Start pressure profile
- `:run all` - Start all profiles
- `:stop temp` - Stop temperature profile
- `:stop press` - Stop pressure profile
- `:stop all` - Stop all profiles
- `:status` - Request status from controller

Other commands are sent directly to the Arduino controller.

### Controller Command Format

The Arduino controller accepts commands in the following format:

- `TEMP_PROFILE:CLEAR` - Clear temperature profile
- `TEMP_PROFILE:ADD:time:value` - Add point to temperature profile
- `PRESS_PROFILE:CLEAR` - Clear pressure profile
- `PRESS_PROFILE:ADD:time:value` - Add point to pressure profile
- `START_PROFILE:{TEMP|PRESS|ALL}` - Start profile(s)
- `STOP_PROFILE:{TEMP|PRESS|ALL}` - Stop profile(s)
- `SET_MS:gain:offset` - Set MS parameters
- `SAMPLE_RATE:rate_ms` - Set sample rate
- `LOGGING:{ON|OFF}` - Enable/disable logging

## Data Logging

The system logs data in CSV format with the following columns:
- Timestamp (computer time)
- System time (milliseconds)
- Temperature value
- Pressure value
- MS value

Log files are stored in the configured log directory with a timestamp in the filename.

## PID Control

The system uses PID (Proportional-Integral-Derivative) control for both temperature and pressure regulation. The PID parameters can be tuned for your specific hardware by modifying the initialization in `src/main.cpp`:

```cpp
// PID controllers
PIDController tempPID(10.0, 0.1, 1.0, 0.0, 100.0);  // Kp, Ki, Kd, min, max
PIDController pressPID(5.0, 0.05, 0.5, 0.0, 100.0); // Kp, Ki, Kd, min, max
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Development and Extending the Project

### Project Structure

```
OpenMS-controller/
├── include/                 # Header files
│   ├── PIDController.h      # PID controller class
│   ├── GCConfig.h           # Configuration and profiles
│   ├── AnalogIO.h           # Analog I/O handling
│   └── SerialComm.h         # Serial communication
├── src/                     # Implementation files
│   ├── main.cpp             # Main Arduino program
│   ├── PIDController.cpp    # PID implementation
│   ├── GCConfig.cpp         # Config implementation
│   ├── AnalogIO.cpp         # I/O implementation
│   └── SerialComm.cpp       # Serial implementation
├── desktop/                 # Desktop software
│   ├── openms_controller.py # Main Python application
│   ├── sample_config.json   # Example configuration
│   └── sample_profile.json  # Example profile
├── platformio.ini           # PlatformIO configuration
└── README.md                # Documentation
```

### Extending the Arduino Firmware

To add new features to the firmware:

1. **Add New Hardware Support**:
   - Create a new class for the hardware in include/ and src/
   - Add initialization in main.cpp
   - Integrate with the existing control loop

2. **Enhance PID Control**:
   - Modify PIDController.h/cpp to add advanced features
   - Implement auto-tuning algorithms
   - Add cascade control for complex systems

3. **Add Communication Protocols**:
   - Extend SerialComm.h/cpp to support new commands
   - Implement additional communication interfaces (I2C, SPI, etc.)

### Extending the Desktop Software

1. **Add Data Visualization**:
   - Integrate with plotting libraries like matplotlib
   - Create real-time visualization of temperature and pressure

2. **Implement Data Analysis**:
   - Add peak detection algorithms
   - Implement calibration and quantification

3. **Create a GUI**:
   - Build a graphical interface using frameworks like Qt or Tkinter
   - Implement visual profile editor

### Contributing

Contributions to this project are welcome! Here's how to contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request