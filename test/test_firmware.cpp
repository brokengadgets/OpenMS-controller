#include "Arduino.h"
#include "PIDController.h"
#include "GCConfig.h"
#include "AnalogIO.h"
#include "SerialComm.h"
#include <assert.h>
#include <math.h>
#include <iostream>

static unsigned long g_mockMillis = 0;
static int g_analogInputs[256] = {0};
static int g_analogOutputs[256] = {0};
MockSerial Serial;

unsigned long millis() { return g_mockMillis; }
void setMockMillis(unsigned long ms) { g_mockMillis = ms; }
void advanceMockMillis(unsigned long deltaMs) { g_mockMillis += deltaMs; }
void pinMode(uint8_t, uint8_t) {}
int analogRead(uint8_t pin) { return g_analogInputs[pin]; }
void setMockAnalogRead(uint8_t pin, int value) { g_analogInputs[pin] = value; }
void analogWrite(uint8_t pin, int val) { g_analogOutputs[pin] = val; }
int getMockAnalogWrite(uint8_t pin) { return g_analogOutputs[pin]; }

static bool approxEqual(float a, float b, float eps = 1e-2f) {
    return fabsf(a - b) <= eps;
}

static void testPIDController() {
    setMockMillis(1000);
    PIDController pid(2.0f, 0.5f, 0.1f, 0.0f, 100.0f);
    pid.setSetpoint(50.0f);

    // First step at dt=0: proportional only, no derivative kick
    float out0 = pid.compute(40.0f);
    assert(approxEqual(out0, 20.0f));
    assert(approxEqual(pid.getLastOutput(), 20.0f));
    assert(approxEqual(pid.getCurrentError(), 10.0f));

    // Step forward 1s with measurement rising from 40 to 42
    advanceMockMillis(1000);
    float out1 = pid.compute(42.0f);
    // error = 8 -> P = 16, I = 0.5 * 8 * 1 = 4, D = -0.1 * (42 - 40) / 1 = -0.2 -> total = 19.8
    assert(approxEqual(out1, 19.8f));
    assert(approxEqual(pid.getLastOutput(), 19.8f));

    // Changing setpoint does not cause derivative kick when input is constant
    pid.setSetpoint(60.0f);
    advanceMockMillis(1000);
    float out2 = pid.compute(42.0f);
    // error = 18 -> P = 36, I = 4 + 0.5*18*1 = 13, D = 0 -> total = 49.0
    assert(approxEqual(out2, 49.0f));

    // Reset clears integral and lastOutput
    pid.reset();
    assert(approxEqual(pid.getLastOutput(), 0.0f));
}

static void testGCConfigAndProfiles() {
    setMockMillis(0);
    GCConfig cfg;

    // Bare STATUS and STATUS:GET commands succeed
    assert(cfg.parseCommand("STATUS"));
    assert(cfg.parseCommand("STATUS:GET"));

    // Invalid commands fail cleanly
    assert(!cfg.parseCommand(""));
    assert(!cfg.parseCommand("INVALID"));
    assert(!cfg.parseCommand("TEMP_PROFILE:ADD:-5:100"));
    assert(!cfg.parseCommand("TEMP_PROFILE:ADD:abc:100"));
    assert(!cfg.parseCommand("SAMPLE_RATE:5")); // below 10ms min

    // Add out-of-order profile points and duplicate timestamp update
    assert(cfg.parseCommand("TEMP_PROFILE:ADD:10:100.0"));
    assert(cfg.parseCommand("TEMP_PROFILE:ADD:0:50.0"));
    assert(cfg.parseCommand("TEMP_PROFILE:ADD:10:150.0")); // updates t=10s to 150.0
    assert(cfg.getTemperatureProfile().numPoints == 2);

    assert(cfg.parseCommand("START_PROFILE:TEMP"));
    assert(cfg.getTemperatureProfile().active);
    assert(approxEqual(cfg.getTemperatureProfile().getCurrentSetpoint(), 50.0f));

    // Midpoint at 5000ms -> 100.0°C
    setMockMillis(5000);
    assert(approxEqual(cfg.getTemperatureProfile().getCurrentSetpoint(), 100.0f));
    assert(!cfg.getTemperatureProfile().isComplete());

    // Endpoint at 10000ms -> 150.0°C and complete
    setMockMillis(10000);
    assert(approxEqual(cfg.getTemperatureProfile().getCurrentSetpoint(), 150.0f));
    assert(cfg.getTemperatureProfile().isComplete());

    // MS and sample rate config
    assert(cfg.parseCommand("SET_MS:1.5:0.25"));
    assert(approxEqual(cfg.getMSGain(), 1.5f));
    assert(approxEqual(cfg.getMSOffset(), 0.25f));
    assert(cfg.parseCommand("SAMPLE_RATE:250"));
    assert(cfg.getSampleRate() == 250);
    assert(cfg.parseCommand("LOGGING:OFF"));
    assert(!cfg.isLoggingEnabled());
}

static void testAnalogIO() {
    AnalogIO io(A0, A1, A2, 3, 5, 6);
    io.begin();

    // Half-scale ADC (511.5 / 1023 -> ~2.5V):
    // Default temp: 0-250°C -> ~125°C; Default pressure: 0-100 PSI -> ~50 PSI; Default MS: ~2.5V
    setMockAnalogRead(A0, 512);
    setMockAnalogRead(A1, 512);
    setMockAnalogRead(A2, 512);
    io.readSensors();

    assert(io.getTemperature() > 124.0f && io.getTemperature() < 126.0f);
    assert(io.getPressure() > 49.5f && io.getPressure() < 50.5f);
    assert(io.getMSValue() > 2.49f && io.getMSValue() < 2.51f);

    // Actuator clamping and PWM conversion
    io.setHeaterOutput(50.0f);
    assert(io.getHeaterPWM() == 127);
    io.setHeaterOutput(150.0f);
    assert(io.getHeaterPWM() == 255);
    io.setHeaterOutput(-10.0f);
    assert(io.getHeaterPWM() == 0);
}

static void testSerialComm() {
    setMockMillis(1000);
    GCConfig cfg;
    SerialComm comm(cfg);
    Serial.clearTx();
    comm.begin(115200);
    Serial.clearTx();

    // Test STATUS command emits STATUS:... followed by RESP:0:...
    Serial.feedInput("STATUS\n");
    comm.update();
    std::string tx = Serial.getTx();
    assert(tx.find("CMD: STATUS\n") != std::string::npos);
    assert(tx.find("STATUS:0:0:0:0:1.00:0.00:1000:1\n") != std::string::npos);
    assert(tx.find("RESP:0:Command executed successfully\n") != std::string::npos);

    // Test oversized command overflow recovery (tail after 128 chars must not execute as a command)
    Serial.clearTx();
    std::string longCmd(140, 'A');
    longCmd += "STATUS\n";
    Serial.feedInput(longCmd);
    comm.update();
    tx = Serial.getTx();
    assert(tx.find("RESP:3:Command too long\n") != std::string::npos);
    assert(tx.find("CMD: STATUS") == std::string::npos);

    // Next valid command after newline succeeds normally
    Serial.clearTx();
    Serial.feedInput("LOGGING:OFF\n");
    comm.update();
    tx = Serial.getTx();
    assert(tx.find("RESP:0:Command executed successfully\n") != std::string::npos);
    assert(!cfg.isLoggingEnabled());
}

int main() {
    testPIDController();
    testGCConfigAndProfiles();
    testAnalogIO();
    testSerialComm();
    std::cout << "All firmware unit tests passed!" << std::endl;
    return 0;
}
