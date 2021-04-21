#ifndef _APP_CONTROLLER_H
#define _APP_CONTROLLER_H

#include <Arduino.h>

#include <list>
#include <memory>
#include <algorithm>

#include <App.h>
#include <HT16K33.h>

class AppController : public AppDisplayInterface {
public:
    AppController(HT16K33 &display);

    void addApp(std::shared_ptr<App> app);
    void removeApp(std::shared_ptr<App> app);

    void update();

    virtual void setBrightness(uint8_t brightness) override;
    virtual void setChar(uint8_t digit, char ch, bool dot, bool case_fallback) override;
    virtual void setColon(bool colon) override;

private:
    HT16K33 &_display;
    std::list<std::shared_ptr<App>> _apps;
    decltype(_apps)::iterator _current_app;

    void _switchToNextApp();
};

#endif
