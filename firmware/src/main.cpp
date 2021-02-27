#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <coredecls.h>
#include <time.h>

#include <HT16K33.h>

#include <CaptiveConfig.h>
#include <SevenSegment.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_SCL D1
#define PIN_SDA D2

DNSServer dnsServer;
ESP8266WebServer webServer(80);
CaptiveConfig captiveConfig(dnsServer, webServer);
HT16K33 display(0x70);
Ticker ticker;

char apSsid[12];
char apPass[9];

char scroller[64];
size_t scrollerPos = 0;

WiFiEventHandler gotIp;

void setup() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);

    display.begin();
    display.setBrightness(0);

    snprintf_P(apSsid, sizeof(apSsid), PSTR("CL-%08u"), ESP.getChipId());
    snprintf_P(apPass, sizeof(apPass), PSTR("%08u"), (ESP.getChipId() ^ ESP.getFlashChipId()) % 100000000U);

    snprintf_P(scroller, sizeof(scroller), PSTR(" -- %s -- %s --"), apSsid, apPass);
    scrollerPos = 0;
    size_t scrollerLen = strlen(scroller);

    auto tickerCallback = [scrollerLen]() {
        for (uint8_t i = 0; i < 4; i++) {
            display.setLedColumn(i, SevenSegment.getBits(*(scroller + scrollerPos + i)));
        }
        display.updateLeds();
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

        display.updateKeys();

        bool dateMode = display.getKeyColumn(0) & 1;
        bool secondMode = display.getKeyColumn(0) & 2;

        uint8_t digits[4];
        bool dots[4];
        bool colon;
        if (dateMode) {
            digits[0] = local.tm_mday / 10;
            digits[1] = local.tm_mday % 10;
            digits[2] = (local.tm_mon + 1) / 10;
            digits[3] = (local.tm_mon + 1) % 10;
            dots[0] = false;
            dots[1] = true;
            dots[2] = false;
            dots[3] = true;
            colon = false;
        } else if (secondMode) {
            digits[0] = local.tm_min / 10;
            digits[1] = local.tm_min % 10;
            digits[2] = local.tm_sec / 10;
            digits[3] = local.tm_sec % 10;
            dots[0] = false;
            dots[1] = false;
            dots[2] = false;
            dots[3] = false;
            colon = now.tv_usec < 500000;
        } else {
            digits[0] = local.tm_hour < 10 ? 0xff : local.tm_hour / 10;
            digits[1] = local.tm_hour % 10;
            digits[2] = local.tm_min / 10;
            digits[3] = local.tm_min % 10;
            dots[0] = false;
            dots[1] = false;
            dots[2] = false;
            dots[3] = false;
            colon = now.tv_usec < 500000;
        }

        for (uint8_t i = 0; i < 4; i++) {
            if (digits[i] < 10) {
                display.setLedColumn(i, SevenSegment.getBits('0' + digits[i]) | (dots[i] << 7));
            } else {
                display.setLedColumn(i, 0);
            }
        }

        display.setLedColumn(4, colon);

        display.updateLeds();
    }

    // reduce power consumption, and make sure that keys are scanned again (1 cycle of HT16K33 is 9.504 ms)
    delay(20);
}
