# Works around an upstream meshtastic/device-ui bug: TFTView_320x240's
# compile-time instance() always constructs its display driver with
# hardcoded (320, 240) dimensions, regardless of whether VIEW_320x240 or
# VIEW_240x320 is defined. Idempotent: skips if already patched, since
# .pio/libdeps is ephemeral.
Import("env")
import os

target = os.path.join(
    env.subst("$PROJECT_LIBDEPS_DIR"),
    env.subst("$PIOENV"),
    "meshtastic-device-ui",
    "source",
    "graphics",
    "TFT",
    "TFTView_320x240.cpp",
)

MARKER = "VIEW_240x320 portrait dimension fix"

OLD = """TFTView_320x240 *TFTView_320x240::instance(void)
{
    if (!gui) {
        gui = new TFTView_320x240(nullptr, DisplayDriverFactory::create(320, 240));
    }
    return gui;
}"""

NEW = """// mercurio_v1: %s
TFTView_320x240 *TFTView_320x240::instance(void)
{
    if (!gui) {
#if defined(VIEW_240x320)
        gui = new TFTView_320x240(nullptr, DisplayDriverFactory::create(240, 320));
#else
        gui = new TFTView_320x240(nullptr, DisplayDriverFactory::create(320, 240));
#endif
    }
    return gui;
}""" % MARKER


def patch():
    if not os.path.isfile(target):
        print(f"[mercurio_v1] TFTView_320x240.cpp not found at {target}, skipping patch (env may not use device-ui)")
        return
    with open(target, "r") as f:
        content = f.read()
    if MARKER in content:
        return  # already patched
    if OLD not in content:
        print(f"[mercurio_v1] WARNING: TFTView_320x240.cpp doesn't match expected text - device-ui may have changed upstream, skipping patch")
        return
    content = content.replace(OLD, NEW, 1)
    with open(target, "w") as f:
        f.write(content)
    print(f"[mercurio_v1] Patched {target} for VIEW_240x320 portrait support")


patch()
