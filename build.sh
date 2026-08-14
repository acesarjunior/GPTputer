#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

ENV=cardputer_adv
OUTBIN=GPTputer_ADV_OpenAI_v12_UTF8_app.bin

# Prefer an already-installed PlatformIO. Otherwise create a local venv once.
if command -v pio >/dev/null 2>&1; then
  PIO=(pio)
elif python3 -c 'import platformio' >/dev/null 2>&1; then
  PIO=(python3 -m platformio)
else
  VENV="$PWD/.pio-venv"
  if [[ ! -x "$VENV/bin/pio" ]]; then
    echo "PlatformIO not found. Installing it in $VENV ..."
    python3 -m venv "$VENV"
    "$VENV/bin/pip" install platformio
  fi
  PIO=("$VENV/bin/pio")
fi

echo "== GPTputer ADV OpenAI v12 UTF-8 =="
echo "Cleaning old objects..."
rm -rf .pio/build
rm -f "$OUTBIN"

"${PIO[@]}" run -e "$ENV"
cp ".pio/build/$ENV/firmware.bin" "$OUTBIN"
python3 check_bin.py "$OUTBIN"

echo
echo "READY FOR M5LAUNCHER:"
echo "  $PWD/$OUTBIN"
