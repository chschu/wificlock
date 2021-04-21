#ifndef _BRIGHTNESS_APP_H
#define _BRIGHTNESS_APP_H

#include <App.h>

class BrightnessApp : public App {
public:
    BrightnessApp();

    virtual void init(AppDisplayInterface &display) override;

    virtual void handleKeyLeft() override;
    virtual void handleKeyRight() override;
    virtual void update(AppDisplayInterface &display) override;

private:
    uint8_t _brightness;
    bool _changed;

    uint8_t _getDisplayBrightnessValue();
};

#endif
