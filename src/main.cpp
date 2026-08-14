#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <SD.h>
#include <WebServer.h>
#include <WiFi.h>
#include <vector>
#include <algorithm>
#include <esp_system.h>

// GPTputer ADV - standalone OpenAI text client for M5Stack Cardputer ADV.
// Application-only workflow (no SoftAP):
// 1) scan nearby Wi-Fi networks automatically and select one by number;
// 2) enter the Wi-Fi password in plain text on the Cardputer keyboard;
// 3) DHCP assigns an address;
// 4) open the displayed LAN URL from a PC/tablet to enter the OpenAI API key/model.

namespace {
constexpr char kApiUrl[] = "https://api.openai.com/v1/chat/completions";
constexpr uint32_t kWifiTimeoutMs = 18000;
constexpr uint32_t kNtpTimeoutMs = 12000;
constexpr uint32_t kApiTimeoutMs = 60000;
constexpr size_t kMaxInputChars = 900;
constexpr size_t kMaxStoredMessageChars = 1400;
constexpr size_t kMaxHistoryMessages = 8;  // four user/assistant turns
constexpr int kMaxCompletionTokens = 2000;

// Current api.openai.com certificates may chain through Let's Encrypt or
// Google Trust Services. These roots are intentionally bundled instead of
// disabling certificate validation.
static const char kRootCAs[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw
CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg
R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00
MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT
ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW
+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9
ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T
AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI
zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW
tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1
/q4AaOeMSQ+2b1tbFfLn
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)EOF";

WebServer web(80);

constexpr int SD_SPI_SCK_PIN = 40;
constexpr int SD_SPI_MISO_PIN = 39;
constexpr int SD_SPI_MOSI_PIN = 14;
constexpr int SD_SPI_CS_PIN = 12;
constexpr char kConfigDir[] = "/gptputer";
constexpr char kConfigPath[] = "/gptputer/config.json";
constexpr char kConfigTmpPath[] = "/gptputer/config.tmp";
bool sdReady = false;

struct ChatMessage {
  String role;
  String content;
};
std::vector<ChatMessage> history;
std::vector<String> screenLines;
size_t scrollTop = 0;
bool manualScroll = false;
constexpr size_t kMaxScreenLines = 240;

String wifiSsid;
String wifiPass;
String apiKey;
String model = "gpt-4.1-mini";
String input;
enum class AppMode { WIFI_SCAN, WIFI_MANUAL_SSID, WIFI_PASS, WEB_SETUP, CHAT };
AppMode appMode = AppMode::CHAT;
String wifiEntry;
struct WifiNetwork {
  String ssid;
  int32_t rssi;
};
std::vector<WifiNetwork> wifiNetworks;
size_t wifiPage = 0;
constexpr size_t kWifiPerPage = 7;
bool webStarted = false;
bool webRoutesRegistered = false;
bool webConfigSaved = false;

String htmlEscape(const String &s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); ++i) {
    const char c = s[i];
    switch (c) {
      case '&': out += F("&amp;"); break;
      case '<': out += F("&lt;"); break;
      case '>': out += F("&gt;"); break;
      case '"': out += F("&quot;"); break;
      case '\'': out += F("&#39;"); break;
      default: out += c; break;
    }
  }
  return out;
}

// UTF-8 display layer ---------------------------------------------------------
// The default tiny M5GFX font is kept to preserve the exact v11 layout and
// memory footprint. UTF-8 is decoded to Unicode code points, then common
// Portuguese/Czech/Latin accents are composed above/below the base ASCII
// glyph. Unsupported scripts/emoji fall back to '?'. The original UTF-8 text
// remains untouched in the conversation sent to OpenAI.
enum AccentType {
  ACC_NONE,
  ACC_ACUTE,
  ACC_GRAVE,
  ACC_CARON,
  ACC_CIRC,
  ACC_TILDE,
  ACC_RING,
  ACC_CEDILLA,
  ACC_DIAERESIS
};

uint32_t nextCodepoint(const String &s, size_t &i) {
  if (i >= s.length()) return 0;
  const uint8_t c = (uint8_t)s[i++];
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0 && i < s.length()) {
    const uint8_t c2 = (uint8_t)s[i++];
    if ((c2 & 0xC0) != 0x80) return '?';
    return ((c & 0x1F) << 6) | (c2 & 0x3F);
  }
  if ((c & 0xF0) == 0xE0 && i + 1 < s.length()) {
    const uint8_t c2 = (uint8_t)s[i++];
    const uint8_t c3 = (uint8_t)s[i++];
    if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return '?';
    return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
  }
  if ((c & 0xF8) == 0xF0 && i + 2 < s.length()) {
    const uint8_t c2 = (uint8_t)s[i++];
    const uint8_t c3 = (uint8_t)s[i++];
    const uint8_t c4 = (uint8_t)s[i++];
    if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) return '?';
    // The tiny font has no emoji/astral glyphs. Consume the whole sequence.
    return '?';
  }
  return '?';
}

size_t utf8Length(const String &s) {
  size_t n = 0, i = 0;
  while (i < s.length()) { nextCodepoint(s, i); ++n; }
  return n;
}

String utf8Slice(const String &s, size_t startCp, size_t countCp) {
  if (countCp == 0) return "";
  size_t i = 0, cp = 0;
  while (i < s.length() && cp < startCp) { nextCodepoint(s, i); ++cp; }
  const size_t byteStart = i;
  while (i < s.length() && cp < startCp + countCp) { nextCodepoint(s, i); ++cp; }
  return s.substring(byteStart, i);
}

String utf8Tail(const String &s, size_t countCp) {
  const size_t len = utf8Length(s);
  if (len <= countCp) return s;
  return utf8Slice(s, len - countCp, countCp);
}

void utf8PopBack(String &s) {
  if (s.isEmpty()) return;
  size_t i = s.length() - 1;
  while (i > 0 && (((uint8_t)s[i] & 0xC0) == 0x80)) --i;
  s.remove(i);
}

bool latinMap(uint32_t cp, char &base, AccentType &accent) {
  accent = ACC_NONE;
  switch (cp) {
    // Portuguese + common Western European Latin.
    case 0x00E1: base='a'; accent=ACC_ACUTE; return true; case 0x00C1: base='A'; accent=ACC_ACUTE; return true;
    case 0x00E0: base='a'; accent=ACC_GRAVE; return true; case 0x00C0: base='A'; accent=ACC_GRAVE; return true;
    case 0x00E2: base='a'; accent=ACC_CIRC; return true; case 0x00C2: base='A'; accent=ACC_CIRC; return true;
    case 0x00E3: base='a'; accent=ACC_TILDE; return true; case 0x00C3: base='A'; accent=ACC_TILDE; return true;
    case 0x00E4: base='a'; accent=ACC_DIAERESIS; return true; case 0x00C4: base='A'; accent=ACC_DIAERESIS; return true;
    case 0x00E9: base='e'; accent=ACC_ACUTE; return true; case 0x00C9: base='E'; accent=ACC_ACUTE; return true;
    case 0x00E8: base='e'; accent=ACC_GRAVE; return true; case 0x00C8: base='E'; accent=ACC_GRAVE; return true;
    case 0x00EA: base='e'; accent=ACC_CIRC; return true; case 0x00CA: base='E'; accent=ACC_CIRC; return true;
    case 0x00EB: base='e'; accent=ACC_DIAERESIS; return true; case 0x00CB: base='E'; accent=ACC_DIAERESIS; return true;
    case 0x00ED: base='i'; accent=ACC_ACUTE; return true; case 0x00CD: base='I'; accent=ACC_ACUTE; return true;
    case 0x00EC: base='i'; accent=ACC_GRAVE; return true; case 0x00CC: base='I'; accent=ACC_GRAVE; return true;
    case 0x00EE: base='i'; accent=ACC_CIRC; return true; case 0x00CE: base='I'; accent=ACC_CIRC; return true;
    case 0x00EF: base='i'; accent=ACC_DIAERESIS; return true; case 0x00CF: base='I'; accent=ACC_DIAERESIS; return true;
    case 0x00F3: base='o'; accent=ACC_ACUTE; return true; case 0x00D3: base='O'; accent=ACC_ACUTE; return true;
    case 0x00F2: base='o'; accent=ACC_GRAVE; return true; case 0x00D2: base='O'; accent=ACC_GRAVE; return true;
    case 0x00F4: base='o'; accent=ACC_CIRC; return true; case 0x00D4: base='O'; accent=ACC_CIRC; return true;
    case 0x00F5: base='o'; accent=ACC_TILDE; return true; case 0x00D5: base='O'; accent=ACC_TILDE; return true;
    case 0x00F6: base='o'; accent=ACC_DIAERESIS; return true; case 0x00D6: base='O'; accent=ACC_DIAERESIS; return true;
    case 0x00FA: base='u'; accent=ACC_ACUTE; return true; case 0x00DA: base='U'; accent=ACC_ACUTE; return true;
    case 0x00F9: base='u'; accent=ACC_GRAVE; return true; case 0x00D9: base='U'; accent=ACC_GRAVE; return true;
    case 0x00FB: base='u'; accent=ACC_CIRC; return true; case 0x00DB: base='U'; accent=ACC_CIRC; return true;
    case 0x00FC: base='u'; accent=ACC_DIAERESIS; return true; case 0x00DC: base='U'; accent=ACC_DIAERESIS; return true;
    case 0x00E7: base='c'; accent=ACC_CEDILLA; return true; case 0x00C7: base='C'; accent=ACC_CEDILLA; return true;
    case 0x00F1: base='n'; accent=ACC_TILDE; return true; case 0x00D1: base='N'; accent=ACC_TILDE; return true;
    case 0x00FD: base='y'; accent=ACC_ACUTE; return true; case 0x00DD: base='Y'; accent=ACC_ACUTE; return true;

    // Czech Latin Extended-A.
    case 0x010D: base='c'; accent=ACC_CARON; return true; case 0x010C: base='C'; accent=ACC_CARON; return true;
    case 0x010F: base='d'; accent=ACC_CARON; return true; case 0x010E: base='D'; accent=ACC_CARON; return true;
    case 0x011B: base='e'; accent=ACC_CARON; return true; case 0x011A: base='E'; accent=ACC_CARON; return true;
    case 0x0148: base='n'; accent=ACC_CARON; return true; case 0x0147: base='N'; accent=ACC_CARON; return true;
    case 0x0159: base='r'; accent=ACC_CARON; return true; case 0x0158: base='R'; accent=ACC_CARON; return true;
    case 0x0161: base='s'; accent=ACC_CARON; return true; case 0x0160: base='S'; accent=ACC_CARON; return true;
    case 0x0165: base='t'; accent=ACC_CARON; return true; case 0x0164: base='T'; accent=ACC_CARON; return true;
    case 0x016F: base='u'; accent=ACC_RING; return true; case 0x016E: base='U'; accent=ACC_RING; return true;
    case 0x017E: base='z'; accent=ACC_CARON; return true; case 0x017D: base='Z'; accent=ACC_CARON; return true;
    default: return false;
  }
}

char punctuationFallback(uint32_t cp) {
  switch (cp) {
    case 0x2013: case 0x2014: return '-';  // en/em dash
    case 0x2018: case 0x2019: return '\'';
    case 0x201C: case 0x201D: return '"';
    case 0x2022: return '*';
    case 0x2026: return '.';
    case 0x00A0: return ' ';
    default: return '?';
  }
}

void drawAccent(int x, int y, AccentType accent, uint16_t color) {
  auto &d = M5Cardputer.Display;
  switch (accent) {
    case ACC_ACUTE: d.drawLine(x+3,y+1,x+4,y,color); break;
    case ACC_GRAVE: d.drawLine(x+1,y,x+2,y+1,color); break;
    case ACC_CARON: d.drawPixel(x+1,y,color); d.drawPixel(x+2,y+1,color); d.drawPixel(x+3,y,color); break;
    case ACC_CIRC: d.drawPixel(x+1,y+1,color); d.drawPixel(x+2,y,color); d.drawPixel(x+3,y+1,color); break;
    case ACC_TILDE: d.drawPixel(x+1,y+1,color); d.drawPixel(x+2,y,color); d.drawPixel(x+3,y,color); d.drawPixel(x+4,y+1,color); break;
    case ACC_RING: d.drawCircle(x+2,y+1,1,color); break;
    case ACC_CEDILLA: d.drawPixel(x+2,y+9,color); d.drawPixel(x+3,y+10,color); break;
    case ACC_DIAERESIS: d.drawPixel(x+1,y,color); d.drawPixel(x+4,y,color); break;
    default: break;
  }
}

// Draws one fixed 6px cell per Unicode code point, preserving the v11 layout.
void drawUtf8String(const String &text, int x, int y,
                    uint16_t color = TFT_WHITE, uint16_t bg = TFT_BLACK,
                    size_t maxCells = SIZE_MAX) {
  auto &d = M5Cardputer.Display;
  d.setTextSize(1);
  d.setTextColor(color, bg);
  size_t i = 0, col = 0;
  while (i < text.length() && col < maxCells) {
    const uint32_t cp = nextCodepoint(text, i);
    char base = '?';
    AccentType accent = ACC_NONE;
    if (cp >= 32 && cp < 127) base = (char)cp;
    else if (!latinMap(cp, base, accent)) base = punctuationFallback(cp);
    d.setCursor(x + (int)col * 6, y + 2);
    d.print(base);
    if (accent != ACC_NONE) drawAccent(x + (int)col * 6, y, accent, color);
    ++col;
  }
}

// ASCII-only fallback used by the legacy Wi-Fi/setup screens. Chat transcript
// rendering does NOT call this function anymore.
String displaySafe(String s) {
  const char *from[] = {
      "á","à","â","ã","ä","Á","À","Â","Ã","Ä",
      "é","è","ê","ë","É","È","Ê","Ë",
      "í","ì","î","ï","Í","Ì","Î","Ï",
      "ó","ò","ô","õ","ö","Ó","Ò","Ô","Õ","Ö",
      "ú","ù","û","ü","Ú","Ù","Û","Ü",
      "ç","Ç","ñ","Ñ","č","Č","ď","Ď","ě","Ě","ň","Ň",
      "ř","Ř","š","Š","ť","Ť","ů","Ů","ý","Ý","ž","Ž",
      "–","—","“","”","‘","’","…","•"};
  const char *to[] = {
      "a","a","a","a","a","A","A","A","A","A",
      "e","e","e","e","E","E","E","E",
      "i","i","i","i","I","I","I","I",
      "o","o","o","o","o","O","O","O","O","O",
      "u","u","u","u","U","U","U","U",
      "c","C","n","N","c","C","d","D","e","E","n","N",
      "r","R","s","S","t","T","u","U","y","Y","z","Z",
      "-","-","\"","\"","'","'","...","*"};
  constexpr size_t count = sizeof(from) / sizeof(from[0]);
  for (size_t i = 0; i < count; ++i) s.replace(from[i], to[i]);
  return s;
}

String clippedForHistory(const String &s) {
  if (utf8Length(s) <= kMaxStoredMessageChars) return s;
  return utf8Slice(s, 0, kMaxStoredMessageChars) + "...";
}

void trimHistory() {
  while (history.size() > kMaxHistoryMessages) history.erase(history.begin());
}

void renderHeader(const String &status = "ONLINE") {
  auto &d = M5Cardputer.Display;
  d.fillRect(0, 0, d.width(), 14, TFT_DARKGREY);
  d.setTextColor(TFT_WHITE, TFT_DARKGREY);
  d.setTextSize(1);
  d.setCursor(2, 3);
  d.print("GPTputer ");
  String shortModel = model;
  shortModel.replace("gpt-5.6-", "");
  d.print(shortModel);
  d.setCursor(d.width() - 48, 3);
  d.print(status.substring(0, 7));
}

void renderInput() {
  auto &d = M5Cardputer.Display;
  const int y = d.height() - 25;
  d.fillRect(0, y, d.width(), 25, TFT_BLACK);
  d.drawFastHLine(0, y, d.width(), TFT_DARKGREY);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  String shown = input;
  const size_t maxShown = 36;
  if (utf8Length(shown) > maxShown) shown = "..." + utf8Tail(shown, maxShown - 3);
  drawUtf8String(String("> ") + shown, 2, y + 5, TFT_WHITE, TFT_BLACK, maxShown + 2);
}

constexpr size_t kScreenCols = 38;
constexpr size_t kScreenRows = 10;

size_t tailScrollTop() {
  return screenLines.size() > kScreenRows ? screenLines.size() - kScreenRows : 0;
}

size_t maxScrollTop() {
  // Manual mode may place the first line of a response at the top even when
  // fewer than kScreenRows lines remain below it. Blank space is intentional.
  return screenLines.empty() ? 0 : screenLines.size() - 1;
}

void clampScrollTop() {
  const size_t maxTop = maxScrollTop();
  if (scrollTop > maxTop) scrollTop = maxTop;
}

void redrawTranscript() {
  auto &d = M5Cardputer.Display;
  const int top = 14;
  const int bottom = d.height() - 25;
  d.fillRect(0, top, d.width(), bottom - top, TFT_BLACK);
  d.setTextSize(1);
  d.setTextColor(TFT_WHITE, TFT_BLACK);

  clampScrollTop();
  int y = top + 2;
  const size_t end = std::min(screenLines.size(), scrollTop + kScreenRows);
  for (size_t i = scrollTop; i < end; ++i) {
    drawUtf8String(screenLines[i], 2, y - 2, TFT_WHITE, TFT_BLACK, kScreenCols);
    y += 9;
  }

  // Small position indicator on the right edge: ^ when more above, v when more below.
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  if (scrollTop > 0) { d.setCursor(d.width() - 7, top + 1); d.print("^"); }
  if (scrollTop + kScreenRows < screenLines.size()) {
    d.setCursor(d.width() - 7, bottom - 10); d.print("v");
  }
}

void setScrollTop(size_t line, bool manual = true) {
  scrollTop = line;
  manualScroll = manual;
  clampScrollTop();
  redrawTranscript();
}

void scrollUp(size_t lines = 1) {
  if (scrollTop >= lines) scrollTop -= lines;
  else scrollTop = 0;
  manualScroll = true;
  redrawTranscript();
}

void scrollDown(size_t lines = 1) {
  const size_t maxTop = maxScrollTop();
  scrollTop = std::min(maxTop, scrollTop + lines);
  manualScroll = true;
  redrawTranscript();
}

void clearChat() {
  screenLines.clear();
  scrollTop = 0;
  manualScroll = false;
  redrawTranscript();
}

void appendWrapped(const String &prefix, const String &raw) {
  String text = raw;  // Keep UTF-8 intact; rendering happens in drawUtf8String().
  text.replace("\r", "");
  String current = prefix;
  auto flush = [&]() {
    screenLines.push_back(current);
    current = "";
  };
  for (size_t i = 0; i < text.length();) {
    if (text[i] == '\n') { flush(); ++i; continue; }
    size_t j = i;
    while (j < text.length() && text[j] != ' ' && text[j] != '\n') ++j;
    String word = text.substring(i, j);  // byte boundaries are safe: delimiters are ASCII
    size_t wordCells = utf8Length(word);
    if (wordCells > kScreenCols) {
      if (!current.isEmpty()) flush();
      while (utf8Length(word) > kScreenCols) {
        screenLines.push_back(utf8Slice(word, 0, kScreenCols));
        word = utf8Slice(word, kScreenCols, utf8Length(word) - kScreenCols);
      }
      current = word;
    } else if (!word.isEmpty()) {
      const size_t currentCells = utf8Length(current);
      const size_t extra = current.isEmpty() ? wordCells : wordCells + 1;
      if (currentCells + extra > kScreenCols) flush();
      if (!current.isEmpty()) current += ' ';
      current += word;
    }
    while (j < text.length() && text[j] == ' ') ++j;
    i = j;
  }
  if (!current.isEmpty()) flush();
}

// Removes only old transcript lines. The caller can protect the beginning of a
// newly-added response so its first line is never discarded before display.
void trimScreenLines(size_t protectedStart = SIZE_MAX) {
  if (screenLines.size() <= kMaxScreenLines) return;
  size_t removeCount = screenLines.size() - kMaxScreenLines;
  if (protectedStart != SIZE_MAX) removeCount = std::min(removeCount, protectedStart);
  if (removeCount == 0) return;
  screenLines.erase(screenLines.begin(), screenLines.begin() + removeCount);
  if (scrollTop >= removeCount) scrollTop -= removeCount;
  else scrollTop = 0;
}

size_t appendChat(const String &who, const String &text, bool showFromStart = false) {
  const size_t before = screenLines.size();
  appendWrapped(who + ":", text);
  screenLines.push_back("");
  const size_t added = screenLines.size() - before;

  // Keep the current response intact whenever possible; discard older lines first.
  if (screenLines.size() > kMaxScreenLines) {
    const size_t overflow = screenLines.size() - kMaxScreenLines;
    const size_t removableOld = before;
    const size_t removeCount = std::min(overflow, removableOld);
    if (removeCount) {
      screenLines.erase(screenLines.begin(), screenLines.begin() + removeCount);
      if (scrollTop >= removeCount) scrollTop -= removeCount;
      else scrollTop = 0;
    }
  }

  const size_t responseStart = screenLines.size() >= added ? screenLines.size() - added : 0;
  if (showFromStart) {
    // Important UX rule: every GPT answer opens on its FIRST line.
    scrollTop = responseStart;
    manualScroll = true;
  } else if (!manualScroll) {
    scrollTop = tailScrollTop();
  }
  redrawTranscript();
  return responseStart;
}

void showHelp() {
  appendChat("SYS", "Scroll: Fn+; up | Fn+. down. Commands: /new /setup /wifi /model MODEL_ID");
}

bool initStorage() {
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) return false;
  if (SD.cardType() == CARD_NONE) return false;
  if (!SD.exists(kConfigDir) && !SD.mkdir(kConfigDir)) return false;
  sdReady = true;
  return true;
}

bool readSettingsFile(String &outSsid, String &outPass, String &outKey, String &outModel) {
  if (!sdReady || !SD.exists(kConfigPath)) return false;
  File f = SD.open(kConfigPath, FILE_READ);
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  outSsid = String((const char *)(doc["ssid"] | ""));
  outPass = String((const char *)(doc["pass"] | ""));
  outKey = String((const char *)(doc["apikey"] | ""));
  outModel = String((const char *)(doc["model"] | "gpt-4.1-mini"));
  return true;
}

bool saveSettingsFile() {
  if (!sdReady) return false;
  JsonDocument doc;
  doc["ssid"] = wifiSsid;
  doc["pass"] = wifiPass;
  doc["apikey"] = apiKey;
  doc["model"] = model;

  if (SD.exists(kConfigTmpPath)) SD.remove(kConfigTmpPath);
  File f = SD.open(kConfigTmpPath, FILE_WRITE);
  if (!f) return false;
  size_t written = serializeJson(doc, f);
  f.flush();
  f.close();
  if (written == 0) { SD.remove(kConfigTmpPath); return false; }

  // Verify the temporary file before replacing the previous config.
  File vf = SD.open(kConfigTmpPath, FILE_READ);
  if (!vf) { SD.remove(kConfigTmpPath); return false; }
  JsonDocument verify;
  DeserializationError err = deserializeJson(verify, vf);
  vf.close();
  if (err) { SD.remove(kConfigTmpPath); return false; }
  String vk = String((const char *)(verify["apikey"] | ""));
  String vm = String((const char *)(verify["model"] | ""));
  String vs = String((const char *)(verify["ssid"] | ""));
  if (vk != apiKey || vm != model || vs != wifiSsid) {
    SD.remove(kConfigTmpPath);
    return false;
  }

  if (SD.exists(kConfigPath)) SD.remove(kConfigPath);
  if (!SD.rename(kConfigTmpPath, kConfigPath)) return false;
  return true;
}

void loadSettings() {
  String s, p, k, m;
  if (readSettingsFile(s, p, k, m)) {
    wifiSsid = s;
    wifiPass = p;
    apiKey = k;
    model = m;
  } else {
    wifiSsid = "";
    wifiPass = "";
    apiKey = "";
    model = "gpt-4.1-mini";
  }
  if (model.length() < 3 || model.length() > 80) model = "gpt-4.1-mini";
}

void saveModel() {
  saveSettingsFile();
}

String setupPage() {
  String h;
  h.reserve(6200);
  h += F("<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
         "<title>GPTputer ADV Setup</title><style>body{font-family:sans-serif;max-width:560px;margin:28px auto;padding:0 16px;background:#111;color:#eee}"
         "input,button{box-sizing:border-box;width:100%;padding:12px;margin:6px 0 16px;border-radius:8px;border:1px solid #555;background:#222;color:#fff}"
         "button{background:#fff;color:#111;font-weight:700}.row{display:flex;gap:8px;align-items:center}.row input{width:auto;margin:0}.muted{color:#aaa}.ok{color:#6f6}.err{color:#f88}"
         "pre{white-space:pre-wrap;background:#1a1a1a;padding:10px;border-radius:8px}</style></head><body>"
         "<h2>GPTputer ADV</h2><p class=ok>Connected to Wi-Fi: ");
  h += htmlEscape(WiFi.SSID());
  h += F("</p><p>Cardputer IP: <b>");
  h += WiFi.localIP().toString();
  h += F("</b></p><form id=cfg>"
         "<label>OpenAI API key</label><input id=apikey name=apikey type=password autocomplete=off autocapitalize=off spellcheck=false placeholder='sk-... (leave blank to keep saved key)'>"
         "<div class=row><input id=showkey type=checkbox><label for=showkey>Show key while typing</label></div><br>"
         "<div class=muted>The key is stored locally in /gptputer/config.json on the microSD. It is never echoed back by this page.</div><br>"
         "<label>Model</label><input id=model name=model type=text value='");
  h += htmlEscape(model);
  h += F("' placeholder='gpt-4.1-mini'><button id=save type=submit>Save and verify</button></form>"
         "<pre id=status class=muted>Waiting for configuration.</pre>"
         "<script>"
         "const key=document.getElementById('apikey'),model=document.getElementById('model'),status=document.getElementById('status'),btn=document.getElementById('save');"
         "document.getElementById('showkey').addEventListener('change',e=>key.type=e.target.checked?'text':'password');"
         "document.getElementById('cfg').addEventListener('submit',async e=>{e.preventDefault();btn.disabled=true;status.className='muted';status.textContent='Saving and verifying...';"
         "try{const body=key.value.trim()+'\\n'+model.value.trim();const r=await fetch('/save',{method:'POST',headers:{'Content-Type':'text/plain;charset=UTF-8'},body});"
         "const t=await r.text();status.textContent=t;status.className=r.ok?'ok':'err';if(r.ok)key.value='';}catch(err){status.textContent='Browser request failed: '+err;status.className='err';}finally{btn.disabled=false;}});"
         "</script><p class=muted>Use any text-capable model enabled for your OpenAI API project.</p></body></html>");
  return h;
}

void drawWifiEntryScreen(bool password) {
  auto &d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setCursor(6, 8);
  d.println("GPTputer ADV - Wi-Fi");
  d.println();
  if (password) {
    d.print("Network: ");
    d.println(displaySafe(wifiSsid));
    d.println("Password (visible):");
  } else {
    d.println("Wi-Fi SSID (manual):");
  }
  d.println();
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.print(displaySafe(wifiEntry));
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.println();
  d.println();
  d.println("ENTER = continue");
  d.println("DEL = erase");
}

void drawWifiScanScreen() {
  auto &d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setCursor(4, 4);
  d.println("GPTputer ADV - Wi-Fi");

  if (wifiNetworks.empty()) {
    d.println();
    d.setTextColor(TFT_YELLOW, TFT_BLACK);
    d.println("No Wi-Fi networks found.");
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.println();
    d.println("R = scan again");
    d.println("0 = manual SSID");
    return;
  }

  const size_t pageCount = (wifiNetworks.size() + kWifiPerPage - 1) / kWifiPerPage;
  if (wifiPage >= pageCount) wifiPage = pageCount - 1;
  const size_t start = wifiPage * kWifiPerPage;
  const size_t candidateEnd = start + kWifiPerPage;
  const size_t end = candidateEnd < wifiNetworks.size() ? candidateEnd : wifiNetworks.size();
  d.printf("Networks %u-%u/%u  page %u/%u\n",
           (unsigned)(start + 1), (unsigned)end, (unsigned)wifiNetworks.size(),
           (unsigned)(wifiPage + 1), (unsigned)pageCount);

  for (size_t i = start; i < end; ++i) {
    const size_t local = i - start + 1;
    String name = displaySafe(wifiNetworks[i].ssid);
    if (name.length() > 23) name = name.substring(0, 23);
    d.setTextColor(TFT_CYAN, TFT_BLACK);
    d.printf("%u ", (unsigned)local);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.print(name);
    d.setTextColor(TFT_DARKGREY, TFT_BLACK);
    d.printf(" %ld\n", (long)wifiNetworks[i].rssi);
  }

  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.println("1-7 select | R rescan");
  if (pageCount > 1) d.println("N next | P previous");
  d.println("0 manual SSID");
}

void scanWifiNetworks() {
  auto &d = M5Cardputer.Display;
  appMode = AppMode::WIFI_SCAN;
  wifiPage = 0;
  wifiNetworks.clear();

  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setCursor(6, 8);
  d.println("GPTputer ADV - Wi-Fi");
  d.println();
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.println("Scanning networks...");

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  delay(100);
  int count = WiFi.scanNetworks(false, true);
  for (int i = 0; i < count; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.trim();
    if (ssid.length() == 0) continue;
    int32_t rssi = WiFi.RSSI(i);
    bool found = false;
    for (auto &net : wifiNetworks) {
      if (net.ssid == ssid) {
        if (rssi > net.rssi) net.rssi = rssi;
        found = true;
        break;
      }
    }
    if (!found) wifiNetworks.push_back({ssid, rssi});
  }
  WiFi.scanDelete();

  // Strongest networks first, without relying on extra STL features.
  for (size_t i = 0; i < wifiNetworks.size(); ++i) {
    for (size_t j = i + 1; j < wifiNetworks.size(); ++j) {
      if (wifiNetworks[j].rssi > wifiNetworks[i].rssi) {
        WifiNetwork tmp = wifiNetworks[i];
        wifiNetworks[i] = wifiNetworks[j];
        wifiNetworks[j] = tmp;
      }
    }
  }
  drawWifiScanScreen();
}

void beginWifiEntry() {
  if (webStarted) {
    web.stop();
    webStarted = false;
  }
  scanWifiNetworks();
}

void drawWebSetupScreen() {
  auto &d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setCursor(6, 8);
  d.println("GPTputer ADV SETUP");
  d.println();
  d.print("Wi-Fi: "); d.println(displaySafe(WiFi.SSID()));
  d.print("IP: ");
  d.setTextColor(TFT_GREEN, TFT_BLACK);
  d.println(WiFi.localIP().toString());
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.println();
  d.println("On PC/tablet open:");
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.print("http://");
  d.println(WiFi.localIP().toString());
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.println();
  d.println("Enter API key + model there.");
}

void startLanWebServer() {
  if (webStarted) return;
  if (!webRoutesRegistered) {
    web.on("/", HTTP_GET, []() { web.send(200, "text/html", setupPage()); });

    web.on("/save", HTTP_POST, []() {
      String newKey;
      String newModel;

      // Primary path: the setup page sends one raw text/plain body. This avoids
      // browser/form parser edge cases with long API keys. Legacy form fields
      // remain supported as a fallback.
      if (web.hasArg("plain")) {
        String body = web.arg("plain");
        int split = body.indexOf('\n');
        if (split >= 0) {
          newKey = body.substring(0, split);
          newModel = body.substring(split + 1);
        } else {
          newKey = body;
        }
      } else {
        newKey = web.arg("apikey");
        newModel = web.arg("model");
      }

      newKey.trim();
      newModel.trim();
      if (newModel.length() < 3 || newModel.length() > 80 || newModel.indexOf(' ') >= 0) {
        newModel = model.length() >= 3 ? model : "gpt-4.1-mini";
      }

      const size_t receivedLen = newKey.length();
      const String previousKey = apiKey;
      const String previousModel = model;
      if (receivedLen != 0) apiKey = newKey;
      model = newModel;

      const bool writeOk = saveSettingsFile();

      // Read back from SD and compare. Never echo the secret itself.
      String loadedSsid, loadedPass, loadedKey, loadedModel;
      const bool readOk = readSettingsFile(loadedSsid, loadedPass, loadedKey, loadedModel);
      const bool keyPresent = loadedKey.length() >= 20;
      const bool keyMatches = readOk && ((receivedLen == 0) ? keyPresent : (loadedKey == newKey));
      const bool modelMatches = readOk && loadedModel == newModel;

      String diag;
      diag.reserve(480);
      diag += "GPTputer configuration check\n\n";
      diag += "storage=SD:/gptputer/config.json\n";
      diag += "sd_ready=" + String(sdReady ? "yes" : "NO") + "\n";
      diag += "received_key_chars=" + String(receivedLen) + "\n";
      diag += "stored_key_chars=" + String(loadedKey.length()) + "\n";
      diag += "file_write=" + String(writeOk ? "yes" : "NO") + "\n";
      diag += "file_read=" + String(readOk ? "yes" : "NO") + "\n";
      diag += "key_verified=" + String(keyMatches ? "yes" : "NO") + "\n";
      diag += "model_verified=" + String(modelMatches ? "yes" : "NO") + "\n";
      diag += "model=" + loadedModel + "\n\n";

      if (!sdReady) {
        apiKey = previousKey; model = previousModel;
        diag += "FAILED: microSD storage is not available.";
        web.send(500, "text/plain", diag);
        return;
      }
      if (!writeOk || !readOk) {
        apiKey = previousKey; model = previousModel;
        diag += "FAILED: could not write/read /gptputer/config.json on the microSD.";
        web.send(500, "text/plain", diag);
        return;
      }
      if (!keyPresent) {
        apiKey = previousKey; model = previousModel;
        diag += receivedLen == 0
                  ? "FAILED: no API key was received and no valid key is stored."
                  : "FAILED: the API key was received but was not stored correctly.";
        web.send(400, "text/plain", diag);
        return;
      }
      if (!keyMatches || !modelMatches) {
        apiKey = previousKey; model = previousModel;
        diag += "FAILED: SD read-back does not match the submitted configuration.";
        web.send(500, "text/plain", diag);
        return;
      }

      apiKey = loadedKey;
      model = loadedModel;
      diag += "OK: API key and model were saved and read back successfully.\nYou can return to the GPTputer now.";
      web.send(200, "text/plain", diag);
      webConfigSaved = true;
    });

    web.onNotFound([]() {
      web.sendHeader("Location", "/", true);
      web.send(302, "text/plain", "");
    });
    webRoutesRegistered = true;
  }
  web.begin();
  webStarted = true;
}

bool syncClock() {
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com", "time.google.com");
  const uint32_t started = millis();
  time_t now = time(nullptr);
  while (now < 1700000000 && millis() - started < kNtpTimeoutMs) {
    delay(250);
    now = time(nullptr);
  }
  return now >= 1700000000;
}

bool connectWifi() {
  if (wifiSsid.length() == 0) return false;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < kWifiTimeoutMs) {
    renderHeader("WIFI...");
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) return false;
  return true;
}

String apiErrorText(int httpCode, const String &payload) {
  JsonDocument doc;
  if (deserializeJson(doc, payload) == DeserializationError::Ok) {
    const char *msg = doc["error"]["message"] | nullptr;
    if (msg) return String("API ") + httpCode + ": " + msg;
  }
  if (httpCode == 401) return "API 401: invalid API key.";
  if (httpCode == 429) return "API 429: rate/spend limit reached.";
  return String("API HTTP error ") + httpCode;
}

bool askOpenAI(const String &prompt, String &answer, String &errorText) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    const uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 8000) delay(150);
  }
  if (WiFi.status() != WL_CONNECTED) {
    errorText = "Wi-Fi disconnected. Use /wifi if credentials changed.";
    return false;
  }

  if (time(nullptr) < 1700000000) {
    renderHeader("TIME...");
    if (!syncClock()) {
      errorText = "Could not synchronize clock for HTTPS.";
      return false;
    }
  }

  JsonDocument request;
  request["model"] = model;
  request["max_completion_tokens"] = kMaxCompletionTokens;

  // GPT-5 mini is a reasoning model. Chat Completions counts hidden reasoning
  // tokens against max_completion_tokens, so medium reasoning can consume the
  // whole budget before any visible text is emitted. Keep reasoning minimal
  // for this tiny-display chat client. Do not send this parameter to models
  // that may not support the same value.
  if (model == "gpt-5-mini" || model.startsWith("gpt-5-mini-") || model == "gpt-5") {
    request["reasoning_effort"] = "minimal";
  }
  JsonArray messages = request["messages"].to<JsonArray>();

  JsonObject system = messages.add<JsonObject>();
  system["role"] = "system";
  system["content"] = "You are GPTputer, a helpful assistant shown on a tiny 240x135 text display. Reply in the user's language. Be concise unless detail is requested. Avoid markdown tables.";

  for (const auto &m : history) {
    JsonObject item = messages.add<JsonObject>();
    item["role"] = m.role;
    item["content"] = m.content;
  }
  JsonObject current = messages.add<JsonObject>();
  current["role"] = "user";
  current["content"] = prompt;

  String body;
  serializeJson(request, body);

  WiFiClientSecure client;
  client.setCACert(kRootCAs);
  client.setHandshakeTimeout(15);

  HTTPClient http;
  http.setConnectTimeout(15000);
  http.setTimeout(kApiTimeoutMs);
  if (!http.begin(client, kApiUrl)) {
    errorText = "Could not start HTTPS connection.";
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", String("Bearer ") + apiKey);

  const int code = http.POST(body);
  const String payload = http.getString();
  http.end();

  if (code != 200) {
    errorText = apiErrorText(code, payload);
    return false;
  }

  JsonDocument response;
  const DeserializationError jsonErr = deserializeJson(response, payload);
  if (jsonErr) {
    errorText = String("Bad JSON response: ") + jsonErr.c_str();
    return false;
  }
  JsonVariantConst contentNode = response["choices"][0]["message"]["content"];
  String visibleText;

  // Normal Chat Completions text responses use a string. Keep a small fallback
  // for array-shaped content so future compatible responses do not look empty.
  if (contentNode.is<const char *>()) {
    const char *content = contentNode.as<const char *>();
    if (content) visibleText = content;
  } else if (contentNode.is<JsonArrayConst>()) {
    for (JsonVariantConst part : contentNode.as<JsonArrayConst>()) {
      const char *text = part["text"] | nullptr;
      if (!text) text = part["content"] | nullptr;
      if (text && *text) {
        if (visibleText.length()) visibleText += "\n";
        visibleText += text;
      }
    }
  }

  visibleText.trim();
  if (!visibleText.length()) {
    const char *finish = response["choices"][0]["finish_reason"] | "unknown";
    const long completionTokens = response["usage"]["completion_tokens"] | -1L;
    const long reasoningTokens = response["usage"]["completion_tokens_details"]["reasoning_tokens"] | -1L;
    errorText = String("No visible text. finish=") + finish;
    if (completionTokens >= 0) errorText += String(" completion=") + completionTokens;
    if (reasoningTokens >= 0) errorText += String(" reasoning=") + reasoningTokens;
    errorText += ". Try /model gpt-4.1-mini if this repeats.";
    return false;
  }

  answer = visibleText;
  return true;
}

void setModelCommand(String choice) {
  choice.trim();
  if (choice.length() < 3 || choice.length() > 80 || choice.indexOf(' ') >= 0) {
    appendChat("SYS", "Use /model MODEL_ID, e.g. /model gpt-4.1-mini");
    return;
  }
  model = choice;
  saveModel();
  renderHeader();
  appendChat("SYS", String("Model set to ") + model);
}

void handlePrompt(String prompt) {
  prompt.trim();
  if (prompt.length() == 0) return;

  if (prompt == "/new" || prompt == "/clear") {
    history.clear();
    clearChat();
    appendChat("SYS", "New chat.");
    return;
  }
  if (prompt == "/help") {
    showHelp();
    return;
  }
  if (prompt == "/setup") {
    appendChat("SYS", String("Setup: http://") + WiFi.localIP().toString());
    return;
  }
  if (prompt == "/wifi") {
    history.clear();
    beginWifiEntry();
    return;
  }
  if (prompt.startsWith("/model")) {
    setModelCommand(prompt.substring(6));
    return;
  }

  manualScroll = false;
  appendChat("YOU", prompt);
  renderHeader("ASKING");
  String answer, err;
  if (askOpenAI(prompt, answer, err)) {
    history.push_back({"user", clippedForHistory(prompt)});
    history.push_back({"assistant", clippedForHistory(answer)});
    trimHistory();
    appendChat("GPT", answer, true);
  } else {
    appendChat("ERR", err, true);
  }
  renderHeader();
}

void initUi() {
  auto &d = M5Cardputer.Display;
  d.setRotation(1);
  d.setBrightness(110);
  d.fillScreen(TFT_BLACK);
  d.setTextSize(1);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  screenLines.clear();
  scrollTop = 0;
  manualScroll = false;
  redrawTranscript();
  renderInput();
}

void bootStage(const char *stage, const String &detail = "") {
  auto &d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  d.setTextSize(1);
  d.setCursor(6, 8);
  d.println("GPTputer ADV v12 UTF-8");
  d.println();
  d.setTextColor(TFT_GREEN, TFT_BLACK);
  d.println(stage);
  d.setTextColor(TFT_WHITE, TFT_BLACK);
  if (detail.length()) { d.println(); d.println(detail); }
  delay(180);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(80);
  Serial.printf("\nGPTputer v12 UTF-8 reset_reason=%d\n", (int)esp_reset_reason());

  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  initUi();
  bootStage("BOOT 1: display OK", String("board=") + String((int)M5.getBoard()));

  if (!initStorage()) {
    bootStage("BOOT 2: SD ERROR", "microSD not available.\nReinsert SD and restart.");
    return;
  }
  bootStage("BOOT 2: SD OK");
  loadSettings();
  bootStage("BOOT 3: settings OK");
  renderHeader("BOOT");

  if (!connectWifi()) {
    bootStage("BOOT 4: Wi-Fi setup");
    beginWifiEntry();
    return;
  }

  startLanWebServer();
  if (apiKey.length() < 20) {
    appMode = AppMode::WEB_SETUP;
    drawWebSetupScreen();
    return;
  }

  appMode = AppMode::CHAT;
  renderHeader();
  appendChat("SYS", "Ready. Type a message and press Enter.");
  appendChat("SYS", String("Setup page: http://") + WiFi.localIP().toString());
  showHelp();
  renderInput();
}

void loop() {
  M5Cardputer.update();

  if (webStarted) {
    web.handleClient();
    if (webConfigSaved) {
      webConfigSaved = false;
      if (appMode == AppMode::WEB_SETUP) {
        appMode = AppMode::CHAT;
        clearChat();
        renderHeader();
        appendChat("SYS", "API configured. GPTputer is ready.");
        showHelp();
        renderInput();
      } else {
        renderHeader();
      }
    }
  }

  if (appMode == AppMode::WIFI_SCAN) {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      for (auto c : state.word) {
        if (c >= '1' && c <= '7') {
          const size_t chosen = wifiPage * kWifiPerPage + (size_t)(c - '1');
          if (chosen < wifiNetworks.size()) {
            wifiSsid = wifiNetworks[chosen].ssid;
            wifiEntry = "";
            appMode = AppMode::WIFI_PASS;
            drawWifiEntryScreen(true);
            delay(120);
            return;
          }
        } else if (c == '0') {
          wifiEntry = "";
          appMode = AppMode::WIFI_MANUAL_SSID;
          drawWifiEntryScreen(false);
          delay(120);
          return;
        } else if (c == 'r' || c == 'R') {
          scanWifiNetworks();
          delay(120);
          return;
        } else if (c == 'n' || c == 'N') {
          const size_t pageCount = (wifiNetworks.size() + kWifiPerPage - 1) / kWifiPerPage;
          if (pageCount > 1 && wifiPage + 1 < pageCount) ++wifiPage;
          drawWifiScanScreen();
          delay(120);
          return;
        } else if (c == 'p' || c == 'P') {
          if (wifiPage > 0) --wifiPage;
          drawWifiScanScreen();
          delay(120);
          return;
        }
      }
    }
    delay(5);
    return;
  }

  if (appMode == AppMode::WIFI_MANUAL_SSID || appMode == AppMode::WIFI_PASS) {
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();
      for (auto c : state.word) {
        if (wifiEntry.length() < 64) wifiEntry += c;
      }
      if (state.del && wifiEntry.length() != 0) wifiEntry.remove(wifiEntry.length() - 1);
      if (state.enter) {
        if (appMode == AppMode::WIFI_MANUAL_SSID) {
          wifiEntry.trim();
          if (wifiEntry.length() != 0) {
            wifiSsid = wifiEntry;
            wifiEntry = "";
            appMode = AppMode::WIFI_PASS;
            drawWifiEntryScreen(true);
            delay(150);
            return;
          }
        } else {
          wifiPass = wifiEntry;
          wifiEntry = "";
          saveSettingsFile();

          auto &d = M5Cardputer.Display;
          d.fillScreen(TFT_BLACK);
          d.setTextColor(TFT_WHITE, TFT_BLACK);
          d.setCursor(6, 8);
          d.println("Connecting to Wi-Fi...");
          d.println(displaySafe(wifiSsid));

          if (connectWifi()) {
            startLanWebServer();
            if (apiKey.length() < 20) {
              appMode = AppMode::WEB_SETUP;
              drawWebSetupScreen();
            } else {
              appMode = AppMode::CHAT;
              clearChat();
              renderHeader();
              appendChat("SYS", "Wi-Fi connected. Ready.");
              appendChat("SYS", String("Setup page: http://") + WiFi.localIP().toString());
              renderInput();
            }
          } else {
            d.fillScreen(TFT_BLACK);
            d.setTextColor(TFT_RED, TFT_BLACK);
            d.setCursor(6, 8);
            d.println("Wi-Fi connection failed.");
            d.setTextColor(TFT_WHITE, TFT_BLACK);
            d.println();
            d.println("Scanning again...");
            delay(900);
            beginWifiEntry();
          }
          delay(150);
          return;
        }
      }
      drawWifiEntryScreen(appMode == AppMode::WIFI_PASS);
    }
    delay(5);
    return;
  }

  if (appMode == AppMode::WEB_SETUP) {
    if (WiFi.status() != WL_CONNECTED) {
      beginWifiEntry();
    }
    delay(5);
    return;
  }

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    Keyboard_Class::KeysState state = M5Cardputer.Keyboard.keysState();

    // M5Cardputer 1.1.1 has `fn`, but not `up/down` fields. On the physical
    // keyboard the ; and . keys are the up/down arrows on the Fn layer.
    // Consume those combinations so they never become chat input.
    bool consumedScroll = false;
    if (state.fn) {
      for (auto c : state.word) {
        if (c == ';' || c == ':') { scrollUp(3); consumedScroll = true; }
        else if (c == '.' || c == '>') { scrollDown(3); consumedScroll = true; }
      }
      if (consumedScroll) {
        renderInput();
        delay(80);
        return;
      }
    }

    for (auto c : state.word) {
      if (input.length() < kMaxInputChars) input += c;
    }
    if (state.del && input.length() != 0) utf8PopBack(input);
    if (state.enter) {
      String prompt = input;
      input = "";
      renderInput();
      handlePrompt(prompt);
    }
    renderInput();
  }
  delay(5);
}
