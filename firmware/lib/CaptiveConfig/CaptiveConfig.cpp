#include <ESP8266mDNS.h>
#include <time.h>

#include "CaptiveConfig.h"

#define CAPTIVE_CONFIG_MAGIC 0xc01db00b

#define CAPTIVE_CONFIG_SSID_PARAM_NAME "ssid"
#define CAPTIVE_CONFIG_PASSWORD_PARAM_NAME "password"
#define CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME "sntp-server-0"
#define CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME "sntp-server-1"
#define CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME "sntp-server-2"
#define CAPTIVE_CONFIG_TZ_PARAM_NAME "tz"

const char CAPTIVE_CONFIG_PAGE_URI[] PROGMEM = "/_captive/config";

CaptiveConfig::CaptiveConfig(DNSServer &dnsServer, ESP8266WebServer &webServer)
    : _dnsServer(dnsServer), _webServer(webServer) {
}

void CaptiveConfig::begin(const char *apSsid, const char *apPassword) {
    EEPROM.begin(sizeof(this->_data));
    EEPROM.get(0, this->_data);

    // initialize config if magic number is not found in EEPROM
    if (this->_data.magic != CAPTIVE_CONFIG_MAGIC) {
        this->_data.magic = CAPTIVE_CONFIG_MAGIC;
        this->_data.ssid[0] = 0;
        this->_data.password[0] = 0;
        strncpy(this->_data.sntpServer0, "0.de.pool.ntp.org", sizeof(this->_data.sntpServer0));
        strncpy(this->_data.sntpServer1, "1.de.pool.ntp.org", sizeof(this->_data.sntpServer1));
        strncpy(this->_data.sntpServer2, "2.de.pool.ntp.org", sizeof(this->_data.sntpServer2));
        strncpy(this->_data.tz, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(this->_data.tz));
    }

    // disable AP once STA got an IP
    this->_onStationModeGotIP = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
        WiFi.softAPdisconnect(true);
        WiFi.enableAP(false);
    });

    // WiFi config is stored in EEPROM, don't store it in Flash
    WiFi.persistent(false);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);

    // start with WiFi off
    WiFi.mode(WIFI_OFF);

    // enable AP during initial phase
    WiFi.softAP(apSsid, apPassword);

    // start DNS server for captive portal
    this->_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    this->_dnsServer.start(53, "*", WiFi.softAPIP());

    // configure web server handlers
    this->_webServer.onNotFound([this] {
        if (!this->handleCaptivePortal()) {
            this->handleNotFound();
        }
    });
    this->_webServer.on(FPSTR(CAPTIVE_CONFIG_PAGE_URI), HTTP_GET, [this] {
        if (!this->handleCaptivePortal()) {
            this->handleGetConfigPage();
        }
    });
    this->_webServer.on(FPSTR(CAPTIVE_CONFIG_PAGE_URI), HTTP_POST, [this] {
        if (!this->handleCaptivePortal()) {
            this->handlePostConfigPage();
        }
    });

    // start web server for captive portal
    this->_webServer.begin();
}

bool CaptiveConfig::handleCaptivePortal() {
    String host = this->_webServer.hostHeader();
    String localIp = this->_webServer.client().localIP().toString();
    Serial.println(host);
    Serial.println(localIp);
    if (host != localIp) {
        this->_webServer.sendHeader(String(F("Location")), String(F("http://")) + localIp + String(FPSTR(CAPTIVE_CONFIG_PAGE_URI)));
        this->_webServer.setContentLength(0);
        this->_webServer.send(302, "text/plain", "");
        return true;
    }
    return false;
}

void CaptiveConfig::handleNotFound() {
    this->_webServer.send(404, "text/plain", "");
}

void CaptiveConfig::handleGetConfigPage() {
    if (this->handleCaptivePortal()) {
        return;
    }

    this->_webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->_webServer.sendHeader("Pragma", "no-cache");
    this->_webServer.sendHeader("Expires", "-1");
    this->_webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    this->_webServer.send(200, "text/html", "");

    this->_sendConfigPageHtml([this] {
        this->_sendFieldset("WiFi", [this]() {
            this->_sendTextInput("SSID", CAPTIVE_CONFIG_SSID_PARAM_NAME, CAPTIVE_CONFIG_SSID_MAX_LENGTH, this->_data.ssid);
            this->_sendPasswordInput("Password (empty to keep)", CAPTIVE_CONFIG_PASSWORD_PARAM_NAME, CAPTIVE_CONFIG_PASSWORD_MAX_LENGTH, "");
        });
        this->_sendFieldset("Time", [this]() {
            this->_sendTextInput("SNTP Server (primary)", CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntpServer0);
            this->_sendTextInput("SNTP Server (1st fallback)", CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntpServer1);
            this->_sendTextInput("SNTP Server (2nd fallback)", CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntpServer2);
            this->_sendTextInput("TZ", CAPTIVE_CONFIG_TZ_PARAM_NAME, CAPTIVE_CONFIG_TZ_MAX_LENGTH, this->_data.tz);
        });
    });

    // stop client (required because content-length is unknown)
    this->_webServer.client().stop();
}

void CaptiveConfig::handlePostConfigPage() {
    if (this->handleCaptivePortal()) {
        return;
    }

    const String &ssid = this->_webServer.arg(CAPTIVE_CONFIG_SSID_PARAM_NAME);
    const String &password = this->_webServer.arg(CAPTIVE_CONFIG_PASSWORD_PARAM_NAME);
    const String &sntpServer0 = this->_webServer.arg(CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME);
    const String &sntpServer1 = this->_webServer.arg(CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME);
    const String &sntpServer2 = this->_webServer.arg(CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME);
    const String &tz = this->_webServer.arg(CAPTIVE_CONFIG_TZ_PARAM_NAME);

    // TODO validate params

    strncpy(this->_data.ssid, ssid.c_str(), sizeof(this->_data.ssid));
    if (!password.isEmpty()) {
        strncpy(this->_data.password, password.c_str(), sizeof(this->_data.password));
    }
    strncpy(this->_data.sntpServer0, sntpServer0.c_str(), sizeof(this->_data.sntpServer0));
    strncpy(this->_data.sntpServer1, sntpServer1.c_str(), sizeof(this->_data.sntpServer1));
    strncpy(this->_data.sntpServer2, sntpServer2.c_str(), sizeof(this->_data.sntpServer2));
    strncpy(this->_data.tz, tz.c_str(), sizeof(this->_data.tz));

    EEPROM.put(0, this->_data);
    EEPROM.commit();

    this->_webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->_webServer.sendHeader("Pragma", "no-cache");
    this->_webServer.sendHeader("Expires", "-1");

    this->_webServer.send(200, "text/html", F(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"
            "<title>Captive Config Saved</title>"
            "<style>"
                "body{font-family:Verdana,sans-serif;padding:0.5em;}"
                "div{border:1px solid #000;border-radius:0.5rem;margin:0;width:100%;box-sizing:border-box;}"
                "p{margin:0.5rem;}"
            "</style>"
        "</head>"
        "<body>"
            "<div>"
                "<p>Configuation has been saved.<p>"
                "<p>Device will be restarted.</p>"
            "</div>"
        "</body>"
        "</html>"
    ));

    // stop client to finish response immediately
    this->_webServer.client().stop();

    // restart the thing
    ESP.restart();
}

const CaptiveConfigData &CaptiveConfig::getData() {
    return this->_data;
}

void CaptiveConfig::doLoop() {
    this->_dnsServer.processNextRequest();
    this->_webServer.handleClient();

    if (WiFi.getMode() == WIFI_AP && this->_data.ssid[0] && this->_data.password[0]) {
        Serial.println(this->_data.ssid);
        Serial.println(this->_data.password);
        WiFi.begin(this->_data.ssid, this->_data.password);
    }
}

void CaptiveConfig::_sendConfigPageHtml(const std::function<void()> &inner) {
    this->_webServer.sendContent(F(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>"
            "<title>Captive Config</title>"
            "<style>"
                "body{font-family:Verdana,sans-serif;padding:0.5em;}"
                "fieldset{border:1px solid #000;border-radius:0.5rem;margin:0;margin-bottom:0.5rem;width:100%;box-sizing:border-box;}"
                "label{display:block;font-size:1rem;margin:0.25rem 0;padding:0.25rem;}"
                "input{display:block;font-size:1rem;margin:0.5rem 0;padding:0.25rem;width:100%;box-sizing:border-box;}"
                "button{font-size:1rem;margin:1em 0 0 0;border:0;padding:1rem;width:100%;border-radius:0.5rem;background-color:#323f77;color:#fff;}"
            "</style>"
        "</head>"
        "<body>"
            "<form action=\"\" method=\"POST\">"
    ));

    inner();

    this->_webServer.sendContent(F(
                "<button id=\"btn\" type=\"submit\">Save</button>"
            "</form>"
        "</body>"
        "</html>"
    ));
}

void CaptiveConfig::_sendFieldset(const char *legend, const std::function<void()> &inner) {
    String prefix = F(
        "<fieldset>"
            "<legend>{l}</legend>"
    );

    prefix.replace(F("{l}"), legend);

    this->_webServer.sendContent(prefix);

    inner();

    this->_webServer.sendContent(F(
        "</fieldset>"
    ));
}

void CaptiveConfig::_sendInput(const char *type, const char *label, const char *name, uint16_t maxLength, const char *value) {
    static uint32_t nextId = 0;
    char idStr[11];
    snprintf_P(idStr, sizeof(idStr), PSTR("id%x"), nextId++);

    char maxLengthStr[11];
    snprintf_P(maxLengthStr, sizeof(maxLengthStr), PSTR("%u"), maxLength);

    String content = F(
        "<label for=\"{i}\">{l}</label>"
        "<input type=\"{t}\" id=\"{i}\" name=\"{n}\" maxlength=\"{m}\" value=\"{v}\"/>"
    );

    content.replace(F("{i}"), idStr);
    content.replace(F("{l}"), label);
    content.replace(F("{t}"), type);
    content.replace(F("{n}"), name);
    content.replace(F("{m}"), maxLengthStr);
    content.replace(F("{v}"), value);

    this->_webServer.sendContent(content);
}

void CaptiveConfig::_sendTextInput(const char *label, const char *name, uint16_t maxLength, const char *value) {
    this->_sendInput("text", label, name, maxLength, value);
}

void CaptiveConfig::_sendPasswordInput(const char *label, const char *name, uint16_t maxLength, const char *value) {
    this->_sendInput("password", label, name, maxLength, value);
}
