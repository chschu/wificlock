#ifndef _CAPTIVE_CONFIG_H
#define _CAPTIVE_CONFIG_H

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define CAPTIVE_CONFIG_SSID_MAX_LENGTH         32
#define CAPTIVE_CONFIG_PASSPHRASE_MAX_LENGTH   63
#define CAPTIVE_CONFIG_HOSTNAME_MAX_LENGTH     24
#define CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH  63
#define CAPTIVE_CONFIG_TZ_MAX_LENGTH           63

struct CaptiveConfigData {
    char ssid[CAPTIVE_CONFIG_SSID_MAX_LENGTH + 1];
    char passphrase[CAPTIVE_CONFIG_PASSPHRASE_MAX_LENGTH + 1];
    char hostname[CAPTIVE_CONFIG_HOSTNAME_MAX_LENGTH + 1];
    char sntp_server[3][CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH + 1];
    char tz[CAPTIVE_CONFIG_TZ_MAX_LENGTH + 1];
};

class CaptiveConfig {
public:
    CaptiveConfig(DNSServer &dns_server, ESP8266WebServer &web_server);

    void begin(const char *ap_ssid, const char *ap_passphrase, bool force_config_mode);
    void doLoop();

    /**
     * Checks if the current web request is a captive portal request, i.e. targeted at another host.
     * If it is, responds with a redirect, and returns true. Otherwise, returns false.
     * 
     * Must be called in every handler added to the web server before handling the actual request.
     */
    bool handleCaptivePortal();

    /**
     * Handles requests that don't have a matching handler registered.
     */
    void handleNotFound();

    /**
     * Handles a GET request to the config page.
     * Renders the configuration page.
     */
    void handleGetConfigPage();

    /**
     * Handles a POST request to the config page, i.e. a form submit.
     */
    void handlePostConfigPage();

    const CaptiveConfigData &getData();

private:
    DNSServer &_dns_server;
    ESP8266WebServer &_web_server;

    bool _config_mode;

    CaptiveConfigData _data;

    void _sendConfigPageHtml(const std::function<void()> &inner);
    void _sendFieldset(const char *legend, const std::function<void()> &inner);
    void _sendInput(const char *type, const char *label, const char *name, uint16_t maxLength, const char *value);
    void _sendTextInput(const char *label, const char *name, uint16_t max_length, const char *value);
    void _sendPasswordInput(const char *label, const char *name, uint16_t max_length, const char *value);
};

#endif
