#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <Arduino.h>
#include "GCConfig.h"

// Maximum command length
#define MAX_COMMAND_LENGTH 128

// Command response codes
#define CMD_OK 0
#define CMD_UNKNOWN 1
#define CMD_INVALID_PARAM 2
#define CMD_BUFFER_OVERFLOW 3

class SerialComm {
private:
    // Command buffer
    char cmdBuffer[MAX_COMMAND_LENGTH];
    uint8_t bufferPos;
    
    // Configuration reference
    GCConfig& config;
    
    // Command processing
    void processCommand();
    void sendResponse(uint8_t responseCode, const char* message = nullptr);
    
    // Data logging
    unsigned long lastLogTime;
    uint16_t sampleRate;
    
public:
    SerialComm(GCConfig& configRef);
    
    // Initialization
    void begin(unsigned long baudRate = 115200);
    
    // Update function (call in loop)
    void update();
    
    // Log data to serial
    void logData(float temperature, float pressure, float msValue);
    
    // Send status update
    void sendStatus();
    
    // Check if there are pending commands
    bool hasCommand();
};

#endif // SERIAL_COMM_H