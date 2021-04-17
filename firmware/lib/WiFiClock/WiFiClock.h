#ifndef _WIFICLOCK_H
#define _WIFICLOCK_H

#include <time.h>

#include <map>
#include <vector>
#include <functional>

#include <HT16K33.h>

class WiFiClock {
public:
    typedef std::function<void(uint8_t key)> KeyHandlerFn;

    WiFiClock(HT16K33 &display);

    void addKeyHandler(uint8_t key, KeyHandlerFn handler);

    void modeNoTime();
    void modeScroller(const char *text, unsigned long delay_millis);
    void modeTime();
    void modeDate();
    void modeSeconds();

    void setWiFiConnected(bool connected);

    void update();

private:
    HT16K33 &_display;

    std::multimap<std::pair<uint8_t, uint8_t>, KeyHandlerFn> _key_handlers;

    enum {
        _CLOCK_MODE_NO_TIME,
        _CLOCK_MODE_SCROLLER,
        _CLOCK_MODE_TIME,
        _CLOCK_MODE_DATE,
        _CLOCK_MODE_SECONDS,
    } _mode;

    bool _wifi_connected;

    const char *_scroller_text;
    unsigned long _scroller_delay_millis;
    size_t _scroller_len;
    size_t _scroller_pos;
    unsigned long _scroller_last_move_millis;
};

#endif