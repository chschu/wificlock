#ifndef _SEVEN_SEGMENT_H
#define _SEVEN_SEGMENT_H

class SevenSegmentClass {
public:
    uint16_t getBits(char ch, bool case_fallback = false, uint16_t default_bits = DEFAULT_BITS);

private:
    static const uint16_t DEFAULT_BITS = 0b00001000;
};

extern SevenSegmentClass SevenSegment;

#endif
