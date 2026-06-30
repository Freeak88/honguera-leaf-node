// MH-Z19B CO2 Sensor Driver for ESP32
// UART communication with MH-Z19B NDIR CO2 sensor

#ifndef MHZ19_H
#define MHZ19_H

#include <Arduino.h>

class MHZ19 {
public:
    MHZ19(int rxPin, int txPin);
    ~MHZ19();

    bool begin();
    bool readCO2(int& co2ppm);
    bool readTemperature(float& temp);
    bool calibrateZero();
    bool calibrateSpan(int spanPPM);
    bool setRange(int rangePPM);
    bool setAutoCalibration(bool enable);

private:
    int rxPin_, txPin_;
    HardwareSerial* serial_;
    bool initialized_;

    bool sendCommand(uint8_t cmd, uint8_t* response, size_t respLen);
    uint8_t checksum(uint8_t* data, size_t len);
};

#endif // MHZ19_H
