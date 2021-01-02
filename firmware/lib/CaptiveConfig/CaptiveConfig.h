#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define CAPTIVE_CONFIG_SSID_MAX_LENGTH         32
#define CAPTIVE_CONFIG_PASSWORD_MAX_LENGTH     63
#define CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH  63
#define CAPTIVE_CONFIG_TZ_MAX_LENGTH           63

struct CaptiveConfigData {
    uint32_t magic;
    char ssid[CAPTIVE_CONFIG_SSID_MAX_LENGTH + 1];
    char password[CAPTIVE_CONFIG_PASSWORD_MAX_LENGTH + 1];
    char sntpServer0[CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH + 1];
    char sntpServer1[CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH + 1];
    char sntpServer2[CAPTIVE_CONFIG_SNTP_SERVER_MAX_LENGTH + 1];
    char tz[CAPTIVE_CONFIG_TZ_MAX_LENGTH + 1];
} __attribute__((packed));

class CaptiveConfig {
public:
    CaptiveConfig(DNSServer &dnsServer, ESP8266WebServer &webServer);

    void begin(const char *apSsid, const char *apPassword);
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
    DNSServer &_dnsServer;
    ESP8266WebServer &_webServer;

    CaptiveConfigData _data;

    WiFiEventHandler _onStationModeGotIP;

    void _sendConfigPageHtml(const std::function<void()> &inner);
    void _sendFieldset(const char *legend, const std::function<void()> &inner);
    void _sendInput(const char *type, const char *label, const char *name, uint16_t maxLength, const char *value);
    void _sendTextInput(const char *label, const char *name, uint16_t maxLength, const char *value);
    void _sendPasswordInput(const char *label, const char *name, uint16_t maxLength, const char *value);
};
