#include "SerialComm.h"

SerialComm::SerialComm(GCConfig& configRef) : config(configRef) {
    bufferPos = 0;
    lastLogTime = 0;
    sampleRate = configRef.getSampleRate();
}

void SerialComm::begin(unsigned long baudRate) {
    Serial.begin(baudRate);
    Serial.println("OpenMS Gas Chromatograph Controller v1.0");
    Serial.println("Ready for commands...");
}

void SerialComm::update() {
    // Check for incoming commands
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        // Process on newline or carriage return
        if (c == '\n' || c == '\r') {
            if (bufferPos > 0) {
                // Null-terminate the command
                cmdBuffer[bufferPos] = '\0';
                // Process the command
                processCommand();
                // Reset buffer
                bufferPos = 0;
            }
        }
        // Add character to buffer if there's space
        else if (bufferPos < MAX_COMMAND_LENGTH - 1) {
            cmdBuffer[bufferPos++] = c;
        }
        // Buffer overflow, discard command
        else {
            sendResponse(CMD_BUFFER_OVERFLOW, "Command too long");
            bufferPos = 0;
        }
    }
    
    // Update sample rate from config
    sampleRate = config.getSampleRate();
}

void SerialComm::processCommand() {
    // Echo command for debugging
    Serial.print("CMD: ");
    Serial.println(cmdBuffer);
    
    // Parse and execute command
    if (config.parseCommand(cmdBuffer)) {
        sendResponse(CMD_OK, "Command executed successfully");
    }
    else {
        sendResponse(CMD_UNKNOWN, "Unknown or invalid command");
    }
}

void SerialComm::sendResponse(uint8_t responseCode, const char* message) {
    // Format: RESP:<code>:<message>
    Serial.print("RESP:");
    Serial.print(responseCode);
    
    if (message) {
        Serial.print(":");
        Serial.println(message);
    }
    else {
        Serial.println();
    }
}

void SerialComm::logData(float temperature, float pressure, float msValue) {
    // Check if logging is enabled
    if (!config.isLoggingEnabled()) {
        return;
    }
    
    // Check sample rate timing
    unsigned long currentTime = millis();
    if (currentTime - lastLogTime < sampleRate) {
        return;
    }
    
    lastLogTime = currentTime;
    
    // Format: DATA:<timestamp>:<temperature>:<pressure>:<ms_value>
    Serial.print("DATA:");
    Serial.print(currentTime);
    Serial.print(":");
    Serial.print(temperature, 2); // 2 decimal places
    Serial.print(":");
    Serial.print(pressure, 2); // 2 decimal places
    Serial.print(":");
    Serial.println(msValue, 4); // 4 decimal places for MS value
}

void SerialComm::sendStatus() {
    // Send current status string
    Serial.println(config.getStatusString());
}

bool SerialComm::hasCommand() {
    return bufferPos > 0;
}