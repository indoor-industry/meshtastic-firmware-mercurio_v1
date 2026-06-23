# Works around two upstream meshtastic/device-ui bugs in URLService.cpp
# (the map tile fetcher, used whenever a tile isn't already on the SD
# card and has to come from WiFi):
#
#   1. bool URLService::load()'s single stream->readBytes(pngImage, len)
#      call only reliably drains one TLS record/chunk per call, silently
#      truncating any tile body bigger than that (observed truncating at
#      a fixed ~4KB boundary regardless of the tile's real size, e.g.
#      "4113 != 54917") instead of looping to read the rest.
#
#   2. load() never checked WiFi connectivity before attempting a
#      connection. With WiFi off, lwIP's TCP/IP stack is never brought
#      up: this used to crash outright, and (after fixing #1, which made
#      the failure path slower instead of instant) instead hangs - and
#      since all tile loading runs synchronously on the "tft" task while
#      it holds spiLock, a hung socket call there freezes the whole
#      display/touch for as long as the hang lasts.
#
# Neither is mercurio_v1-specific - any device-ui/MUI board fetching
# tiles over WiFi hits the same truncation bug, and any board without
# WiFi (or with it disabled) hits the same crash/freeze.
#
# Idempotent: skips if already patched, since .pio/libdeps is ephemeral.
Import("env")
import os

target = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"),
    env.subst("$PIOENV"),
    "meshtastic-device-ui",
    "source",
    "graphics",
    "map",
    "URLService.cpp",
)

MARKER = "mercurio_v1: tile-fetch read loop + WiFi-connected guard"

OLD_INCLUDE = "#ifdef ARDUINO_ARCH_ESP32\n\n// from ConvertPNG.c"
NEW_INCLUDE = '#ifdef ARDUINO_ARCH_ESP32\n#include <WiFi.h> // %s\n\n// from ConvertPNG.c' % MARKER

OLD_GUARD = """    // transform filename to provider url
    std::string url = TileProvider::url(name);"""
NEW_GUARD = """    // With WiFi off/disconnected, lwIP's TCP/IP stack was never brought up -
    // attempting a connection here previously crashed, and after fixing the
    // read loop it instead hung (this whole call runs synchronously on the
    // "tft" task while holding spiLock, so a hung socket op freezes the
    // display/touch). Bail out immediately rather than ever attempting it.
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    // transform filename to provider url
    std::string url = TileProvider::url(name);"""

OLD_READ = """    size_t bytesRead = stream->readBytes(pngImage, len);
    if (bytesRead != len) {"""
NEW_READ = """    // A single readBytes() call only reliably drains one TLS record/chunk
    // (observed truncating at a fixed ~4KB boundary regardless of total
    // tile size) - loop explicitly until the full content-length arrives,
    // or we genuinely stall for longer than a generous overall timeout.
    size_t bytesRead = 0;
    uint32_t lastProgressMs = millis();
    // Kept short deliberately: this whole call runs on the "tft" task while
    // holding spiLock, so the display/touch are unresponsive for as long as
    // this takes - and a tile batch can be 9+ tiles. A long stall timeout
    // here multiplies into a much longer apparent freeze across a batch.
    const uint32_t stallTimeoutMs = 3000;
    while (bytesRead < len) {
        size_t n = stream->readBytes(pngImage + bytesRead, len - bytesRead);
        if (n > 0) {
            bytesRead += n;
            lastProgressMs = millis();
        } else if (millis() - lastProgressMs > stallTimeoutMs) {
            break;
        }
    }
    if (bytesRead != len) {"""


def patch():
    if not os.path.isfile(target):
        print(f"[mercurio_v1] URLService.cpp not found at {target}, skipping patch (env may not use device-ui)")
        return
    with open(target, "r") as f:
        content = f.read()
    if MARKER in content:
        return  # already patched
    if OLD_INCLUDE not in content or OLD_GUARD not in content or OLD_READ not in content:
        print(f"[mercurio_v1] WARNING: URLService.cpp doesn't match expected text - device-ui may have changed upstream, skipping patch")
        return
    content = content.replace(OLD_INCLUDE, NEW_INCLUDE, 1)
    content = content.replace(OLD_GUARD, NEW_GUARD, 1)
    content = content.replace(OLD_READ, NEW_READ, 1)
    with open(target, "w") as f:
        f.write(content)
    print(f"[mercurio_v1] Patched {target} for tile-fetch read loop + WiFi guard")


patch()
