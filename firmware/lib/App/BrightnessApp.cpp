#include <BrightnessApp.h>

BrightnessApp::BrightnessApp() :  _brightness(4), _changed(true) {
}

void BrightnessApp::init(AppDisplayInterface &display) {
    display.setBrightness(_getDisplayBrightnessValue());
    _changed = false;
}

void BrightnessApp::handleKeyLeft() {
    _brightness = _brightness > 1 ? _brightness - 1 : 1;
    _changed = true;
}

void BrightnessApp::handleKeyRight() {;
    _brightness = _brightness < 4 ? _brightness + 1 : 4;
    _changed = true;
}

void BrightnessApp::update(AppDisplayInterface &display) {
    if (_changed) {
        display.setBrightness(_getDisplayBrightnessValue());
    }
    display.setChar(0, 'B', false, true);
    display.setChar(1, 'R', false, true);
    display.setChar(2, ' ');
    display.setChar(3, '0' + _brightness);
    display.setColon(true);
}

uint8_t BrightnessApp::_getDisplayBrightnessValue() {
    // 0, 3, 8 and 15 give somewhat uniform perceived brightness steps
    return _brightness * _brightness - 1;
}
