#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <coredecls.h>

#include <memory>

#include <HT16K33.h>
#include <CaptiveConfig.h>
#include <SevenSegment.h>

#include <AppController.h>
#include <ClockApp.h>
#include <BrightnessApp.h>
#include <ScrollerApp.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_SCL D1
#define PIN_SDA D2

DNSServer dns_server;
ESP8266WebServer web_server(80);
CaptiveConfig captive_config(dns_server, web_server);
HT16K33 display(0x70);
AppController app_controller(display);

char ap_ssid[12];
char ap_passphrase[9];

WiFiEventHandler got_ip;
WiFiEventHandler connected;
WiFiEventHandler disconnected;

void setup() {
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000);

    display.begin();

    bool force_config_mode = display.getKeyColumn(0) == 0b111;

    snprintf_P(ap_ssid, sizeof(ap_ssid), PSTR("CL-%08u"), ESP.getChipId());
    snprintf_P(ap_passphrase, sizeof(ap_passphrase), PSTR("%08u"), (ESP.getChipId() ^ ESP.getFlashChipId()) % 100000000U);

    captive_config.begin(ap_ssid, ap_passphrase, force_config_mode);

    if (captive_config.isConfigMode()) {
        char ap_ssid_scroller[16];
        snprintf_P(ap_ssid_scroller, sizeof(ap_ssid_scroller), PSTR("- %s -"), ap_ssid);
        auto ap_ssid_scroller_app = std::make_shared<ScrollerApp>(ap_ssid_scroller, 500);

        char ap_passphrase_scroller[13];
        snprintf_P(ap_passphrase_scroller, sizeof(ap_passphrase_scroller), PSTR("- %s -"), ap_passphrase);
        auto ap_passphrase_scroller_app = std::make_shared<ScrollerApp>(ap_passphrase_scroller, 500);

        app_controller.addApp(ap_ssid_scroller_app);
        app_controller.addApp(ap_passphrase_scroller_app);
    } else {
        // configure time stuff when we got an IP
        got_ip = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
            const CaptiveConfigData &data = captive_config.getData();
            configTime(data.tz, data.sntp_server[0], data.sntp_server[1], data.sntp_server[2]);
        });

        auto clock_app = std::make_shared<ClockApp>();

        // show trailing dot in clock app when WiFi is connected
        connected = WiFi.onStationModeConnected([clock_app](const WiFiEventStationModeConnected &event) {
            clock_app->setTimeTrailingDot(true);
        });
        disconnected = WiFi.onStationModeDisconnected([clock_app](const WiFiEventStationModeDisconnected &event) {
            clock_app->setTimeTrailingDot(false);
        });

        // show time as soon as it is set (which is only done by SNTP here)
        settimeofday_cb([clock_app] {
            clock_app->notifyTimeSet();
        });

        app_controller.addApp(clock_app);
        app_controller.addApp(std::make_shared<BrightnessApp>());
    }
}

void loop() {
    captive_config.doLoop();

    app_controller.update();
}
