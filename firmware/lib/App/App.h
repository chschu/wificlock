#ifndef _APP_H
#define _APP_H

#include <inttypes.h>

class AppDisplayInterface {
public:
    virtual void setBrightness(uint8_t brightness) = 0;
    virtual void setChar(uint8_t digit, char ch, bool dot = false, bool case_fallback = false) = 0;
    virtual void setColon(bool colon) = 0;
};

class App {
public:
    virtual void init(AppDisplayInterface &display) {
    }

    virtual void enter() {
    }

    virtual void handleKeyLeft() {
    }

    virtual void handleKeyRight() {
    }

    virtual void update(AppDisplayInterface &display) = 0;
};

#endif
