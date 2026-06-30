#include "MHZ19.h"

MHZ19::MHZ19(int rxPin, int txPin)
    : rxPin_(rxPin), txPin_(txPin), serial_(nullptr), initialized_(false) {}

MHZ19::~MHZ19() {
    if (serial_) {
        serial_->end();
        delete serial_;
    }
}

bool MHZ19::begin() {
    // Use UART2 on ESP32-S3
    serial_ = new HardwareSerial(2);
    serial_->begin(9600, SERIAL_8N1, rxPin_, txPin_);
    
    // Warm-up: give sensor time to stabilize
    delay(100);
    
    initialized_ = true;
    return true;
}

bool MHZ19::readCO2(int& co2ppm) {
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[9] = {0};
    
    if (!sendCommand(cmd[1], resp, 9)) {
        return false;
    }
    
    // Validate response
    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return false;
    }
    
    uint8_t cs = checksum(resp, 8);
    if (resp[8] != cs) {
        return false;
    }
    
    co2ppm = (resp[2] << 8) | resp[3];
    return true;
}

bool MHZ19::readTemperature(float& temp) {
    uint8_t resp[9] = {0};
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    
    if (!sendCommand(cmd[1], resp, 9)) {
        return false;
    }
    
    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return false;
    }
    
    // Temperature is in resp[4], units: °C (value - 40)
    temp = resp[4] - 40.0f;
    return true;
}

bool MHZ19::calibrateZero() {
    uint8_t cmd[] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
    uint8_t resp[9] = {0};
    return sendCommand(cmd[1], resp, 9);
}

bool MHZ19::calibrateSpan(int spanPPM) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cmd[2] = (spanPPM >> 8) & 0xFF;
    cmd[3] = spanPPM & 0xFF;
    cmd[8] = checksum(cmd, 8);
    
    uint8_t resp[9] = {0};
    return sendCommand(cmd[1], resp, 9);
}

bool MHZ19::setRange(int rangePPM) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cmd[2] = (rangePPM >> 8) & 0xFF;
    cmd[3] = rangePPM & 0xFF;
    cmd[8] = checksum(cmd, 8);
    
    uint8_t resp[9] = {0};
    return sendCommand(cmd[1], resp, 9);
}

bool MHZ19::setAutoCalibration(bool enable) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[8] = checksum(cmd, 8);
    
    uint8_t resp[9] = {0};
    return sendCommand(cmd[1], resp, 9);
}

bool MHZ19::sendCommand(uint8_t cmd, uint8_t* response, size_t respLen) {
    if (!serial_) return false;
    
    serial_->flush();
    
    // Build and send command
    uint8_t buf[9] = {0xFF, 0x01, cmd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    buf[8] = checksum(buf, 8);
    
    serial_->write(buf, 9);
    serial_->flush();
    
    // Wait for response
    unsigned long start = millis();
    size_t idx = 0;
    while (millis() - start < 1000) {
        if (serial_->available()) {
            response[idx++] = serial_->read();
            if (idx >= respLen) break;
        }
        delay(1);
    }
    
    return idx >= respLen;
}

uint8_t MHZ19::checksum(uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 1; i < len; i++) {
        sum += data[i];
    }
    return (0xFF - sum) + 1;
}
