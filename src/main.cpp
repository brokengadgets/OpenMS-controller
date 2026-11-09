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
    
    // Check if active profiles have reached their final time point
    if (config.getTemperatureProfile().active && config.getTemperatureProfile().isComplete()) {
      config.getTemperatureProfile().stop();
      tempPID.reset();
      analogIO.setHeaterOutput(0.0f);
      Serial.println("Temperature profile completed");
    }
    
    if (config.getPressureProfile().active && config.getPressureProfile().isComplete()) {
      config.getPressureProfile().stop();
      pressPID.reset();
      analogIO.setPressureOutput(0.0f);
      Serial.println("Pressure profile completed");
    }
    
    // Update temperature control when profile is active; otherwise keep heater off
    if (config.getTemperatureProfile().active) {
      float tempSetpoint = config.getTemperatureProfile().getCurrentSetpoint();
      tempPID.setSetpoint(tempSetpoint);
      float heaterOutput = tempPID.compute(currentTemp);
      analogIO.setHeaterOutput(heaterOutput);
    } else {
      tempPID.reset();
      analogIO.setHeaterOutput(0.0f);
    }
    
    // Update pressure control when profile is active; otherwise keep valve/pump off
    if (config.getPressureProfile().active) {
      float pressSetpoint = config.getPressureProfile().getCurrentSetpoint();
      pressPID.setSetpoint(pressSetpoint);
      float pressureOutput = pressPID.compute(currentPress);
      analogIO.setPressureOutput(pressureOutput);
    } else {
      pressPID.reset();
      analogIO.setPressureOutput(0.0f);
    }
    
    // Set MS output (0-100% PWM) scaled from calibrated 0-5V MS signal
    float msGain = config.getMSGain();
    float msOffset = config.getMSOffset();
    float calibratedMSVolts = (msValue * msGain) + msOffset;
    float msOutputPercent = (calibratedMSVolts / 5.0f) * 100.0f;
    analogIO.setMSOutput(msOutputPercent);
    
    // Log data if enabled
    serialComm.logData(currentTemp, currentPress, calibratedMSVolts);
  }
  
  // Send periodic status updates
  if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
    lastStatusUpdate = currentTime;
    serialComm.sendStatus();
  }
}