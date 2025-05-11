#include "PIDController.h"

PIDController::PIDController(float kp, float ki, float kd, float outputMin, float outputMax) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
    this->outputMin = outputMin;
    this->outputMax = outputMax;
    
    reset();
}

float PIDController::compute(float input) {
    // Calculate time delta
    unsigned long now = millis();
    float timeChange = (now - lastTime) / 1000.0f; // Convert to seconds
    lastTime = now;
    
    // Error computation
    float error = setpoint - input;
    
    // Proportional term
    float proportional = kp * error;
    
    // Integral term with anti-windup
    integral += ki * error * timeChange;
    if (integral > outputMax) integral = outputMax;
    else if (integral < outputMin) integral = outputMin;
    
    // Derivative term
    float derivative = 0;
    if (timeChange > 0) {
        derivative = kd * (error - lastError) / timeChange;
    }
    lastError = error;
    
    // Calculate output
    float output = proportional + integral + derivative;
    
    // Constrain output to limits
    if (output > outputMax) output = outputMax;
    else if (output < outputMin) output = outputMin;
    
    return output;
}

void PIDController::setTunings(float kp, float ki, float kd) {
    // Validate parameters (prevent negative gains unless specially needed)
    if (kp < 0 || ki < 0 || kd < 0) return;
    
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

void PIDController::setOutputLimits(float min, float max) {
    if (min >= max) return;
    
    outputMin = min;
    outputMax = max;
    
    // Adjust integral term if outside new limits
    if (integral > outputMax) integral = outputMax;
    else if (integral < outputMin) integral = outputMin;
}

void PIDController::setSetpoint(float newSetpoint) {
    setpoint = newSetpoint;
}

float PIDController::getSetpoint() const {
    return setpoint;
}

void PIDController::reset() {
    integral = 0.0;
    lastError = 0.0;
    lastTime = millis();
    steadyStateValue = 0.0;
    tuningActive = false;
}

void PIDController::startAutoTune() {
    // Simple auto-tuning flag - actual implementation would use more complex algorithms
    tuningActive = true;
    steadyStateValue = 0.0;
    // In a real implementation, you would use methods like Ziegler-Nichols or relay methods
}

bool PIDController::autoTuneStep(float input) {
    // This is a placeholder for auto-tuning logic
    // A real implementation would implement methods like relay feedback, step response, etc.
    
    if (!tuningActive) return false;
    
    // Simple steady-state detection (just an example, not a real auto-tuning algorithm)
    steadyStateValue = (steadyStateValue * 0.95) + (input * 0.05);
    
    // In a real implementation, this would calculate and set optimal PID parameters
    
    return false; // Return true when tuning is complete
}

void PIDController::stopAutoTune() {
    tuningActive = false;
}

float PIDController::getLastOutput() const {
    return lastError + kp; // This is just a proxy to estimate last output
}

float PIDController::getCurrentError() const {
    return lastError;
}