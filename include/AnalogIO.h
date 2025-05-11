#ifndef ANALOG_IO_H
#define ANALOG_IO_H

#include <Arduino.h>

// Class to handle analog I/O for Mass Spectrometer and other sensors
class AnalogIO {
private:
    // Pin assignments
    uint8_t temperatureSensorPin;   // Input from temperature sensor
    uint8_t pressureSensorPin;      // Input from pressure sensor
    uint8_t msInputPin;             // Input from Mass Spectrometer
    uint8_t msOutputPin;            // Output to Mass Spectrometer
    uint8_t heaterOutputPin;        // Output to heater
    uint8_t pressureOutputPin;      // Output to pressure control

    // Calibration values
    float tempSensorOffset;
    float tempSensorGain;
    float pressSensorOffset;
    float pressSensorGain;
    float msInputOffset;
    float msInputGain;
    
    // Current readings
    float temperatureValue;
    float pressureValue;
    float msValue;
    
    // Last output values
    int heaterOutputValue;
    int pressureOutputValue;
    int msOutputValue;
    
    // Conversion functions
    float adcToTemperature(int adcValue);
    float adcToPressure(int adcValue);
    float adcToMSValue(int adcValue);
    
public:
    AnalogIO(uint8_t tempPin, uint8_t pressPin, uint8_t msInPin, uint8_t msOutPin, 
             uint8_t heaterPin, uint8_t pressControlPin);
    
    // Initialization
    void begin();
    
    // Sensor readings
    void readSensors();
    float getTemperature() const { return temperatureValue; }
    float getPressure() const { return pressureValue; }
    float getMSValue() const { return msValue; }
    
    // Actuator control
    void setHeaterOutput(float value);    // 0.0 to 100.0 (percentage)
    void setPressureOutput(float value);  // 0.0 to 100.0 (percentage)
    void setMSOutput(float value);        // 0.0 to 100.0 (percentage)
    
    // Calibration
    void calibrateTemperature(float offset, float gain);
    void calibratePressure(float offset, float gain);
    void calibrateMS(float offset, float gain);
};

#endif // ANALOG_IO_H