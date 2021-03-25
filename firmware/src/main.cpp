#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <coredecls.h>
#include <time.h>

#include <HT16K33.h>
#include <CaptiveConfig.h>
#include <SevenSegment.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_SCL D1
#define PIN_SDA D2

DNSServer dns_server;
ESP8266WebServer web_server(80);
CaptiveConfig captive_config(dns_server, web_server);
HT16K33 display(0x70);

class WiFiClock {
public:
    WiFiClock(HT16K33 &display) : _display(display) {
    }

    void begin(const char *ap_ssid, const char *ap_passphrase) {
        _mode = _CLOCK_MODE_INIT;
        _ap_ssid = ap_ssid;
        _ap_passphrase = ap_passphrase;
    }

    void setClockReady() {
        _mode = _CLOCK_MODE_TIME;
    }

    void update(unsigned long millis) {
        bool button_0_was_down = display.getKeyColumn(0) & 1;

        display.updateKeys();

        bool button_0_is_down = display.getKeyColumn(0) & 1;
        if (!button_0_was_down && button_0_is_down) {
            _handlePushButton0(millis);
        }

        timeval now;
        gettimeofday(&now, nullptr);

        tm local;
        localtime_r(&now.tv_sec, &local);

        char digits[5] = "    ";
        bool dots[4] = { false, false, false, false };
        bool colon = false;
       
        switch (_mode) {
            case _CLOCK_MODE_INIT:
                colon = now.tv_usec < 500000;
                break;
            case _CLOCK_MODE_WIFI_SSID:
            case _CLOCK_MODE_WIFI_PASSPHRASE:
                if (millis - _scroller_last_move_millis > 500) {
                    if (++_scroller_pos == _scroller_len + 4) {
                        _scroller_pos = 0;
                    }
                    _scroller_last_move_millis = millis;
                }
                {
                    size_t p = _scroller_pos;
                    for (uint8_t i = 0; i < 4; i++) {
                        if (p == 0 || p == _scroller_len + 3) {
                            digits[i] = '-';
                        } else if (p == 1 || p == _scroller_len + 2) {
                            digits[i] = ' ';
                        } else {
                            digits[i] = _scroller[p - 2];
                        }
                        if (++p == _scroller_len + 4) {
                            p = 0;
                        }
                    }
                }
                break;
            case _CLOCK_MODE_TIME:
                snprintf_P(digits, sizeof(digits), PSTR("%2d%02d"), local.tm_hour, local.tm_min);
                colon = now.tv_usec < 500000;
                break;
            case _CLOCK_MODE_DATE:
                snprintf_P(digits, sizeof(digits), PSTR("%02d%02d"), local.tm_mday, local.tm_mon);
                dots[1] = true;
                dots[3] = true;
                break;
            case _CLOCK_MODE_SECONDS:
                snprintf_P(digits, sizeof(digits), PSTR("  %02d"), local.tm_sec);
                colon = now.tv_usec < 500000;
                break;
        }

        for (uint8_t i = 0; i < 4; i++) {
            _display.setLedColumn(i, SevenSegment.getBits(digits[i]) | (dots[i] << 7));
        }
        _display.setLedColumn(4, colon);

        display.updateLeds();
    }

private:
    HT16K33 &_display;

    enum {
        _CLOCK_MODE_INIT,
        _CLOCK_MODE_WIFI_SSID,
        _CLOCK_MODE_WIFI_PASSPHRASE,
        _CLOCK_MODE_TIME,
        _CLOCK_MODE_DATE,
        _CLOCK_MODE_SECONDS,
    } _mode;
    const char *_ap_ssid;
    const char *_ap_passphrase;

    const char *_scroller;
    size_t _scroller_len;
    size_t _scroller_pos;
    unsigned long _scroller_last_move_millis;

    void _handlePushButton0(unsigned long millis) {
        switch (_mode) {
            case _CLOCK_MODE_INIT:
                _scroller = _ap_ssid;
                _scroller_len = strlen(_scroller);
                _scroller_pos = 0;
                _scroller_last_move_millis = millis;
                _mode = _CLOCK_MODE_WIFI_SSID;
                break;
            case _CLOCK_MODE_WIFI_SSID:
                _scroller = _ap_passphrase;
                _scroller_len = strlen(_scroller);
                _scroller_pos = 0;
                _scroller_last_move_millis = millis;
                _mode = _CLOCK_MODE_WIFI_PASSPHRASE;
                break;
            case _CLOCK_MODE_WIFI_PASSPHRASE:
                _mode = _CLOCK_MODE_INIT;
                break;
            case _CLOCK_MODE_TIME:
                _mode = _CLOCK_MODE_DATE;
                break;
            case _CLOCK_MODE_DATE:
                _mode = _CLOCK_MODE_SECONDS;
                break;
            case _CLOCK_MODE_SECONDS:
                _mode = _CLOCK_MODE_TIME;
                break;
        }
    }
};

WiFiClock wifi_clock(display);

char ap_ssid[12];
char ap_passphrase[9];

WiFiEventHandler got_ip;

void setup() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);

    display.begin();
    display.setBrightness(0);

    snprintf_P(ap_ssid, sizeof(ap_ssid), PSTR("CL-%08u"), ESP.getChipId());
    snprintf_P(ap_passphrase, sizeof(ap_passphrase), PSTR("%08u"), (ESP.getChipId() ^ ESP.getFlashChipId()) % 100000000U);

    wifi_clock.begin(ap_ssid, ap_passphrase);

    // configure time stuff when we got an IP
    got_ip = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        const CaptiveConfigData &data = captive_config.getData();
        configTime(data.tz, data.sntpServer0, data.sntpServer1, data.sntpServer2);
    });

    // update clock state when time is set (which is only done by SNTP here)
    settimeofday_cb([] {
        wifi_clock.setClockReady();
    });

    web_server.begin();

    // wait for the keys to be readable
    delay(20);
    display.updateKeys();
    uint8_t buttons_down = display.getKeyColumn(0) & 3;

    captive_config.begin(ap_ssid, ap_passphrase, buttons_down != 3);

    // make sure that keys are scanned again (1 cycle of HT16K33 is 9.504 ms)
    delay(20);
}


void loop() {
    captive_config.doLoop();

    wifi_clock.update(millis());

    // make sure that keys are scanned again (1 cycle of HT16K33 is 9.504 ms)
    delay(20);
}
