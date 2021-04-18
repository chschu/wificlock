#include <ctype.h>
#include <inttypes.h>

#include "SevenSegment.h"

struct mapping_t {
    char ch;
    uint16_t bits;
};

const mapping_t MAPPINGS[] = {
    { ' ', 0b00000000 },
    { '0', 0b00111111 },
    { '1', 0b00000110 }, 
    { '2', 0b01011011 },
    { '3', 0b01001111 },
    { '4', 0b01100110 },
    { '5', 0b01101101 },
    { '6', 0b01111101 },
    { '7', 0b00000111 },
    { '8', 0b01111111 },
    { '9', 0b01101111 },
    { 'A', 0b01110111 },
    { 'b', 0b01111100 },
    { 'c', 0b01011000 },
    { 'C', 0b00111001 },
    { 'd', 0b01011110 },
    { 'E', 0b01111001 },
    { 'F', 0b01110001 },
    { 'G', 0b00111101 },
    { 'h', 0b01110100 },
    { 'H', 0b01110110 },
    { 'i', 0b00000100 },
    { 'j', 0b00001100 },
    { 'J', 0b00011110 },
    { 'L', 0b00111000 },
    { 'n', 0b01010100 },
    { 'o', 0b01011100 },
    { 'P', 0b01110011 },
    { 'q', 0b01100111 },
    { 'r', 0b01010000 },
    { 'S', 0b01101101 },
    { 't', 0b01111000 },
    { 'u', 0b00011100 },
    { 'y', 0b01101110 },
    { 'Y', 0b01100110 },
    { '-', 0b01000000 },
    { '_', 0b00001000 },
    { '@', 0b01111011 },
    {   0, 0b00000000 }
};

static const mapping_t *findMapping(char ch) {
    for (const mapping_t *m = &MAPPINGS[0]; m->ch; m++) {
        if (m->ch == ch) {
            return m;
        }
    }
    return nullptr;
}

static const mapping_t *findMappingWithCaseFallback(char ch) {
    const mapping_t *m = findMapping(ch);
    if (m != nullptr) {
        return m;
    }
    char ch2 = tolower(ch);
    if (ch2 == ch) {
        ch2 = toupper(ch);
    }
    if (ch2 != ch) {
        m = findMapping(ch2);
        if (m != nullptr) {
            return m;
        }
    }
    return nullptr;
}

uint16_t SevenSegmentClass::getBits(char ch, bool case_fallback, uint16_t default_bits) {
    const mapping_t *m = case_fallback ? findMappingWithCaseFallback(ch) : findMapping(ch);
    if (m != nullptr) {
        return m->bits;
    }
    return DEFAULT_BITS;
}

SevenSegmentClass SevenSegment;