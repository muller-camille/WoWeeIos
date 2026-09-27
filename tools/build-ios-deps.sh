#!/usr/bin/env bash
# Builds the iOS dependencies CMake cannot fetch for itself.
#
# SDL3 and glm come down through FetchContent in CMakeLists, as on Android.
# Two things do not:
#
#   OpenSSL   does not build with CMake. Built here once, for iphoneos arm64,
#             and handed to the client build the way Android's is.
#   MoltenVK  is the Vulkan driver. iOS has no Vulkan loader and no system
#             copy, so the static XCFramework from Khronos' own release is
#             linked into the app. Downloaded, not built: building it takes
#             the better part of an hour and gives the same library.
#
#   tools/build-ios-deps.sh [deployment-target]
#
# Needs macOS with Xcode. Defaults to iOS 16.0, the floor CMakeLists assumes.
# The results land in build-ios-deps/, which is gitignored and which the iOS
# configure finds without being told:
#
#   build-ios-deps/iphoneos/   OpenSSL: include/, lib/libssl.a, lib/libcrypto.a
#   build-ios-deps/MoltenVK/   MoltenVK's release package
set -euo pipefail

TARGET_VERSION="${1:-16.0}"
OPENSSL_VERSION="${OPENSSL_VERSION:-3.5.1}"
# 1.4.2 is the first to require iOS 15, which is below the floor above anyway.
MOLTENVK_VERSION="${MOLTENVK_VERSION:-1.4.2}"

if [ "$(uname -s)" != "Darwin" ]; then
    echo "The iOS build needs macOS with Xcode; this is $(uname -s)." >&2
    exit 1
fi
if ! xcrun --sdk iphoneos --show-sdk-path >/dev/null 2>&1; then
    echo "No iPhoneOS SDK. Install Xcode, then:" >&2
    echo "  sudo xcode-select -s /Applications/Xcode.app" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="$ROOT/build-ios-deps"
PREFIX="$WORK/iphoneos"
mkdir -p "$WORK"
JOBS="$(sysctl -n hw.logicalcpu)"

# ---- OpenSSL ---------------------------------------------------------------
if [ -f "$PREFIX/lib/libcrypto.a" ] && [ -f "$PREFIX/lib/libssl.a" ]; then
    echo "OpenSSL already built: $PREFIX"
else
    TARBALL="$WORK/openssl-$OPENSSL_VERSION.tar.gz"
    SRC="$WORK/openssl-$OPENSSL_VERSION"
    [ -f "$TARBALL" ] || curl -fL --retry 3 -o "$TARBALL" \
        "https://github.com/openssl/openssl/releases/download/openssl-$OPENSSL_VERSION/openssl-$OPENSSL_VERSION.tar.gz"
    [ -d "$SRC" ] || tar -xzf "$TARBALL" -C "$WORK"

    # ios64-xcrun is OpenSSL's own iphoneos arm64 target: it asks xcrun for the
    # SDK and the compiler. Its built-in deployment target is iOS 7, which a
    # current Xcode warns about at every link; the one given here comes after
    # it on the command line and wins. no-shared because an iOS app may not
    # carry a loose dylib, and no-tests and no-apps cut most of the build.
    (cd "$SRC" && ./Configure ios64-xcrun no-shared no-tests no-apps \
        "-mios-version-min=$TARGET_VERSION" \
        --prefix="$PREFIX" --openssldir="$PREFIX/ssl")
    make -C "$SRC" -j"$JOBS"
    make -C "$SRC" install_sw
fi

# ---- MoltenVK --------------------------------------------------------------
MVK="$WORK/MoltenVK"
MVK_LIB="$MVK/MoltenVK/static/MoltenVK.xcframework/ios-arm64/libMoltenVK.a"
if [ -f "$MVK_LIB" ] && [ "$(cat "$MVK/.version" 2>/dev/null)" = "$MOLTENVK_VERSION" ]; then
    echo "MoltenVK $MOLTENVK_VERSION already unpacked: $MVK"
else
    TAR="$WORK/MoltenVK-ios-$MOLTENVK_VERSION.tar"
    [ -f "$TAR" ] || curl -fL --retry 3 -o "$TAR" \
        "https://github.com/KhronosGroup/MoltenVK/releases/download/v$MOLTENVK_VERSION/MoltenVK-ios.tar"
    rm -rf "$MVK"
    # The archive's top directory is MoltenVK/, which is where this puts it.
    tar -xf "$TAR" -C "$WORK"
    if [ ! -f "$MVK_LIB" ]; then
        echo "MoltenVK-ios.tar $MOLTENVK_VERSION has no $MVK_LIB; its layout has changed." >&2
        exit 1
    fi
    echo "$MOLTENVK_VERSION" > "$MVK/.version"
fi

echo
echo "Dependencies ready. Configure the client with:"
echo "  cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS \\"
echo "        -DCMAKE_OSX_DEPLOYMENT_TARGET=$TARGET_VERSION -DWOWEE_IOS_TEAM_ID=<your team>"
