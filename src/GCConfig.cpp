#include "GCConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parseFloatStrict(const char* str, float* outVal) {
    if (!str || *str == '\0') return false;
    char* endPtr = nullptr;
    double val = strtod(str, &endPtr);
    if (endPtr == str || *endPtr != '\0') return false;
    *outVal = (float)val;
    return true;
}

static bool parseProfileAddArgs(const char* params, unsigned long* outTimeMs, float* outValue) {
    // Expected format: ADD:<time_sec>:<value>
    if (strncmp(params, "ADD:", 4) != 0) return false;
    const char* timeStart = params + 4;
    if (*timeStart == '\0' || *timeStart == '-') return false;
    
    char* endPtr = nullptr;
    double timeSec = strtod(timeStart, &endPtr);
    if (endPtr == timeStart || *endPtr != ':' || timeSec < 0.0) return false;
    
    const char* valStart = endPtr + 1;
    float value = 0.0f;
    if (!parseFloatStrict(valStart, &value)) return false;
    
    *outTimeMs = (unsigned long)(timeSec * 1000.0 + 0.5);
    *outValue = value;
    return true;
}

GCConfig::GCConfig() {
    // Default values
    msGain = 1.0f;
    msOffset = 0.0f;
    sampleRate = 1000; // 1 second default
    loggingEnabled = true;
}

bool GCConfig::parseCommand(const char* command) {
    if (!command || *command == '\0') return false;
    
    // Allow bare STATUS or STATUS:GET
    if (strcmp(command, "STATUS") == 0 || strcmp(command, "STATUS:GET") == 0) {
        return true;
    }
    
    // Command format: CMD:ARG1:ARG2:...
    const char* firstColon = strchr(command, ':');
    if (!firstColon) return false;
    
    size_t cmdLen = (size_t)(firstColon - command);
    const char* params = firstColon + 1;
    
    if (cmdLen == 12 && strncmp(command, "TEMP_PROFILE", 12) == 0) {
        if (strcmp(params, "CLEAR") == 0) {
            temperatureProfile.clear();
            return true;
        }
        unsigned long timeOffset = 0;
        float setValue = 0.0f;
        if (!parseProfileAddArgs(params, &timeOffset, &setValue)) return false;
        return temperatureProfile.addPoint(timeOffset, setValue);
    }
    else if (cmdLen == 13 && strncmp(command, "PRESS_PROFILE", 13) == 0) {
        if (strcmp(params, "CLEAR") == 0) {
            pressureProfile.clear();
            return true;
        }
        unsigned long timeOffset = 0;
        float setValue = 0.0f;
        if (!parseProfileAddArgs(params, &timeOffset, &setValue)) return false;
        return pressureProfile.addPoint(timeOffset, setValue);
    }
    else if (cmdLen == 13 && strncmp(command, "START_PROFILE", 13) == 0) {
        if (strcmp(params, "TEMP") == 0) {
            temperatureProfile.start();
            return true;
        }
        else if (strcmp(params, "PRESS") == 0) {
            pressureProfile.start();
            return true;
        }
        else if (strcmp(params, "ALL") == 0) {
            temperatureProfile.start();
            pressureProfile.start();
            return true;
        }
    }
    else if (cmdLen == 12 && strncmp(command, "STOP_PROFILE", 12) == 0) {
        if (strcmp(params, "TEMP") == 0) {
            temperatureProfile.stop();
            return true;
        }
        else if (strcmp(params, "PRESS") == 0) {
            pressureProfile.stop();
            return true;
        }
        else if (strcmp(params, "ALL") == 0) {
            temperatureProfile.stop();
            pressureProfile.stop();
            return true;
        }
    }
    else if (cmdLen == 6 && strncmp(command, "SET_MS", 6) == 0) {
        // Format: SET_MS:gain:offset
        const char* secondColon = strchr(params, ':');
        if (!secondColon || secondColon == params) return false;
        
        char* endPtr = nullptr;
        double gain = strtod(params, &endPtr);
        if (endPtr != secondColon) return false;
        
        float offset = 0.0f;
        if (!parseFloatStrict(secondColon + 1, &offset)) return false;
        
        setMSParams((float)gain, offset);
        return true;
    }
    else if (cmdLen == 11 && strncmp(command, "SAMPLE_RATE", 11) == 0) {
        // Format: SAMPLE_RATE:rate_ms
        if (*params == '\0' || *params == '-') return false;
        char* endPtr = nullptr;
        long rate = strtol(params, &endPtr, 10);
        if (endPtr == params || *endPtr != '\0') return false;
        if (rate < 10 || rate > 60000) return false; // 10ms to 60s
        
        setSampleRate((uint16_t)rate);
        return true;
    }
    else if (cmdLen == 7 && strncmp(command, "LOGGING", 7) == 0) {
        // Format: LOGGING:ON or LOGGING:OFF
        if (strcmp(params, "ON") == 0) {
            setLoggingEnabled(true);
            return true;
        }
        else if (strcmp(params, "OFF") == 0) {
            setLoggingEnabled(false);
            return true;
        }
    }
    
    return false; // Unknown command
}

void GCConfig::formatStatusString(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize == 0) return;
    
    float tempSetpoint = temperatureProfile.active ? temperatureProfile.getCurrentSetpoint() : 0.0f;
    float pressSetpoint = pressureProfile.active ? pressureProfile.getCurrentSetpoint() : 0.0f;
    
    // Format: STATUS:temp_active:temp_setpoint:press_active:press_setpoint:ms_gain:ms_offset:sample_rate:logging
    String status = "STATUS:";
    status += temperatureProfile.active ? "1:" : "0:";
    status += temperatureProfile.active ? String(tempSetpoint, 2) : "0";
    status += ":";
    status += pressureProfile.active ? "1:" : "0:";
    status += pressureProfile.active ? String(pressSetpoint, 2) : "0";
    status += ":";
    status += String(msGain, 2) + ":" + String(msOffset, 2) + ":";
    status += String(sampleRate) + ":" + (loggingEnabled ? "1" : "0");
    
    strncpy(buffer, status.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
}

String GCConfig::getStatusString() const {
    char buffer[128];
    formatStatusString(buffer, sizeof(buffer));
    return String(buffer);
}