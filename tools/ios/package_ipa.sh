#!/usr/bin/env bash
# Wraps a built WoWee.app into an .ipa: the app under Payload/, zipped.
#
#   tools/ios/package_ipa.sh <path/to/WoWee.app> <out.ipa>
#
# The app goes in as it was built. Built with signing, the .ipa installs on the
# devices its profile names, through Xcode's Devices window or Apple
# Configurator. Built with CODE_SIGNING_ALLOWED=NO, as CI builds it, it is the
# unsigned .ipa that sideloading tools - AltStore, SideStore, Sideloadly - sign
# with the player's own Apple ID as they install it.
set -euo pipefail

APP="${1:?usage: package_ipa.sh <WoWee.app> <out.ipa>}"
OUT="${2:?usage: package_ipa.sh <WoWee.app> <out.ipa>}"

if [ ! -f "$APP/Info.plist" ]; then
    echo "$APP is not an iOS app bundle (no Info.plist at its root)." >&2
    exit 1
fi

# The executable the plist names has to be there; an app missing it installs
# and then refuses to open, which is a worse place to find out.
EXE="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$APP/Info.plist")"
if [ ! -x "$APP/$EXE" ]; then
    echo "$APP has no executable named $EXE." >&2
    exit 1
fi
for required in assets/shaders Data/expansions addons; do
    if [ ! -d "$APP/$required" ]; then
        echo "$APP has no $required/ - the build's resource copy did not run." >&2
        exit 1
    fi
done

OUT_DIR="$(cd "$(dirname "$OUT")" && pwd)"
OUT_ABS="$OUT_DIR/$(basename "$OUT")"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

mkdir -p "$STAGE/Payload"
# ditto keeps the symlinks, modes and extended attributes an app is made of.
ditto "$APP" "$STAGE/Payload/$(basename "$APP")"
rm -f "$OUT_ABS"
(cd "$STAGE" && zip -qry "$OUT_ABS" Payload)
echo "Wrote $OUT_ABS ($(du -h "$OUT_ABS" | cut -f1))"
