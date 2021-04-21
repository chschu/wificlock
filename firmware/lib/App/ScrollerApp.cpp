#include <Arduino.h>

#include <string>

#include <ScrollerApp.h>

ScrollerApp::ScrollerApp(const std::string &text, unsigned long autoscroll_delay_millis) : _text(text), _autoscroll_delay_millis(autoscroll_delay_millis) {
}

void ScrollerApp::enter() {
    _position = _text.c_str();
    _last_autoscroll_millis = millis();
    _autoscroll = _autoscroll_delay_millis > 0;
}

void ScrollerApp::handleKeyLeft() {
    _autoscroll = false;
    if (--_position < _text.c_str()) {
        _position += _text.length();
    }
}

void ScrollerApp::handleKeyRight() {
    _autoscroll = false;
    if (!*++_position) {
        _position = _text.c_str();
    }
}

void ScrollerApp::update(AppDisplayInterface &display) {
    if (*_position) {
        if (_autoscroll) {
            unsigned long cur_millis = millis();
            if (cur_millis - _last_autoscroll_millis > _autoscroll_delay_millis) {
                if (!*++_position) {
                    _position = _text.c_str();
                }
                _last_autoscroll_millis = cur_millis;
            }
        }
        const char *p = _position;
        for (uint8_t i = 0; i < 4; i++) {
            display.setChar(i, *p, false, true);
            if (!*++p) {
                p = _text.c_str();
            }
        }
    }
}
