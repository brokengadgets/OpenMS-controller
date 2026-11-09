#include "PIDController.h"

PIDController::PIDController(float kp, float ki, float kd, float outputMin, float outputMax) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
    this->outputMin = outputMin;
    this->outputMax = outputMax;
    this->setpoint = 0.0f;
    
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
    if (timeChange > 0.0f) {
        integral += ki * error * timeChange;
        if (integral > outputMax) integral = outputMax;
        else if (integral < outputMin) integral = outputMin;
    }
    
    // Derivative-on-measurement to prevent derivative kick on setpoint changes
    float derivative = 0.0f;
    if (hasLastInput && timeChange > 0.0f) {
        derivative = -kd * (input - lastInput) / timeChange;
    }
    lastInput = input;
    hasLastInput = true;
    lastError = error;
    
    // Calculate output
    float output = proportional + integral + derivative;
    
    // Constrain output to limits
    if (output > outputMax) output = outputMax;
    else if (output < outputMin) output = outputMin;
    
    lastOutput = output;
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
    integral = 0.0f;
    lastError = 0.0f;
    lastInput = 0.0f;
    lastOutput = 0.0f;
    hasLastInput = false;
    lastTime = millis();
    steadyStateValue = 0.0f;
    peakHigh = -1e6f;
    peakLow = 1e6f;
    lastCrossTime = 0;
    oscillationPeriodMs = 0;
    cycleCount = 0;
    tuningActive = false;
}

void PIDController::startAutoTune() {
    tuningActive = true;
    steadyStateValue = setpoint;
    peakHigh = -1e6f;
    peakLow = 1e6f;
    lastCrossTime = millis();
    oscillationPeriodMs = 0;
    cycleCount = 0;
}

bool PIDController::autoTuneStep(float input) {
    if (!tuningActive) return false;
    
    unsigned long now = millis();
    steadyStateValue = (steadyStateValue * 0.95f) + (input * 0.05f);
    
    if (input > peakHigh) peakHigh = input;
    if (input < peakLow) peakLow = input;
    
    // Relay feedback step around setpoint
    float prevOutput = lastOutput;
    lastOutput = (input < setpoint) ? outputMax : outputMin;
    lastError = setpoint - input;
    
    // Detect upward relay switching (crossing setpoint from above to below)
    if (prevOutput == outputMin && lastOutput == outputMax) {
        if (lastCrossTime > 0 && now > lastCrossTime) {
            oscillationPeriodMs = now - lastCrossTime;
            cycleCount++;
        }
        lastCrossTime = now;
    }
    
    // Complete after 4 full oscillation cycles with valid amplitude and period
    float amplitude = (peakHigh - peakLow) * 0.5f;
    if (cycleCount >= 4 && amplitude > 0.01f && oscillationPeriodMs > 0) {
        float relayStep = (outputMax - outputMin) * 0.5f;
        float ku = (4.0f * relayStep) / (3.14159265f * amplitude);
        float tu = oscillationPeriodMs / 1000.0f;
        
        // Classic Ziegler-Nichols PID tuning
        setTunings(0.6f * ku, (1.2f * ku) / tu, 0.075f * ku * tu);
        tuningActive = false;
        return true;
    }
    
    return false;
}

void PIDController::stopAutoTune() {
    tuningActive = false;
}

float PIDController::getLastOutput() const {
    return lastOutput;
}

float PIDController::getCurrentError() const {
    return lastError;
}