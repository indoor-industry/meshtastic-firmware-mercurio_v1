# Works around a type bug in the vendored meshtastic/ESP8266Audio fork's
# AudioOutputI2S class: mclkPin is declared uint8_t, while every other
# pin field and every SetPinout()/SetPinout(...,mclk) parameter use a
# signed int. Boards with no MCLK pin (this one included - the MAX98357A
# doesn't need one) pass -1 ("no MCLK") through the 4-arg SetPinout()
# overload; stored into a uint8_t, -1 silently truncates to 255.
# ESP-IDF's i2s_set_pin() then rejects 255 as an invalid GPIO and returns
# an error BEFORE configuring the BCK/WS/DOUT pins at all - meaning the
# I2S peripheral is never actually wired to its real clock/data lines, no
# matter what else is configured correctly. Confirmed via serial log:
# "i2s_check_set_mclk: mck_io_num invalid" / "i2s_set_pin: mclk config
# failed" on every attempt to play audio, with total silence, before this
# fix.
#
# Not mercurio_v1-specific - hits any board using this library with no
# MCLK pin (mclkPin = -1), which is most simple I2S DAC/amps that don't
# need a master clock (e.g. the MAX98357A here).
#
# Idempotent: skips if already patched, since .pio/libdeps is ephemeral.
Import("env")
import os

target = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"),
    env.subst("$PIOENV"),
    "ESP8266Audio",
    "src",
    "AudioOutputI2S.h",
)

MARKER = "mercurio_v1: mclkPin must be signed to round-trip -1"

OLD = """    uint8_t bclkPin;
    uint8_t wclkPin;
    uint8_t doutPin;
    uint8_t mclkPin;"""

NEW = """    uint8_t bclkPin;
    uint8_t wclkPin;
    uint8_t doutPin;
    // int, not uint8_t: must round-trip -1 ("no MCLK pin") correctly. As
    // uint8_t, -1 truncated to 255, which i2s_set_pin() then rejected as
    // an invalid GPIO and aborted before configuring bck/ws/dout at all -
    // silently leaving I2S never actually wired to its pins. (%s)
    int mclkPin;""" % MARKER


def patch():
    if not os.path.isfile(target):
        print(f"[mercurio_v1] AudioOutputI2S.h not found at {target}, skipping patch (env may not use ESP8266Audio)")
        return
    with open(target, "r") as f:
        content = f.read()
    if MARKER in content:
        return  # already patched
    if OLD not in content:
        print(f"[mercurio_v1] WARNING: AudioOutputI2S.h doesn't match expected text - ESP8266Audio may have changed upstream, skipping patch")
        return
    content = content.replace(OLD, NEW, 1)
    with open(target, "w") as f:
        f.write(content)
    print(f"[mercurio_v1] Patched {target} to fix mclkPin's -1 sentinel truncation")


patch()
