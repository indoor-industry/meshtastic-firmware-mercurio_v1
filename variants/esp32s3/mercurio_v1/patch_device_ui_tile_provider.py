# Cosmetic preference, not a bug fix: makes OpenStreetMap (a documented,
# stable public tile service) the default map tile provider instead of
# device-ui's stock default, Google's undocumented mt0.google.com/vt
# endpoint. Both work once patch_device_ui_map_fetch.py's read-loop fix
# is applied - this just changes which one loads first.
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
    "TileProvider.cpp",
)

MARKER = "mercurio_v1: OpenStreetMap default"

OLD = '''std::vector<std::tuple<std::string, std::string>> TileProvider::urlTemplates = {
    {"URL: Google Maps", "https://mt0.google.com/vt?lyrs=m&x={x}&s=&y={y}&z={z}"}};'''

NEW = '''// %s
std::vector<std::tuple<std::string, std::string>> TileProvider::urlTemplates = {
    {"URL: OpenStreetMap", "https://tile.openstreetmap.org/{z}/{x}/{y}.png"},
    {"URL: Google Maps", "https://mt0.google.com/vt?lyrs=m&x={x}&s=&y={y}&z={z}"}};''' % MARKER


def patch():
    if not os.path.isfile(target):
        print(f"[mercurio_v1] TileProvider.cpp not found at {target}, skipping patch (env may not use device-ui)")
        return
    with open(target, "r") as f:
        content = f.read()
    if MARKER in content:
        return  # already patched
    if OLD not in content:
        print(f"[mercurio_v1] WARNING: TileProvider.cpp doesn't match expected text - device-ui may have changed upstream, skipping patch")
        return
    content = content.replace(OLD, NEW, 1)
    with open(target, "w") as f:
        f.write(content)
    print(f"[mercurio_v1] Patched {target} to default to OpenStreetMap")


patch()
