#include <Arduino.h>

#include <stdio.h>
#include <sys/time.h>

#include <SevenSegment.h>

#include <WiFiClock.h>

WiFiClock::WiFiClock(HT16K33 &display) : _display(display), _mode(_CLOCK_MODE_NO_TIME) {
}

void WiFiClock::addKeyHandler(uint8_t key, KeyHandlerFn handler) {
    _key_handlers.insert(std::make_pair(std::make_pair(key / 13, key % 10), handler));
}

void WiFiClock::modeNoTime() {
    _mode = _CLOCK_MODE_NO_TIME;
}

void WiFiClock::modeScroller(const char *text, unsigned long delay_millis) {
    _mode = _CLOCK_MODE_SCROLLER;
    _scroller_text = text;
    _scroller_delay_millis = delay_millis;
    _scroller_len = strlen(_scroller_text);
    _scroller_pos = 0;
    _scroller_last_move_millis = millis();
}

void WiFiClock::modeTime() {
    _mode = _CLOCK_MODE_TIME;
}

void WiFiClock::modeDate() {
    _mode = _CLOCK_MODE_DATE;
}

void WiFiClock::modeSeconds() {
    _mode = _CLOCK_MODE_SECONDS;
}

void WiFiClock::setWiFiConnected(bool connected) {
    _wifi_connected = connected;
}

void WiFiClock::update() {
    uint16_t keys_old[3];
    for (uint8_t col = 0; col < 3; col++) {
        keys_old[col] = _display.getKeyColumn(col);
    }

    _display.updateKeys();

    uint16_t keys_new[3];
    for (uint8_t col = 0; col < 3; col++) {
        keys_new[col] = _display.getKeyColumn(col);
    }

    for (auto mapping : _key_handlers) {
        uint8_t col = mapping.first.first;
        uint8_t row = mapping.first.second;
        KeyHandlerFn keyHandlerFn = mapping.second;
        if ((keys_new[col] & (1 << row)) && !(keys_old[col] & (1 << row))) {
            keyHandlerFn(13 * col + row);
        }
    }

    timeval now;
    gettimeofday(&now, nullptr);

    tm local;
    localtime_r(&now.tv_sec, &local);

    char digits[5] = "    ";
    bool dots[4] = { false, false, false, false };
    bool colon = false;

    unsigned long cur_millis = millis();

    switch (_mode) {
        case _CLOCK_MODE_NO_TIME:
            dots[3] = _wifi_connected;
            colon = now.tv_usec < 500000;
            break;
        case _CLOCK_MODE_SCROLLER:
            if (cur_millis - _scroller_last_move_millis > _scroller_delay_millis) {
                if (++_scroller_pos == _scroller_len) {
                    _scroller_pos = 0;
                }
                _scroller_last_move_millis = cur_millis;
            }
            {
                size_t p = _scroller_pos;
                for (uint8_t i = 0; i < 4; i++) {
                    digits[i] = _scroller_text[p];
                    if (++p == _scroller_len) {
                        p = 0;
                    }
                }
            }
            break;
        case _CLOCK_MODE_TIME:
            snprintf_P(digits, sizeof(digits), PSTR("%2d%02d"), local.tm_hour, local.tm_min);
            dots[3] = _wifi_connected;
            colon = now.tv_usec < 500000;
            break;
        case _CLOCK_MODE_DATE:
            snprintf_P(digits, sizeof(digits), PSTR("%02d%02d"), local.tm_mday, local.tm_mon + 1);
            dots[1] = true;
            dots[3] = true;
            break;
        case _CLOCK_MODE_SECONDS:
            snprintf_P(digits, sizeof(digits), PSTR("  %02d"), local.tm_sec);
            colon = now.tv_usec < 500000;
            break;
    }

    for (uint8_t i = 0; i < 4; i++) {
        _display.setLedColumn(i, SevenSegment.getBits(digits[i]) | (dots[i] << 7));
    }
    _display.setLedColumn(4, colon);

    _display.updateLeds();
}

