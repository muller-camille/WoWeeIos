#!/usr/bin/env bash
# Copies an extraction into the iOS app bundle, so the app starts with game
# data and nothing has to be copied onto the device through Files or Finder.
#
#   tools/ios/bundle_game_data.sh <data-root> <app-bundle> [expansion...]
#
# <data-root> is what extract_assets.sh wrote, the folder holding
# expansions/<id>/manifest.json - Data/ in this checkout by default. Every
# expansion there with a manifest goes to <app-bundle>/Data/expansions/<id>/,
# or only the ones named after the bundle. An extraction already in the
# bundle for an expansion no longer asked for is taken out again.
# Called by the build, from CMakeLists, when WOWEE_IOS_GAME_DATA is set.
#
# rsync, so a rebuild copies only what changed. The client's own executables,
# libraries and settings, which an extraction can carry beside the data, are
# left out: nothing reads them, and a nested .app in the bundle fails signing.
# So are the client's own tables: the build has already put this checkout's
# copies there, and an extraction's are whatever was current when it was made.
set -euo pipefail

SRC="${1:?data root}"
APP="${2:?app bundle}"
shift 2
WANTED=" $* "

wanted() { [ "$WANTED" = "  " ] || [[ "$WANTED" == *" $1 "* ]]; }

# Out with what was bundled before and is not wanted now. Only the extracted
# files go: the client's own tables beside them came from the build.
for manifest in "$APP"/Data/expansions/*/manifest.json; do
    [ -f "$manifest" ] || continue
    exp_dir="$(dirname "$manifest")"
    wanted "$(basename "$exp_dir")" && continue
    echo "Removing bundled game data: $(basename "$exp_dir")"
    find "$exp_dir" -mindepth 1 -maxdepth 1 ! -name '*.json' -exec rm -rf {} +
    rm -f "$manifest"
done

found=0
for manifest in "$SRC"/expansions/*/manifest.json; do
    [ -f "$manifest" ] || continue
    exp_dir="$(dirname "$manifest")"
    id="$(basename "$exp_dir")"
    wanted "$id" || continue
    dest="$APP/Data/expansions/$id"
    mkdir -p "$dest"
    echo "Bundling game data: $id ($exp_dir)"
    rsync -a --delete \
        --exclude '*.exe' --exclude '*.dll' --exclude '*.app' \
        --exclude '*.wtf' --exclude '*.ini' --exclude 'wtf/' \
        --exclude 'documentation/' --exclude 'uninstalllocalization.xml' \
        --exclude '/dbc_layouts.json' --exclude '/expansion.json' \
        --exclude '/opcodes.json' --exclude '/update_fields.json' \
        --exclude '/movement_sequences.json' \
        "$exp_dir/" "$dest/"
    found=1
done

if [ "$found" = 0 ]; then
    echo "WOWEE_IOS_GAME_DATA: no wanted expansions/*/manifest.json under $SRC" >&2
    exit 1
fi
