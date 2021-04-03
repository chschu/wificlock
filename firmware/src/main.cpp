#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <coredecls.h>

#include <HT16K33.h>
#include <CaptiveConfig.h>
#include <SevenSegment.h>
#include <WiFiClock.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_SCL D1
#define PIN_SDA D2

DNSServer dns_server;
ESP8266WebServer web_server(80);
CaptiveConfig captive_config(dns_server, web_server);
HT16K33 display(0x70);
WiFiClock wifi_clock(display);

enum display_mode_t {
    DISPLAY_MODE_INIT,
    DISPLAY_MODE_SSID,
    DISPLAY_MODE_PASSPHRASE,
    DISPLAY_MODE_TIME,
    DISPLAY_MODE_DATE,
    DISPLAY_MODE_SECONDS
};

display_mode_t mode = DISPLAY_MODE_INIT;
uint8_t brightness = 0;

char ap_ssid[12];
char ap_passphrase[9];

char ap_ssid_scroller[16];
char ap_passphrase_scroller[13];

WiFiEventHandler got_ip;
WiFiEventHandler connected;
WiFiEventHandler disconnected;

void setup() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);

    display.begin();

    snprintf_P(ap_ssid, sizeof(ap_ssid), PSTR("CL-%08u"), ESP.getChipId());
    snprintf_P(ap_passphrase, sizeof(ap_passphrase), PSTR("%08u"), (ESP.getChipId() ^ ESP.getFlashChipId()) % 100000000U);

    snprintf_P(ap_ssid_scroller, sizeof(ap_ssid_scroller), PSTR("- %s -"), ap_ssid);
    snprintf_P(ap_passphrase_scroller, sizeof(ap_passphrase_scroller), PSTR("- %s -"), ap_passphrase);

    wifi_clock.setKeyHandler(0, 0, [](uint8_t col, uint8_t row) {
        switch (mode) {
            case DISPLAY_MODE_INIT:
                mode = DISPLAY_MODE_SSID;
                break;
            case DISPLAY_MODE_SSID:
                mode = DISPLAY_MODE_PASSPHRASE;
                break;
            case DISPLAY_MODE_PASSPHRASE:
                mode = DISPLAY_MODE_INIT;
                break;
            case DISPLAY_MODE_TIME:
                mode = DISPLAY_MODE_DATE;
                break;
            case DISPLAY_MODE_DATE:
                mode = DISPLAY_MODE_SECONDS;
                break;
            case DISPLAY_MODE_SECONDS:
                mode = DISPLAY_MODE_TIME;
                break;
        }
        switch (mode) {
            case DISPLAY_MODE_INIT:
                wifi_clock.modeNoTime();
                break;
            case DISPLAY_MODE_SSID:
                wifi_clock.modeScroller(ap_ssid_scroller, 500);
                break;
            case DISPLAY_MODE_PASSPHRASE:
                wifi_clock.modeScroller(ap_passphrase_scroller, 500);
                break;
            case DISPLAY_MODE_TIME:
                wifi_clock.modeTime();
                break;
            case DISPLAY_MODE_DATE:
                wifi_clock.modeDate();
                break;
            case DISPLAY_MODE_SECONDS:
                wifi_clock.modeSeconds();
                break;
        }
    });

    display.setBrightness(brightness);
    wifi_clock.setKeyHandler(0, 1, [](uint8_t col, uint8_t row) {
        // 0 -> 1 -> 3 -> 7 -> 15 -> 0
        brightness = 2 * brightness + 1;
        if (brightness > 15) {
            brightness = 0;
        }
        display.setBrightness(brightness);
    });

    // configure time stuff when we got an IP
    got_ip = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        const CaptiveConfigData &data = captive_config.getData();
        configTime(data.tz, data.sntp_server[0], data.sntp_server[1], data.sntp_server[2]);
    });

    // notify clock of WiFi state change
    connected = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &event) {
        wifi_clock.setWiFiConnected(true);
    });
    disconnected = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event) {
        wifi_clock.setWiFiConnected(false);
    });

    // update display mode when time is set (which is only done by SNTP here)
    settimeofday_cb([] {
        mode = DISPLAY_MODE_TIME;
        wifi_clock.modeTime();
    });

    web_server.begin();

    captive_config.begin(ap_ssid, ap_passphrase);
}

void loop() {
    captive_config.doLoop();

    wifi_clock.update();
}
