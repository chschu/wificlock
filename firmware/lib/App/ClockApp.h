#ifndef _CLOCK_APP_H
#define _CLOCK_APP_H

#include <App.h>

class ClockApp : public App {
public:
    ClockApp();

    void notifyTimeSet();
    void setTimeTrailingDot(bool time_trailing_dot);

    virtual void handleKeyLeft() override;
    virtual void handleKeyRight() override;
    virtual void update(AppDisplayInterface &display) override;

private:
    enum {
        _CLOCK_APP_MODE_TIME_NOT_SET,
        _CLOCK_APP_MODE_TIME,
        _CLOCK_APP_MODE_DATE,
        _CLOCK_APP_MODE_SECONDS
    } _mode;
    bool _time_trailing_dot;
    bool _time_blinking_colon;
};

#endif
