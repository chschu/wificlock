#include <time.h>

#include "CaptiveConfig.h"

#define CAPTIVE_CONFIG_MAGIC 0xc01db00b

#define CAPTIVE_CONFIG_SSID_PARAM_NAME "ssid"
#define CAPTIVE_CONFIG_PASSPHRASE_PARAM_NAME "passphrase"
#define CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME "sntp-server-0"
#define CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME "sntp-server-1"
#define CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME "sntp-server-2"
#define CAPTIVE_CONFIG_TZ_PARAM_NAME "tz"

const char CAPTIVE_CONFIG_PAGE_URI[] PROGMEM = "/_captive/config";

CaptiveConfig::CaptiveConfig(DNSServer &dns_server, ESP8266WebServer &web_server)
    : _dns_server(dns_server), _web_server(web_server) {
}

void CaptiveConfig::begin(const char *ap_ssid, const char *ap_passphrase) {
    EEPROM.begin(sizeof(this->_data));
    EEPROM.get(0, this->_data);

    // initialize config if magic number is not found in EEPROM
    if (this->_data.magic != CAPTIVE_CONFIG_MAGIC) {
        this->_data.magic = CAPTIVE_CONFIG_MAGIC;
        this->_data.ssid[0] = 0;
        this->_data.passphrase[0] = 0;
        strncpy(this->_data.sntp_server[0], "0.de.pool.ntp.org", sizeof(this->_data.sntp_server[0]));
        strncpy(this->_data.sntp_server[1], "1.de.pool.ntp.org", sizeof(this->_data.sntp_server[1]));
        strncpy(this->_data.sntp_server[2], "2.de.pool.ntp.org", sizeof(this->_data.sntp_server[2]));
        strncpy(this->_data.tz, "CET-1CEST,M3.5.0,M10.5.0/3", sizeof(this->_data.tz));
    }

    // disable AP once STA got an IP
    this->_on_station_mode_got_ip = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &event) {
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
    WiFi.softAP(ap_ssid, ap_passphrase);

    // start DNS server for captive portal
    this->_dns_server.setErrorReplyCode(DNSReplyCode::NoError);
    this->_dns_server.start(53, "*", WiFi.softAPIP());

    // configure web server handlers
    this->_web_server.onNotFound([this] {
        if (!this->handleCaptivePortal()) {
            this->handleNotFound();
        }
    });
    this->_web_server.on(FPSTR(CAPTIVE_CONFIG_PAGE_URI), HTTP_GET, [this] {
        if (!this->handleCaptivePortal()) {
            this->handleGetConfigPage();
        }
    });
    this->_web_server.on(FPSTR(CAPTIVE_CONFIG_PAGE_URI), HTTP_POST, [this] {
        if (!this->handleCaptivePortal()) {
            this->handlePostConfigPage();
        }
    });

    // start web server for captive portal
    this->_web_server.begin();
}

bool CaptiveConfig::handleCaptivePortal() {
    String host = this->_web_server.hostHeader();
    String local_ip = this->_web_server.client().localIP().toString();
    if (host != local_ip) {
        this->_web_server.sendHeader(String(F("Location")), String(F("http://")) + local_ip + String(FPSTR(CAPTIVE_CONFIG_PAGE_URI)));
        this->_web_server.setContentLength(0);
        this->_web_server.send(302, "text/plain", "");
        return true;
    }
    return false;
}

void CaptiveConfig::handleNotFound() {
    this->_web_server.send(404, "text/plain", "");
}

void CaptiveConfig::handleGetConfigPage() {
    if (this->handleCaptivePortal()) {
        return;
    }

    this->_web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->_web_server.sendHeader("Pragma", "no-cache");
    this->_web_server.sendHeader("Expires", "-1");
    this->_web_server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    this->_web_server.send(200, "text/html", "");

    this->_sendConfigPageHtml([this] {
        this->_sendFieldset("WiFi", [this]() {
            this->_sendTextInput("SSID", CAPTIVE_CONFIG_SSID_PARAM_NAME, CAPTIVE_CONFIG_SSID_MAX_LENGTH, this->_data.ssid);
            this->_sendPasswordInput("Passphrase (empty to keep)", CAPTIVE_CONFIG_PASSPHRASE_PARAM_NAME, CAPTIVE_CONFIG_PASSPHRASE_MAX_LENGTH, "");
        });
        this->_sendFieldset("Time", [this]() {
            this->_sendTextInput("SNTP Server (primary)", CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntp_server[0]);
            this->_sendTextInput("SNTP Server (1st fallback)", CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntp_server[1]);
            this->_sendTextInput("SNTP Server (2nd fallback)", CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME, CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH, this->_data.sntp_server[2]);
            this->_sendTextInput("TZ", CAPTIVE_CONFIG_TZ_PARAM_NAME, CAPTIVE_CONFIG_TZ_MAX_LENGTH, this->_data.tz);
        });
    });

    // stop client (required because content-length is unknown)
    this->_web_server.client().stop();
}

void CaptiveConfig::handlePostConfigPage() {
    if (this->handleCaptivePortal()) {
        return;
    }

    const String &ssid = this->_web_server.arg(CAPTIVE_CONFIG_SSID_PARAM_NAME);
    const String &passphrase = this->_web_server.arg(CAPTIVE_CONFIG_PASSPHRASE_PARAM_NAME);
    const String &sntp_server_0 = this->_web_server.arg(CAPTIVE_CONFIG_SNTP_SERVER_0_PARAM_NAME);
    const String &sntp_server_1 = this->_web_server.arg(CAPTIVE_CONFIG_SNTP_SERVER_1_PARAM_NAME);
    const String &sntp_server_2 = this->_web_server.arg(CAPTIVE_CONFIG_SNTP_SERVER_2_PARAM_NAME);
    const String &tz = this->_web_server.arg(CAPTIVE_CONFIG_TZ_PARAM_NAME);

    // TODO validate params

    strncpy(this->_data.ssid, ssid.c_str(), sizeof(this->_data.ssid));
    if (!passphrase.isEmpty()) {
        strncpy(this->_data.passphrase, passphrase.c_str(), sizeof(this->_data.passphrase));
    }
    strncpy(this->_data.sntp_server[0], sntp_server_0.c_str(), sizeof(this->_data.sntp_server[0]));
    strncpy(this->_data.sntp_server[0], sntp_server_1.c_str(), sizeof(this->_data.sntp_server[1]));
    strncpy(this->_data.sntp_server[2], sntp_server_2.c_str(), sizeof(this->_data.sntp_server[2]));
    strncpy(this->_data.tz, tz.c_str(), sizeof(this->_data.tz));

    EEPROM.put(0, this->_data);
    EEPROM.commit();

    this->_web_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    this->_web_server.sendHeader("Pragma", "no-cache");
    this->_web_server.sendHeader("Expires", "-1");

    this->_web_server.send(200, "text/html", F(
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
    this->_web_server.client().stop();

    // restart the thing
    ESP.restart();
}

const CaptiveConfigData &CaptiveConfig::getData() {
    return this->_data;
}

void CaptiveConfig::doLoop() {
    this->_dns_server.processNextRequest();
    this->_web_server.handleClient();

    if (WiFi.getMode() == WIFI_AP && this->_data.ssid[0] && this->_data.passphrase[0]) {
        WiFi.begin(this->_data.ssid, this->_data.passphrase);
    }
}

void CaptiveConfig::_sendConfigPageHtml(const std::function<void()> &inner) {
    this->_web_server.sendContent(F(
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

    this->_web_server.sendContent(F(
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

    this->_web_server.sendContent(prefix);

    inner();

    this->_web_server.sendContent(F(
        "</fieldset>"
    ));
}

void CaptiveConfig::_sendInput(const char *type, const char *label, const char *name, uint16_t max_length, const char *value) {
    static uint32_t next_id = 0;
    char id_str[11];
    snprintf_P(id_str, sizeof(id_str), PSTR("id%x"), next_id++);

    char max_length_str[11];
    snprintf_P(max_length_str, sizeof(max_length_str), PSTR("%u"), max_length);

    String content = F(
        "<label for=\"{i}\">{l}</label>"
        "<input type=\"{t}\" id=\"{i}\" name=\"{n}\" maxlength=\"{m}\" value=\"{v}\"/>"
    );

    content.replace(F("{i}"), id_str);
    content.replace(F("{l}"), label);
    content.replace(F("{t}"), type);
    content.replace(F("{n}"), name);
    content.replace(F("{m}"), max_length_str);
    content.replace(F("{v}"), value);

    this->_web_server.sendContent(content);
}

void CaptiveConfig::_sendTextInput(const char *label, const char *name, uint16_t max_length, const char *value) {
    this->_sendInput("text", label, name, max_length, value);
}

void CaptiveConfig::_sendPasswordInput(const char *label, const char *name, uint16_t max_length, const char *value) {
    this->_sendInput("password", label, name, max_length, value);
}
