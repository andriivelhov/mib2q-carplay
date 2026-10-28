#!/bin/sh
# Protocol/ownership tests; no car, firmware flashing or proprietary SDK needed.
set -eu
cd "$(dirname "$0")/.."
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
SANITIZE=${SANITIZE:-}
cc -std=gnu11 -O1 -Wall -Wextra -Werror -Wno-unused-function $SANITIZE \
    tests/hw301_dongle_test.c -lpthread -ldl -o "$OUT/dongle"
"$OUT/dongle"
cc -std=gnu99 -O1 -Wall -Wextra -Werror -Wno-unused-function $SANITIZE \
    -Ialtscreen_hook/include tests/hw301_headunit_test.c \
    altscreen_hook/src/alt111_profile.c altscreen_hook/src/alt111_control.c \
    altscreen_hook/src/alt111_video.c altscreen_hook/src/alt111_resync.c \
    -lpthread -ldl -o "$OUT/headunit"
"$OUT/headunit"
