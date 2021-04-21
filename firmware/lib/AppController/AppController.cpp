#include <Arduino.h>

#include <list>
#include <memory>
#include <algorithm>

#include <App.h>
#include <AppController.h>
#include <HT16K33.h>
#include <SevenSegment.h>

AppController::AppController(HT16K33 &display) : _display(display), _current_app(_apps.begin()) {
}

void AppController::addApp(std::shared_ptr<App> app) {
    _apps.push_back(app);
    app->init(*this);

    // if this is the first app, notify it that it has been entered
    if (_current_app == _apps.end()) {
        _current_app = _apps.begin();
        app->enter();
    }
}

void AppController::removeApp(std::shared_ptr<App> app) {
    auto found = std::find(_apps.begin(), _apps.end(), app);

    if (_current_app == found) {
        _switchToNextApp();
        if (_current_app == found) {
            // last app will be deleted
            _current_app = _apps.end();
        }
    }

    _apps.erase(found);
}

void AppController::update() {
    uint16_t keys_old = _display.getKeyColumn(0);

    _display.updateKeys();

    uint16_t keys_new = _display.getKeyColumn(0);

    if (_current_app != _apps.end()) {
        if ((keys_new & 2) && !(keys_old & 2)) {
            _switchToNextApp();
        }

        if ((keys_new & 4) && !(keys_old & 4)) {
            (*_current_app)->handleKeyLeft();
        }

        if ((keys_new & 1) && !(keys_old & 1)) {
            (*_current_app)->handleKeyRight();
        }

        _display.clearAllLedColumns();

        (*_current_app)->update(*this);

    } else {
        _display.clearAllLedColumns();
    }

    _display.updateLeds();
}

void AppController::setBrightness(uint8_t brightness) {
    _display.setBrightness(brightness);
}

void AppController::setChar(uint8_t digit, char ch, bool dot, bool case_fallback) {
    _display.setLedColumn(digit, SevenSegment.getBits(ch, case_fallback) | (dot << 7));
}

void AppController::setColon(bool colon) {
    _display.setLedColumn(4, colon);
}

void AppController::_switchToNextApp() {
    if (_current_app != _apps.end()) {
        auto prev = _current_app;
        if (++_current_app == _apps.end()) {
            _current_app = _apps.begin();
        }
        if (_current_app != prev) {
            (*_current_app)->enter();
        }
    }
}
