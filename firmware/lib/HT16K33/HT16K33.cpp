#include <HT16K33.h>
#include <Wire.h>

HT16K33::HT16K33(uint8_t addr)
    : _addr(addr), _data { 0, 0, 0, 0, 0, 0, 0, 0 }, _dirty(false) {
}

void HT16K33::begin() {
    // turn on oscillator
    Wire.beginTransmission(_addr);
    Wire.write(0x21);
    Wire.endTransmission();

    // set full brightness
    setBrightness(15);

    // clear data and force flush
    clear();
    flush(true);
    
    // turn on display, disable blinking
    Wire.beginTransmission(_addr);
    Wire.write(0x81);
    Wire.endTransmission();
}

void HT16K33::setBrightness(uint8_t brightness) {
    Wire.beginTransmission(_addr);
    Wire.write(0xE0 | (brightness & 0x0F));
    Wire.endTransmission();
}

void HT16K33::clear() {
    for (uint8_t column = 0; column < 8; column++) {
        set(column, 0);
    }
}

void HT16K33::set(uint8_t column, uint16_t rowBits) {
    if (_data[column] != rowBits) {
        _data[column] = rowBits;
        _dirty = true;
    }
}

void HT16K33::flush(bool force) {
    if (_dirty || force) {
        Wire.beginTransmission(_addr);
        Wire.write(0);
        for (uint8_t i = 0; i < 8; i++) {
            Wire.write(_data[i] & 0xFF);
            Wire.write(_data[i] >> 8);
        }
        Wire.endTransmission();
        _dirty = false;
    }
}
