#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <coredecls.h>
#include <time.h>

#include <HT16K33.h>

#include <CaptiveConfig.h>

struct mapping_t {
    char ch;
    uint16_t bits;
};

const mapping_t MAPPINGS_7SEG[]= {
    { ' ', 0b00000000 },
    { '0', 0b00111111 },
    { '1', 0b00000110 }, 
    { '2', 0b01011011 },
    { '3', 0b01001111 },
    { '4', 0b01100110 },
    { '5', 0b01101101 },
    { '6', 0b01111101 },
    { '7', 0b00000111 },
    { '8', 0b01111111 },
    { '9', 0b01101111 },
    { 'A', 0b01110111 },
    { 'b', 0b01111100 },
    { 'c', 0b01011000 },
    { 'C', 0b00111001 },
    { 'd', 0b01011110 },
    { 'E', 0b01111001 },
    { 'F', 0b01110001 },
    { 'G', 0b00111101 },
    { 'h', 0b01110100 },
    { 'H', 0b01110110 },
    { 'i', 0b00000100 },
    { 'j', 0b00001100 },
    { 'J', 0b00011110 },
    { 'L', 0b00111000 },
    { 'n', 0b01010100 },
    { 'o', 0b01011100 },
    { 'P', 0b01110011 },
    { 'q', 0b01100111 },
    { 'r', 0b01010000 },
    { 'S', 0b01101101 },
    { 't', 0b01111000 },
    { 'u', 0b00011100 },
    { 'y', 0b01101110 },
    { 'Y', 0b01100110 },
    { '-', 0b01000000 },
    { '_', 0b00001000 },
    { '@', 0b01111011 },
    {   0, 0b00000000 }
};

bool hasMapping(char ch) {
    for (const mapping_t *m = &MAPPINGS_7SEG[0]; m->ch; m++) {
        if (m->ch == ch) {
            return true;
        }
    }
    return false;
}

char getMapped(char ch) {
    if (hasMapping(ch)) {
        return ch;
    }
    char ch2 = tolower(ch);
    if (ch2 == ch) {
        ch2 = toupper(ch);
    }
    if (ch2 != ch && hasMapping(ch2)) {
        return ch2;
    }
    return ' ';
}

uint16_t getBits(char ch) {
    for (const mapping_t *m = &MAPPINGS_7SEG[0]; m->ch; m++) {
        if (m->ch == ch) {
            return m->bits;
        }
    }
    return 0;
}

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
            display.set(i + (i > 1), getBits(*(scroller + scrollerPos + i)));
        }
        display.flush();
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

        uint8_t digits[4];
        digits[0] = local.tm_hour < 10 ? 0xff : local.tm_hour / 10;
        digits[1] = local.tm_hour % 10;
        digits[2] = local.tm_min / 10;
        digits[3] = local.tm_min % 10;
        bool colon = now.tv_usec < 500000;

        for (uint8_t i = 0; i < 4; i++) {
            if (digits[i] < 10) {
                display.set(i + (i > 1), getBits('0' + digits[i]));
            } else {
                display.set(i + (i > 1), 0);
            }
        }

        display.set(2, colon << 8);

        display.flush();
    }

    // reduce power consumption
    delay(10);
}
