#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <coredecls.h>
#include <time.h>

#include <TM1637.h>

#include <CaptiveConfig.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_TM1637_CLK D1
#define PIN_TM1637_DIO D2

DNSServer dnsServer;
ESP8266WebServer webServer(80);
CaptiveConfig captiveConfig(dnsServer, webServer);
TM1637 display(PIN_TM1637_DIO, PIN_TM1637_CLK);
Ticker ticker;

char apSsid[12];
char apPass[9];

char scroller[64];
size_t scrollerPos = 0;

WiFiEventHandler gotIp;

void setup() {
    display.setupDisplay(true, 0);
    display.clearDisplay();

    snprintf_P(apSsid, sizeof(apSsid), PSTR("CL-%08u"), ESP.getChipId());
    snprintf_P(apPass, sizeof(apPass), PSTR("%08u"), (ESP.getChipId() ^ ESP.getFlashChipId()) % 100000000U);

    snprintf_P(scroller, sizeof(scroller), PSTR(" -- %s -- %s --"), apSsid, apPass);
    scrollerPos = 0;
    size_t scrollerLen = strlen(scroller);

    auto tickerCallback = [scrollerLen]() {
        display.setDisplayToString(scroller + scrollerPos);
        scrollerPos = (scrollerPos + 1) % (scrollerLen - 3);
    };
    ticker.attach_ms_scheduled_accurate(500, tickerCallback);
    tickerCallback();

    // configure time stuff when we got an IP
    gotIp = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        const CaptiveConfigData &data = captiveConfig.getData();
        configTime(data.tz, data.sntpServer0, data.sntpServer1, data.sntpServer2);
    });

    // disable ticker when time is set (which is only done by SNTP here)
    settimeofday_cb([] {
        if (ticker.active()) {
            ticker.detach();
        }
    });

    webServer.begin();
    captiveConfig.begin(apSsid, apPass);
}

void loop() {
    captiveConfig.doLoop();

    if (!ticker.active()) {
        timeval now;
        gettimeofday(&now, nullptr);

        tm local;
        localtime_r(&now.tv_sec, &local);

        static uint8_t curDigits[4] = { 0xfe, 0xfe, 0xfe, 0xfe };
        static bool curDots[4] = { false, false, false, false };

        uint8_t digits[4];
        digits[0] = local.tm_hour < 10 ? 0xff : local.tm_hour / 10;
        digits[1] = local.tm_hour % 10;
        digits[2] = local.tm_min / 10;
        digits[3] = local.tm_min % 10;
        bool dots[4] = { false, now.tv_usec < 500000, false, false };

        for (uint8_t i = 0; i < 4; i++) {
            if (digits[i] != curDigits[i] || dots[i] != curDots[i]) {
                if (digits[i] < 10) {
                    display.setDisplayDigit(digits[i], i, dots[i]);
                } else {
                    display.clearDisplayDigit(i, dots[i]);
                }
            }
            curDigits[i] = digits[i];
            curDots[i] = dots[i];
        }
    }

    // reduce power consumption
    delay(10);
}