#ifndef GC_CONFIG_H
#define GC_CONFIG_H

#include <Arduino.h>

// Maximum number of time points in a temperature or pressure profile
#define MAX_PROFILE_POINTS 20

// Structure to represent a single setpoint in a time-based profile
struct ProfilePoint {
    unsigned long timeOffset; // Time in milliseconds from start
    float setValue;           // Target value at this time point
    
    ProfilePoint() : timeOffset(0), setValue(0.0) {}
    ProfilePoint(unsigned long t, float val) : timeOffset(t), setValue(val) {}
};

// Structure to hold a complete profile (temperature or pressure)
struct Profile {
    ProfilePoint points[MAX_PROFILE_POINTS];
    uint8_t numPoints;
    bool active;
    unsigned long startTime;
    
    Profile() : numPoints(0), active(false), startTime(0) {}
    
    // Add a point to the profile
    bool addPoint(unsigned long timeOffset, float setValue) {
        if (numPoints >= MAX_PROFILE_POINTS) return false;
        
        points[numPoints] = ProfilePoint(timeOffset, setValue);
        numPoints++;
        
        // Sort points by time (simple insertion sort)
        for (int i = numPoints - 1; i > 0; i--) {
            if (points[i].timeOffset < points[i-1].timeOffset) {
                // Swap
                ProfilePoint temp = points[i];
                points[i] = points[i-1];
                points[i-1] = temp;
            } else {
                break;
            }
        }
        
        return true;
    }
    
    // Clear all points
    void clear() {
        numPoints = 0;
        active = false;
    }
    
    // Start the profile
    void start() {
        if (numPoints > 0) {
            active = true;
            startTime = millis();
        }
    }
    
    // Stop the profile
    void stop() {
        active = false;
    }
    
    // Get current setpoint based on elapsed time
    float getCurrentSetpoint() const {
        if (!active || numPoints == 0) return 0.0;
        
        unsigned long elapsed = millis() - startTime;
        
        // If we're before the first point, return its value
        if (elapsed < points[0].timeOffset) {
            return points[0].setValue;
        }
        
        // If we're after the last point, return its value
        if (elapsed >= points[numPoints-1].timeOffset) {
            return points[numPoints-1].setValue;
        }
        
        // Find the two surrounding points and interpolate
        for (int i = 0; i < numPoints - 1; i++) {
            if (elapsed >= points[i].timeOffset && elapsed < points[i+1].timeOffset) {
                // Linear interpolation between points
                float timeRatio = (float)(elapsed - points[i].timeOffset) / 
                                 (points[i+1].timeOffset - points[i].timeOffset);
                
                return points[i].setValue + timeRatio * (points[i+1].setValue - points[i].setValue);
            }
        }
        
        // Fallback (shouldn't happen with the checks above)
        return points[0].setValue;
    }
    
    // Check if profile is complete
    bool isComplete() const {
        if (!active || numPoints == 0) return true;
        
        unsigned long elapsed = millis() - startTime;
        return elapsed >= points[numPoints-1].timeOffset;
    }
};

// Main configuration class
class GCConfig {
private:
    // Profiles for temperature and pressure
    Profile temperatureProfile;
    Profile pressureProfile;
    
    // Configuration parameters
    float msGain;          // Gain for Mass Spectrometer signal
    float msOffset;        // Offset for Mass Spectrometer signal
    uint16_t sampleRate;   // Sample rate in milliseconds
    bool loggingEnabled;   // Enable/disable data logging
    
public:
    GCConfig();
    
    // Profile management
    Profile& getTemperatureProfile() { return temperatureProfile; }
    Profile& getPressureProfile() { return pressureProfile; }
    
    // MS configuration
    void setMSParams(float gain, float offset) {
        msGain = gain;
        msOffset = offset;
    }
    
    float getMSGain() const { return msGain; }
    float getMSOffset() const { return msOffset; }
    
    // General configuration
    void setSampleRate(uint16_t rate) { sampleRate = rate; }
    uint16_t getSampleRate() const { return sampleRate; }
    
    void setLoggingEnabled(bool enabled) { loggingEnabled = enabled; }
    bool isLoggingEnabled() const { return loggingEnabled; }
    
    // Parse a command string from the desktop software
    bool parseCommand(const char* command);
    
    // Generate a status string to send to the desktop software
    String getStatusString() const;
};

#endif // GC_CONFIG_H