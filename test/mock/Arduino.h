#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define INPUT 0x0
#define OUTPUT 0x1
#define A0 54
#define A1 55
#define A2 56

unsigned long millis();
void setMockMillis(unsigned long ms);
void advanceMockMillis(unsigned long deltaMs);

void pinMode(uint8_t pin, uint8_t mode);
int analogRead(uint8_t pin);
void setMockAnalogRead(uint8_t pin, int value);
void analogWrite(uint8_t pin, int val);
int getMockAnalogWrite(uint8_t pin);

class String {
private:
    std::string data;
public:
    String() : data("") {}
    String(const char* s) : data(s ? s : "") {}
    String(const std::string& s) : data(s) {}
    String(int val) : data(std::to_string(val)) {}
    String(unsigned int val) : data(std::to_string(val)) {}
    String(long val) : data(std::to_string(val)) {}
    String(unsigned long val) : data(std::to_string(val)) {}
    String(float val, int decimals = 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        data = buf;
    }

    const char* c_str() const { return data.c_str(); }
    String& operator+=(const String& rhs) { data += rhs.data; return *this; }
    String& operator+=(const char* rhs) { if (rhs) data += rhs; return *this; }
    friend String operator+(const String& lhs, const String& rhs) { return String(lhs.data + rhs.data); }
    friend String operator+(const String& lhs, const char* rhs) { return String(lhs.data + (rhs ? rhs : "")); }
    friend String operator+(const char* lhs, const String& rhs) { return String((lhs ? lhs : "") + rhs.data); }
    bool operator==(const char* rhs) const { return data == (rhs ? rhs : ""); }
    bool operator==(const String& rhs) const { return data == rhs.data; }
};

class MockSerial {
private:
    std::string rxBuffer;
    size_t rxPos = 0;
    std::string txBuffer;
public:
    void begin(unsigned long) {}
    int available() const { return (int)(rxBuffer.size() - rxPos); }
    int read() {
        if (rxPos >= rxBuffer.size()) return -1;
        return (unsigned char)rxBuffer[rxPos++];
    }
    void feedInput(const std::string& input) {
        rxBuffer.append(input);
    }
    void clearTx() { txBuffer.clear(); }
    const std::string& getTx() const { return txBuffer; }

    void print(const char* s) { if (s) txBuffer += s; }
    void print(const String& s) { txBuffer += s.c_str(); }
    void print(uint8_t v) { txBuffer += std::to_string((unsigned int)v); }
    void print(unsigned long v) { txBuffer += std::to_string(v); }
    void print(float v, int decimals = 2) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.*f", decimals, v);
        txBuffer += buf;
    }

    void println() { txBuffer += "\n"; }
    void println(const char* s) { if (s) txBuffer += s; txBuffer += "\n"; }
    void println(const String& s) { txBuffer += s.c_str(); txBuffer += "\n"; }
    void println(float v, int decimals = 2) {
        print(v, decimals);
        txBuffer += "\n";
    }
};

extern MockSerial Serial;

#endif // ARDUINO_MOCK_H
