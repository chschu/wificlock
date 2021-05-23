#ifndef _SCROLLER_APP_H
#define _SCROLLER_APP_H

#include <string>

#include <App.h>

class ScrollerApp : public App {
public:
    ScrollerApp(const std::string &text, unsigned long autoscroll_delay_millis);

    virtual void enter() override;
    virtual void handleKeyLeft() override;
    virtual void handleKeyRight() override;
    virtual void update(AppDisplayInterface &display) override;

private:
    const std::string _text;
    const char *_position;

    bool _autoscroll;
    unsigned long _autoscroll_delay_millis;
    unsigned long _last_autoscroll_millis;
};

#endif
