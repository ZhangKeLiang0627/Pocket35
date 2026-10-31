#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "../hal_config.h"
#include "../hal_board.h"
#include "wifi_portal_html.h"

namespace
{
    constexpr const char* kApSsid = "Pocket35-Setup";
    constexpr const char* kNvsNamespace = "wifi";
    constexpr const char* kNvsKeySsid = "ssid";
    constexpr const char* kNvsKeyPass = "pass";
    constexpr uint32_t kConnectTimeoutMs = 30000;

    WebServer server(80);
    DNSServer dnsServer;
    Preferences prefs;
    uint32_t connectStartTick = 0;

    const char kSaveOkHtml[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>已保存</title>
<style>
body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;font-family:-apple-system,"Segoe UI","PingFang SC","Microsoft YaHei",Roboto,sans-serif;color:#0f172a;padding:20px;background:radial-gradient(1200px 700px at 12% -10%,rgba(99,102,241,.22),transparent 60%),radial-gradient(1000px 700px at 110% 8%,rgba(14,165,233,.2),transparent 55%),linear-gradient(160deg,#f5f7ff,#eef2fe,#ecf8ff)}
.card{background:rgba(255,255,255,.9);border:1px solid rgba(255,255,255,.7);border-radius:26px;padding:40px 32px;text-align:center;max-width:340px;box-shadow:0 30px 80px -18px rgba(37,52,110,.35);animation:rise .5s cubic-bezier(.16,1,.3,1)}
@keyframes rise{from{opacity:0;transform:translateY(20px) scale(.98)}to{opacity:1;transform:none}}
.dot{width:60px;height:60px;margin:0 auto 18px;border-radius:18px;background:linear-gradient(135deg,#10b981,#059669);display:flex;align-items:center;justify-content:center;box-shadow:0 12px 26px -6px rgba(16,185,129,.5)}
.dot svg{width:28px;height:28px}
h1{font-size:21px;margin:0 0 8px;font-weight:800}
p{font-size:14px;color:#64748b;margin:0}
</style>
</head>
<body>
<div class="card">
<div class="dot"><svg viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"></path></svg></div>
<h1>已保存</h1>
<p>正在连接 Wi-Fi，设备即将重启…</p>
</div>
</body>
</html>
)rawliteral";

    String jsonEscape(const String& input)
    {
        String out;
        out.reserve(input.length() + 8);
        for (size_t i = 0; i < input.length(); i++)
        {
            char c = input[i];
            if (c == '"')
            {
                out += "\\\"";
            }
            else if (c == '\\')
            {
                out += "\\\\";
            }
            else if (c == '\n')
            {
                out += "\\n";
            }
            else if (c == '\r')
            {
                out += "\\r";
            }
            else if (c == '\t')
            {
                out += "\\t";
            }
            else
            {
                out += c;
            }
        }
        return out;
    }

    void handleRoot()
    {
        server.send(200, "text/html", kWifiPortalHtml);
    }

    void handleNotFound()
    {
        handleRoot();
    }

    void handleScan()
    {
        int count = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < count; i++)
        {
            if (i > 0)
            {
                json += ',';
            }
            bool open = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
            json += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
                    ",\"open\":" + (open ? "true" : "false") + "}";
        }
        json += ']';
        WiFi.scanDelete();
        server.send(200, "application/json", json);
    }

    void handleSave()
    {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        if (ssid.length() > 0)
        {
            prefs.putString(kNvsKeySsid, ssid);
            prefs.putString(kNvsKeyPass, pass);
            printf("wifi: saved credential for %s\n", ssid.c_str());
        }
        server.send(200, "text/html", kSaveOkHtml);
        delay(800);
        ESP.restart();
    }
} // namespace

void HAL_Board::_wifi_init()
{
    prefs.begin(kNvsNamespace, false);
    String ssid = prefs.getString(kNvsKeySsid, "");
    String pass = prefs.getString(kNvsKeyPass, "");

    if (ssid.isEmpty())
    {
        printf("wifi: no credential, start config ap\n");
        wifiStartConfig();
    }
    else
    {
        _wifi.ssid = ssid.c_str();
        _wifi.hasCredential = true;
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        _wifi.state = WIFI::State::connecting;
        connectStartTick = millis();
        printf("wifi: connecting to %s\n", ssid.c_str());
    }
}

void HAL_Board::wifiStartConfig()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApSsid);
    dnsServer.start(53, "*", WiFi.softAPIP());
    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/save", HTTP_POST, handleSave);
    server.onNotFound(handleNotFound);
    server.begin();
    _wifi.state = WIFI::State::configAp;
    _wifi.ssid = kApSsid;
    _wifi.ip = WiFi.softAPIP().toString().c_str();
    printf("wifi: config ap %s @ %s\n", kApSsid, _wifi.ip.c_str());
}

void HAL_Board::wifiUpdate()
{
    if (_wifi.state == WIFI::State::configAp)
    {
        dnsServer.processNextRequest();
        server.handleClient();
    }
    else if (_wifi.state == WIFI::State::connecting)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            _wifi.state = WIFI::State::connected;
            _wifi.ip = WiFi.localIP().toString().c_str();
            printf("wifi: connected ip=%s\n", _wifi.ip.c_str());
        }
        else if (millis() - connectStartTick > kConnectTimeoutMs)
        {
            printf("wifi: connect timeout, fallback to ap\n");
            wifiStartConfig();
        }
    }
    else if (_wifi.state == WIFI::State::connected)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            _wifi.state = WIFI::State::connecting;
            connectStartTick = millis();
        }
    }
}
