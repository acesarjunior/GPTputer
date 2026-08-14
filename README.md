GPTputer ADV OpenAI v12 UTF-8 - SD-backed configuration
=================================================

Application-only firmware for M5Stack Cardputer ADV / M5Launcher.

WHAT CHANGED IN V11
V9 could successfully save the API key and call OpenAI, but gpt-5-mini could
return HTTP 200 with no visible assistant text when the completion budget was
consumed by reasoning tokens. V11 keeps the SD-backed configuration and also:

  - sets reasoning_effort=minimal for gpt-5-mini / gpt-5
  - raises max_completion_tokens from 700 to 2000
  - reports finish_reason, completion_tokens and reasoning_tokens if a response
    still contains no visible text
  - keeps gpt-4.1-mini as a fallback via /model gpt-4.1-mini

The firmware does not erase or modify the Launcher's NVS. Settings remain on:

  /gptputer/config.json

The firmware initializes the SD using the M5Stack-documented Cardputer/ADV pins:
SCK=40, MISO=39, MOSI=14, CS=12.

WI-FI FLOW
1. Launch the app.
2. If saved Wi-Fi cannot connect, nearby networks are scanned automatically.
3. Select a network with keys 1-7.
   N = next page, P = previous page, R = rescan, 0 = manual SSID.
4. Type the Wi-Fi password. It is intentionally shown in PLAIN TEXT on screen.
5. Press ENTER to connect.
6. After DHCP, open http://<cardputer-ip> from a PC/tablet on the same LAN.
7. Enter OpenAI API key + model ID.
8. The web page writes the config to SD, reads it back, and verifies it before
   allowing the app to enter chat mode.

COMMANDS
/new                clear conversation
/setup              show local setup URL again
/wifi               scan Wi-Fi networks again
/model MODEL_ID      change model and save to SD
/help                show commands

BUILD ON UBUNTU
./build.sh

OUTPUT
GPTputer_ADV_OpenAI_v12 UTF-8_app.bin

Install that application BIN with M5Launcher. Do not flash it at 0x0.

SECURITY
/gptputer/config.json contains the Wi-Fi password and OpenAI API key in plaintext
on the removable microSD. Use a dedicated OpenAI API key with a low spend limit.
OpenAI API traffic itself is HTTPS/TLS.


V11 SCROLL
- Every GPT answer opens at its first line.
- Fn + ; scrolls up 3 lines.
- Fn + . scrolls down 3 lines.
- The ^ / v indicators show when more text exists above/below.
- Scroll is manual; answers no longer jump to the tail.

SCROLL CONTROLS
---------------
Fn + ;   Scroll UP 3 lines (physical up-arrow key)
Fn + .   Scroll DOWN 3 lines (physical down-arrow key)

Behavior:
- When a GPT response arrives, its first line (GPT:) is placed at the top.
- The app never jumps directly to the end of a new GPT response.
- Scroll is manual while reading.
- ^ at the right edge means there is text above.
- v at the right edge means there is text below.
- Sending a new prompt moves to the newest prompt while waiting; when GPT replies, view resets to the start of that reply.


UTF-8 UPDATE (v12)
-------------------
- Keeps the v11 behavior: each GPT answer opens at its FIRST line.
- Scroll remains manual with Fn+; (up) and Fn+. (down).
- OpenAI request/response/history remain UTF-8 end-to-end.
- Chat display is UTF-8 codepoint-aware and supports Portuguese/Czech/Western Latin accents.
- UTF-8 strings are never sliced in the middle of a multibyte code point.
- Unsupported scripts/emoji display as '?' because the tiny built-in font is intentionally preserved.
- Wi-Fi/setup screens keep the v11 ASCII fallback to minimize unrelated changes.
