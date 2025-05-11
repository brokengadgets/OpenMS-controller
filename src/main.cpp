#include <Arduino.h>
#include "PIDController.h"
#include "GCConfig.h"
#include "AnalogIO.h"
#include "SerialComm.h"

// Pin definitions
#define TEMP_SENSOR_PIN      A0
#define PRESSURE_SENSOR_PIN  A1
#define MS_INPUT_PIN         A2
#define MS_OUTPUT_PIN        3  // PWM capable pin
#define HEATER_OUTPUT_PIN    5  // PWM capable pin
#define PRESSURE_OUTPUT_PIN  6  // PWM capable pin

// Update interval (ms)
#define UPDATE_INTERVAL 100

// Global objects
GCConfig config;
AnalogIO analogIO(TEMP_SENSOR_PIN, PRESSURE_SENSOR_PIN, MS_INPUT_PIN, 
                  MS_OUTPUT_PIN, HEATER_OUTPUT_PIN, PRESSURE_OUTPUT_PIN);
SerialComm serialComm(config);

// PID controllers
PIDController tempPID(10.0, 0.1, 1.0, 0.0, 100.0);  // Initial PID values (should be tuned)
PIDController pressPID(5.0, 0.05, 0.5, 0.0, 100.0); // Initial PID values (should be tuned)

// Timing variables
unsigned long lastUpdateTime = 0;
unsigned long statusUpdateInterval = 1000; // 1 second status updates
unsigned long lastStatusUpdate = 0;

void setup() {
  // Initialize analog I/O
  analogIO.begin();
  
  // Initialize serial communication
  serialComm.begin(115200);
  
  // Initial PID setpoints (will be updated by profiles)
  tempPID.setSetpoint(25.0);  // Room temperature default
  pressPID.setSetpoint(0.0);  // Ambient pressure default
  
  // Setup complete
  Serial.println("Setup complete. System ready.");
}

void loop() {
  // Update serial communication (check for commands)
  serialComm.update();
  
  // Get current time
  unsigned long currentTime = millis();
  
  // Run the main control loop at the specified interval
  if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = currentTime;
    
    // Read all analog inputs
    analogIO.readSensors();
    
    // Get current temperature and pressure values
    float currentTemp = analogIO.getTemperature();
    float currentPress = analogIO.getPressure();
    float msValue = analogIO.getMSValue();
    
    // Get setpoints from active profiles
    if (config.getTemperatureProfile().active) {
      float tempSetpoint = config.getTemperatureProfile().getCurrentSetpoint();
      tempPID.setSetpoint(tempSetpoint);
    }
    
    if (config.getPressureProfile().active) {
      float pressSetpoint = config.getPressureProfile().getCurrentSetpoint();
      pressPID.setSetpoint(pressSetpoint);
    }
    
    // Compute PID outputs
    float heaterOutput = tempPID.compute(currentTemp);
    float pressureOutput = pressPID.compute(currentPress);
    
    // Set outputs to actuators
    analogIO.setHeaterOutput(heaterOutput);
    analogIO.setPressureOutput(pressureOutput);
    
    // Set MS output based on configured gain/offset
    float msGain = config.getMSGain();
    float msOffset = config.getMSOffset();
    float msOutputValue = (msValue * msGain) + msOffset;
    analogIO.setMSOutput(msOutputValue);
    
    // Log data if enabled
    serialComm.logData(currentTemp, currentPress, msValue);
  }
  
  // Send periodic status updates
  if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
    lastStatusUpdate = currentTime;
    serialComm.sendStatus();
    
    // Check if profiles are complete
    if (config.getTemperatureProfile().active && config.getTemperatureProfile().isComplete()) {
      config.getTemperatureProfile().stop();
      Serial.println("Temperature profile completed");
    }
    
    if (config.getPressureProfile().active && config.getPressureProfile().isComplete()) {
      config.getPressureProfile().stop();
      Serial.println("Pressure profile completed");
    }
  }
}