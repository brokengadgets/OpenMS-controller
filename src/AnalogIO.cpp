#include "AnalogIO.h"

AnalogIO::AnalogIO(uint8_t tempPin, uint8_t pressPin, uint8_t msInPin, uint8_t msOutPin, 
                   uint8_t heaterPin, uint8_t pressControlPin) {
    temperatureSensorPin = tempPin;
    pressureSensorPin = pressPin;
    msInputPin = msInPin;
    msOutputPin = msOutPin;
    heaterOutputPin = heaterPin;
    pressureOutputPin = pressControlPin;
    
    // Default linear calibration matching 0-5V ADC input ranges:
    // Temperature: 0-5V -> 0-250 °C (50 °C/V)
    // Pressure:    0-5V -> 0-100 PSI (20 PSI/V)
    // MS input:    0-5V -> 0-5 V (1 V/V)
    tempSensorOffset = 0.0f;
    tempSensorGain = 50.0f;
    pressSensorOffset = 0.0f;
    pressSensorGain = 20.0f;
    msInputOffset = 0.0f;
    msInputGain = 1.0f;
    
    // Initialize values
    temperatureValue = 0.0;
    pressureValue = 0.0;
    msValue = 0.0;
    heaterOutputValue = 0;
    pressureOutputValue = 0;
    msOutputValue = 0;
}

void AnalogIO::begin() {
    // Configure pins
    pinMode(temperatureSensorPin, INPUT);
    pinMode(pressureSensorPin, INPUT);
    pinMode(msInputPin, INPUT);
    
    pinMode(msOutputPin, OUTPUT);
    pinMode(heaterOutputPin, OUTPUT);
    pinMode(pressureOutputPin, OUTPUT);
    
    // Initialize outputs to zero
    analogWrite(msOutputPin, 0);
    analogWrite(heaterOutputPin, 0);
    analogWrite(pressureOutputPin, 0);
}

float AnalogIO::adcToTemperature(int adcValue) {
    // Convert ADC reading to actual temperature
    // This implementation assumes linear relationship
    // For real-world applications, you might use lookup tables or non-linear formulas
    
    // Example: 10-bit ADC (0-1023) to temperature in Celsius (0-250°C)
    float voltage = (adcValue / 1023.0) * 5.0; // Convert to voltage (0-5V)
    float temperature = (voltage * tempSensorGain) + tempSensorOffset;
    
    return temperature;
}

float AnalogIO::adcToPressure(int adcValue) {
    // Convert ADC reading to actual pressure
    // This implementation assumes linear relationship
    
    // Example: 10-bit ADC (0-1023) to pressure in PSI (0-100 PSI)
    float voltage = (adcValue / 1023.0) * 5.0; // Convert to voltage (0-5V)
    float pressure = (voltage * pressSensorGain) + pressSensorOffset;
    
    return pressure;
}

float AnalogIO::adcToMSValue(int adcValue) {
    // Convert ADC reading to MS signal value
    float voltage = (adcValue / 1023.0) * 5.0; // Convert to voltage (0-5V)
    float value = (voltage * msInputGain) + msInputOffset;
    
    return value;
}

void AnalogIO::readSensors() {
    // Read all analog sensors and convert to appropriate units
    int tempADC = analogRead(temperatureSensorPin);
    int pressADC = analogRead(pressureSensorPin);
    int msADC = analogRead(msInputPin);
    
    // Convert ADC values to actual measurements
    temperatureValue = adcToTemperature(tempADC);
    pressureValue = adcToPressure(pressADC);
    msValue = adcToMSValue(msADC);
}

void AnalogIO::setHeaterOutput(float value) {
    // Ensure value is within 0-100% range
    if (value < 0.0) value = 0.0;
    if (value > 100.0) value = 100.0;
    
    // Convert percentage to PWM value (assuming 8-bit PWM)
    heaterOutputValue = (int)(value * 255.0 / 100.0);
    analogWrite(heaterOutputPin, heaterOutputValue);
}

void AnalogIO::setPressureOutput(float value) {
    // Ensure value is within 0-100% range
    if (value < 0.0) value = 0.0;
    if (value > 100.0) value = 100.0;
    
    // Convert percentage to PWM value (assuming 8-bit PWM)
    pressureOutputValue = (int)(value * 255.0 / 100.0);
    analogWrite(pressureOutputPin, pressureOutputValue);
}

void AnalogIO::setMSOutput(float value) {
    // Ensure value is within 0-100% range
    if (value < 0.0) value = 0.0;
    if (value > 100.0) value = 100.0;
    
    // Convert percentage to PWM value (assuming 8-bit PWM)
    msOutputValue = (int)(value * 255.0 / 100.0);
    analogWrite(msOutputPin, msOutputValue);
}

void AnalogIO::calibrateTemperature(float offset, float gain) {
    tempSensorOffset = offset;
    tempSensorGain = gain;
}

void AnalogIO::calibratePressure(float offset, float gain) {
    pressSensorOffset = offset;
    pressSensorGain = gain;
}

void AnalogIO::calibrateMS(float offset, float gain) {
    msInputOffset = offset;
    msInputGain = gain;
}