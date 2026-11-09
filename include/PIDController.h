#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PIDController {
private:
    // PID parameters
    float kp;  // Proportional gain
    float ki;  // Integral gain
    float kd;  // Derivative gain
    
    // PID internal variables
    float setpoint;         // Desired value
    float lastError;        // Last error
    float lastInput;        // Last measurement (for derivative-on-measurement)
    float lastOutput;       // Last computed output
    bool hasLastInput;      // Whether lastInput has been initialized
    float integral;         // Accumulated integral
    float outputMin;        // Output limits
    float outputMax;
    unsigned long lastTime; // Last sample time
    
    // System identification and analysis
    float steadyStateValue; // Used for auto-tuning and monitoring
    float peakHigh;         // Relay auto-tune peak tracking
    float peakLow;          // Relay auto-tune trough tracking
    unsigned long lastCrossTime; // Last relay zero-crossing timestamp
    unsigned long oscillationPeriodMs; // Measured oscillation period
    uint8_t cycleCount;     // Number of relay cycles observed
    bool tuningActive;      // Flag for auto-tuning

public:
    PIDController(float kp, float ki, float kd, float outputMin, float outputMax);
    
    // Main PID computation
    float compute(float input);
    
    // Configuration methods
    void setTunings(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setSetpoint(float setpoint);
    float getSetpoint() const;
    
    // Runtime management
    void reset();
    
    // Auto-tuning and system identification
    void startAutoTune();
    bool autoTuneStep(float input);
    void stopAutoTune();
    
    // Status
    float getLastOutput() const;
    float getCurrentError() const;
};

#endif // PID_CONTROLLER_H