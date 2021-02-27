#include <HT16K33.h>
#include <Wire.h>

HT16K33::HT16K33(uint8_t addr)
    : _addr(addr), _ledMem { 0, 0, 0, 0, 0, 0, 0, 0 }, _ledMemDirty(false), _keyMem { 0, 0, 0 } {
}

static void i2c_write(uint8_t addr, uint8_t data) {
    Wire.beginTransmission(addr);
    Wire.write(data);
    Wire.endTransmission();
}

static void i2c_write(uint8_t addr, uint8_t *data, size_t num) {
    Wire.beginTransmission(addr);
    for (size_t i = 0; i < num; i++) {
        Wire.write(data[i]);
    }
    Wire.endTransmission();
}

static void i2c_read(uint8_t addr, uint8_t *data, size_t num) {
    Wire.requestFrom(addr, 6);
    for (size_t i = 0; i < num && Wire.available(); i++) {
        data[i] = Wire.read();
    }
}

void HT16K33::begin() {
    // turn on oscillator
    i2c_write(_addr, 0x21);

    // set full brightness
    setBrightness(15);

    // clear data and force update
    for (uint8_t column = 0; column < 8; column++) {
        setLedColumn(column, 0);
    }
    updateLeds(true);

    // initialize key memory
    updateKeys();
    
    // turn on display, disable blinking
    i2c_write(_addr, 0x81);
}

void HT16K33::setBrightness(uint8_t brightness) {
    i2c_write(_addr, 0xE0 | (brightness & 0x0F));
}

void HT16K33::updateLeds(bool force) {
    // update LEDs
    if (_ledMemDirty || force) {
        uint8_t tmp[17];
        size_t num = 0;
        tmp[num++] = 0x00;
        for (uint8_t i = 0; i < 8; i++) {
            tmp[num++] = _ledMem[i] & 0xFF;
            tmp[num++] = _ledMem[i] >> 8;
        }

        i2c_write(_addr, tmp, num);

        _ledMemDirty = false;
    }
}

void HT16K33::updateKeys() {
    i2c_write(_addr, 0x40);

    uint8_t tmp[3] = { 0, 0, 0 };
    i2c_read(_addr, tmp, 6);

    for (int i = 0; i < 3; i++) {
        _keyMem[i] = (tmp[2 * i + 1] << 8) | tmp[2 * i];
    }
}

void HT16K33::setLedColumn(uint8_t column, uint16_t rowBits) {
    if (_ledMem[column] != rowBits) {
        _ledMem[column] = rowBits;
        _ledMemDirty = true;
    }
}

uint16_t HT16K33::getKeyColumn(uint8_t column) {
    return _keyMem[column];
}