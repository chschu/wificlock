#ifndef _HT16K33_H
#define _HT16K33_H

#include <Wire.h>

class HT16K33 {
public:
    HT16K33(uint8_t addr = 0x70);

    void begin();

    void setBrightness(uint8_t brightness);

    // updates key memory from HT16K33
    void updateKeys();
    // writes LED memory to HT16K33
    void updateLeds(bool force = false);

    void setLedColumn(uint8_t column, uint16_t row_bits);

    uint16_t getKeyColumn(uint8_t column);

private:
    uint8_t _addr;
    uint16_t _led_mem[8];
    bool _led_mem_dirty;

    uint16_t _key_mem[3];
};

#endif