#ifndef _HT16K33_H
#define _HT16K33_H

#include <Wire.h>

class HT16K33 {
public:
    HT16K33(uint8_t addr = 0x70);

    void begin();

    void setBrightness(uint8_t brightness);

    void clear();
    void set(uint8_t column, uint16_t rowBits);

    void flush(bool force = false);

private:
    uint8_t _addr;
    uint16_t _data[8];
    bool _dirty;
};

#endif