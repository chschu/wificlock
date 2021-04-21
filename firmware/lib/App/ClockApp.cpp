#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <ClockApp.h>

ClockApp::ClockApp() : _mode(_CLOCK_APP_MODE_TIME_NOT_SET), _time_trailing_dot(false), _time_blinking_colon(true) {
}

void ClockApp::notifyTimeSet() {
    _mode = _CLOCK_APP_MODE_TIME;
}

void ClockApp::setTimeTrailingDot(bool time_trailing_dot) {
    _time_trailing_dot = time_trailing_dot;
}

void ClockApp::handleKeyLeft() {
    switch (_mode) {
    case _CLOCK_APP_MODE_TIME_NOT_SET:
    case _CLOCK_APP_MODE_TIME:
        _time_blinking_colon = !_time_blinking_colon;
        break;
    case _CLOCK_APP_MODE_DATE:
    case _CLOCK_APP_MODE_SECONDS:
        break;
    }
}

void ClockApp::handleKeyRight() {
    switch (_mode) {
    case _CLOCK_APP_MODE_TIME_NOT_SET:
        break;
    case _CLOCK_APP_MODE_TIME:
        _mode = _CLOCK_APP_MODE_DATE;
        break;
    case _CLOCK_APP_MODE_DATE:
        _mode = _CLOCK_APP_MODE_SECONDS;
        break;
    case _CLOCK_APP_MODE_SECONDS:
        _mode = _CLOCK_APP_MODE_TIME;
        break;
    }
}

void ClockApp::update(AppDisplayInterface &display) {
    timeval now;
    gettimeofday(&now, nullptr);

    tm local;
    localtime_r(&now.tv_sec, &local);

    char chars[5] = "    ";
    bool dots[4] = { false, false, false, false };
    bool colon = false;

    switch (_mode) {
    case _CLOCK_APP_MODE_TIME:
        snprintf_P(chars, sizeof(chars), PSTR("%2d%02d"), local.tm_hour, local.tm_min);
    case _CLOCK_APP_MODE_TIME_NOT_SET:
        colon = !_time_blinking_colon || now.tv_usec < 500000;
        dots[3] = _time_trailing_dot;
        break;
    case _CLOCK_APP_MODE_DATE:
        snprintf_P(chars, sizeof(chars), PSTR("%02d%02d"), local.tm_mday, local.tm_mon + 1);
        dots[1] = true;
        dots[3] = true;
        break;
    case _CLOCK_APP_MODE_SECONDS:
        snprintf_P(chars, sizeof(chars), PSTR("  %02d"), local.tm_sec);
        colon = now.tv_usec < 500000;
        break;
    }

    for (uint8_t i = 0; i < 4; i++) {
        display.setChar(i, chars[i], dots[i]);
    }
    display.setColon(colon);
}
