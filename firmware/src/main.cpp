#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <coredecls.h>
#include <time.h>

#include <TM1637Display.h>

#include <CaptiveConfig.h>

#define PIN_STATUS LED_BUILTIN
#define PIN_TM1637_CLK D1
#define PIN_TM1637_DIO D2

DNSServer dnsServer;
ESP8266WebServer webServer(80);
CaptiveConfig captiveConfig(dnsServer, webServer);
TM1637Display display(PIN_TM1637_CLK, PIN_TM1637_DIO);
Ticker ticker;

typedef uint8_t mapping_t[2];

const mapping_t MAPPINGS[]= {
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

bool hasMapping(char c) {
    for (const mapping_t *m = &MAPPINGS[0]; (*m)[0]; m++) {
        if ((*m)[0] == c) {
            return true;
        }
    }
    return false;
}

char getMapped(char c) {
    if (hasMapping(c)) {
        return c;
    }
    char c2 = tolower(c);
    if (c2 == c) {
        c2 = toupper(c);
    }
    if (c2 != c && hasMapping(c2)) {
        return c2;
    }
    return ' ';
}

void replaceWithMapped(char *s) {
    for (char *p = s; *p; p++) {
        *p = getMapped(*p);
    }
}

uint8_t getSegments(char c) {
    for (const mapping_t *m = &MAPPINGS[0]; (*m)[0]; m++) {
        if ((*m)[0] == c) {
            return (*m)[1];
        }
    }
    return 0b00000000;
}

char apSsid[10];
char apPass[9];

char scroller[64];
size_t scrollerPos = 0;

WiFiEventHandler gotIp;

void setup() {
    display.setBrightness(0);
    display.clear();

    snprintf_P(apSsid, sizeof(apSsid), PSTR("CL-%06X"), ESP.getChipId());
    replaceWithMapped(apSsid);

    snprintf_P(apPass, sizeof(apPass), PSTR("%08X"), (ESP.getChipId() << 8) ^ ESP.getFlashChipId());
    replaceWithMapped(apPass);

    snprintf_P(scroller, sizeof(scroller), PSTR(" -- %s -- %s"), apSsid, apPass);
    replaceWithMapped(scroller);
    scrollerPos = 0;
    size_t scrollerLen = strlen(scroller);

    auto tickerCallback = [scrollerLen]() {
        uint8_t segments[] = { 0, 0, 0, 0 };
        for (size_t i = 0; i < 4; i++) {
            segments[i] = getSegments(scroller[(scrollerPos + i) % scrollerLen]);
        }
        scrollerPos = (scrollerPos + 1) % scrollerLen;
        display.setSegments(segments);
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

        uint8_t segments[4];
        if (local.tm_hour > 10) {
            segments[0] =  display.encodeDigit(local.tm_hour / 10);
        } else {
            segments[0] = 0;
        }
        segments[1] = display.encodeDigit(local.tm_hour % 10);
        segments[2] = display.encodeDigit(local.tm_min / 10);
        segments[3] = display.encodeDigit(local.tm_min % 10);
        if (now.tv_usec < 500000) {
            segments[1] |= SEG_DP;
        }
        display.setSegments(segments);
    }

    // reduce power consumption
    delay(10);
}