#include <Arduino.h>

#include <HT16K33.h>
#include <Wire.h>

HT16K33::HT16K33(uint8_t addr)
    : _addr(addr), _led_mem { 0, 0, 0, 0, 0, 0, 0, 0 }, _led_next_mem { 0, 0, 0, 0, 0, 0, 0, 0 }, _key_mem { 0, 0, 0 } {
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

static size_t i2c_read(uint8_t addr, uint8_t *data, size_t num) {
    size_t actual_num = Wire.requestFrom(addr, num, true);
    for (size_t i = 0; i < actual_num; i++) {
        data[i] = Wire.read();
    }
    return actual_num;
}

void HT16K33::begin() {
    // datasheet, page 8: "Data transfers on the I2C-bus should be avoided for 1 ms following a power-on to allow completion of the reset action."
    delay(1);

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
    if (force || memcmp(_led_mem, _led_next_mem, sizeof(_led_mem))) {
        memcpy(_led_mem, _led_next_mem, sizeof(_led_mem));

        uint8_t tmp[17];
        size_t num = 0;
        tmp[num++] = 0x00;
        for (uint8_t i = 0; i < 8; i++) {
            tmp[num++] = _led_mem[i] & 0xFF;
            tmp[num++] = _led_mem[i] >> 8;
        }

        i2c_write(_addr, tmp, num);
    }
}

void HT16K33::updateKeys() {
    // wait for at least two cycles (2 * 9.504ms) to make sure key scanning hast been performed since the last read
    delay(20);

    i2c_write(_addr, 0x40);
    uint8_t tmp[6] = { 0, 0, 0, 0, 0, 0 };
    i2c_read(_addr, tmp, 6);

    for (int i = 0; i < 3; i++) {
        _key_mem[i] = (tmp[2 * i + 1] << 8) | tmp[2 * i];
    }
}

void HT16K33::setLedColumn(uint8_t column, uint16_t row_bits) {
    _led_next_mem[column] = row_bits;
}

void HT16K33::clearAllLedColumns() {
    memset(_led_next_mem, 0, sizeof(_led_next_mem));
}

uint16_t HT16K33::getKeyColumn(uint8_t column) {
    return _key_mem[column];
}
