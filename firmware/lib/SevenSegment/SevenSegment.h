#ifndef _SEVEN_SEGMENT_H
#define _SEVEN_SEGMENT_H

class SevenSegmentClass {
public:
    uint16_t getBits(char ch, bool caseFallback = false, uint16_t defaultBits = DEFAULT_BITS);

private:
    static const uint16_t DEFAULT_BITS = 0b00001000;
};

extern SevenSegmentClass SevenSegment;

#endif
