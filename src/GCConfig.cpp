#include "GCConfig.h"

GCConfig::GCConfig() {
    // Default values
    msGain = 1.0;
    msOffset = 0.0;
    sampleRate = 1000; // 1 second default
    loggingEnabled = true;
}

bool GCConfig::parseCommand(const char* command) {
    // Command format: CMD:ARG1:ARG2:...
    String cmd(command);
    
    // Find command type
    int firstColon = cmd.indexOf(':');
    if (firstColon < 0) return false; // No command separator
    
    String cmdType = cmd.substring(0, firstColon);
    String params = cmd.substring(firstColon + 1);
    
    // Process different command types
    if (cmdType.equals("TEMP_PROFILE")) {
        // Format: TEMP_PROFILE:CLEAR or TEMP_PROFILE:ADD:time:value
        if (params.equals("CLEAR")) {
            temperatureProfile.clear();
            return true;
        }
        
        // Parse ADD command
        int secondColon = params.indexOf(':');
        int thirdColon = params.indexOf(':', secondColon + 1);
        
        if (secondColon < 0 || thirdColon < 0) return false;
        
        String action = params.substring(0, secondColon);
        if (!action.equals("ADD")) return false;
        
        // Extract time and value
        String timeStr = params.substring(secondColon + 1, thirdColon);
        String valueStr = params.substring(thirdColon + 1);
        
        unsigned long timeOffset = timeStr.toInt() * 1000; // Convert seconds to ms
        float setValue = valueStr.toFloat();
        
        return temperatureProfile.addPoint(timeOffset, setValue);
    }
    else if (cmdType.equals("PRESS_PROFILE")) {
        // Format: PRESS_PROFILE:CLEAR or PRESS_PROFILE:ADD:time:value
        if (params.equals("CLEAR")) {
            pressureProfile.clear();
            return true;
        }
        
        // Parse ADD command
        int secondColon = params.indexOf(':');
        int thirdColon = params.indexOf(':', secondColon + 1);
        
        if (secondColon < 0 || thirdColon < 0) return false;
        
        String action = params.substring(0, secondColon);
        if (!action.equals("ADD")) return false;
        
        // Extract time and value
        String timeStr = params.substring(secondColon + 1, thirdColon);
        String valueStr = params.substring(thirdColon + 1);
        
        unsigned long timeOffset = timeStr.toInt() * 1000; // Convert seconds to ms
        float setValue = valueStr.toFloat();
        
        return pressureProfile.addPoint(timeOffset, setValue);
    }
    else if (cmdType.equals("START_PROFILE")) {
        // Format: START_PROFILE:TEMP or START_PROFILE:PRESS or START_PROFILE:ALL
        if (params.equals("TEMP")) {
            temperatureProfile.start();
            return true;
        }
        else if (params.equals("PRESS")) {
            pressureProfile.start();
            return true;
        }
        else if (params.equals("ALL")) {
            temperatureProfile.start();
            pressureProfile.start();
            return true;
        }
    }
    else if (cmdType.equals("STOP_PROFILE")) {
        // Format: STOP_PROFILE:TEMP or STOP_PROFILE:PRESS or STOP_PROFILE:ALL
        if (params.equals("TEMP")) {
            temperatureProfile.stop();
            return true;
        }
        else if (params.equals("PRESS")) {
            pressureProfile.stop();
            return true;
        }
        else if (params.equals("ALL")) {
            temperatureProfile.stop();
            pressureProfile.stop();
            return true;
        }
    }
    else if (cmdType.equals("SET_MS")) {
        // Format: SET_MS:gain:offset
        int secondColon = params.indexOf(':');
        if (secondColon < 0) return false;
        
        String gainStr = params.substring(0, secondColon);
        String offsetStr = params.substring(secondColon + 1);
        
        float gain = gainStr.toFloat();
        float offset = offsetStr.toFloat();
        
        setMSParams(gain, offset);
        return true;
    }
    else if (cmdType.equals("SAMPLE_RATE")) {
        // Format: SAMPLE_RATE:rate_ms
        int rate = params.toInt();
        if (rate < 10) return false; // Minimum 10ms to prevent overloading
        
        setSampleRate(rate);
        return true;
    }
    else if (cmdType.equals("LOGGING")) {
        // Format: LOGGING:ON or LOGGING:OFF
        if (params.equals("ON")) {
            setLoggingEnabled(true);
            return true;
        }
        else if (params.equals("OFF")) {
            setLoggingEnabled(false);
            return true;
        }
    }
    
    return false; // Unknown command
}

String GCConfig::getStatusString() const {
    // Format: STATUS:temp_active:temp_setpoint:press_active:press_setpoint:ms_gain:ms_offset:sample_rate:logging
    String status = "STATUS:";
    
    // Temperature status
    status += temperatureProfile.active ? "1:" : "0:";
    // If active, get the current setpoint, otherwise 0
    status += temperatureProfile.active ? String(temperatureProfile.getCurrentSetpoint()) : "0";
    status += ":";
    
    // Pressure status
    status += pressureProfile.active ? "1:" : "0:";
    // If active, get the current setpoint, otherwise 0
    status += pressureProfile.active ? String(pressureProfile.getCurrentSetpoint()) : "0";
    status += ":";
    
    // MS parameters
    status += String(msGain) + ":" + String(msOffset) + ":";
    
    // Sample rate and logging
    status += String(sampleRate) + ":" + (loggingEnabled ? "1" : "0");
    
    return status;
}