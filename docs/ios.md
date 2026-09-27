# iOS

The iOS client is the same tree built for iPhone and iPad on arm64, the way the
Android client is. It is a UIKit app through SDL3, Vulkan through MoltenVK
linked into the executable, and the touch controls Android already has.

> [!WARNING]
> **Not yet built or run.** This port was written on a machine with neither a
> Mac nor an iOS device, so nothing below has been compiled for iOS, let alone
> started on a phone. The desktop build it shares code with was rebuilt and is
> unchanged. The `build-ios` CI job is the first thing that will compile it;
> expect a round of fixes the first time it runs, as Android needed.

## What you need

- A Mac with Xcode 16 or newer, and CMake 3.28 or newer (`brew install cmake`)
- An iPhone or iPad on **iOS 16 or newer** (arm64, which is every device that
  can run iOS 16)
- An Apple ID. A free one signs for your own devices, for 7 days at a time; a
  paid developer account signs for a year

The iOS Simulator is not supported: MoltenVK's iOS package carries a device
library only, and the simulator's Metal lacks much of what the renderer asks
for anyway.

## Building it

```bash
tools/build-ios-deps.sh              # OpenSSL for iphoneos, and MoltenVK, once

cmake -S . -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DWOWEE_IOS_TEAM_ID=ABCDE12345 \
  -DWOWEE_IOS_BUNDLE_ID=com.yourname.wowee

open build-ios/wowee.xcodeproj
```

In Xcode pick the **wowee** scheme and your device, then Run. The first run on
a device asks for two things on the device itself:

- **Developer Mode**: Settings > Privacy & Security > Developer Mode
- **Trust the developer**: Settings > General > VPN & Device Management

`WOWEE_IOS_TEAM_ID` is the ten-character ID in Xcode > Settings > Accounts.
`WOWEE_IOS_BUNDLE_ID` has to be unique to your team: a free account cannot sign
`com.wowee.client`, the default, because somebody else already has.

From the command line instead of Xcode:

```bash
cmake --build build-ios --config Release --target wowee -- -allowProvisioningUpdates
tools/ios/package_ipa.sh build-ios/bin/Release-iphoneos/WoWee.app WoWee.ipa
```

Debug builds carry this project's own music; Release builds leave it out, the
same split every other platform's release makes.

### The unsigned .ipa

CI and the release workflow build `wowee-<version>-ios-arm64-unsigned.ipa`
without signing it, because there is no team every player belongs to. A
sideloading tool signs it with your own Apple ID as it installs it:
[AltStore](https://altstore.io), [SideStore](https://sidestore.io) or
[Sideloadly](https://sideloadly.io). The same 7-day limit applies to a free
Apple ID.

## Game data on a phone or tablet

Exactly as for Android: extract on a desktop with `wowee_assets` or the
`extract_assets` scripts, cut it down to a profile that fits, and copy the
result across. There is no asset builder on the device; StormLib and the game
install both live on the computer.

```bash
tools/android/make_minimal_data.py --source ~/Data --out ~/Data-phone \
    --profile world --maps all
```

The profiles and their sizes are in the [README](../README.md#game-data-on-a-phone);
nothing about them is Android specific despite the directory name.

Open WoWee once first, so iOS creates its folder. Then copy the **contents** of
`~/Data-phone` into WoWee's `Data` folder, so that `manifest.json` sits directly
inside it. Either way works:

- **Finder**, with the device connected by cable: select the device, open the
  **Files** tab, and drag the contents onto **WoWee > Data**. This is the one to
  use for gigabytes.
- **Files app** on the device: **On My iPhone** (or iPad) > **WoWee** > **Data**.

With nothing there, the login screen says so and says where.

## Where things are

Inside the app's sandbox, which the Files app shows as the WoWee folder:

| Path | What |
|---|---|
| `Documents/Data/` | Your game data. Kept out of iCloud backups: it is gigabytes you can always copy again |
| `Documents/config/` | Settings, saved servers and characters |
| `Documents/config/logs/wowee.log` | The log, beside the settings so a bug report can include it |
| `Library/Caches/wowee/` | The Vulkan pipeline cache, which iOS may clear when space is short |

Log lines also go to the unified log, so Xcode's console shows them live, and
so does Console.app on a Mac with the device selected. Set `WOWEE_LOG_LEVEL`
to `info` or `debug` under the scheme's Run > Arguments > Environment
Variables to see more than warnings.

## Touch controls

The same as on Android:

| Input | Action |
|---|---|
| Left thumb, lower left | Move and strafe |
| Right thumb, drag | Turn the view; the character faces where it looks |
| Two fingers | Zoom the camera |
| Tap | Target, interact, and everything in the interface |

A game controller works through SDL, as on every platform. An iPad's keyboard,
trackpad or mouse arrive the way they do on a desktop.

## How it differs from the other platforms

- **Leaving the app ends the session.** iOS suspends a backgrounded app within
  seconds and its socket with it, so the server times the character out. Android
  keeps the loop running in the background; iOS does not allow that for a game.
- **No Warden emulation.** Unicorn is a JIT and an iOS app may not map
  executable memory. The stub the Android build uses is the one used here, and
  the module image is mapped read-write only.
- **No cinematics.** There is no FFmpeg in the build, as on Android.
- **Landscape only**, and full screen on an iPad.
- **Memory.** iOS kills an app that passes a per-app ceiling well below the
  device's memory. The client measures its room under that ceiling rather than
  free memory, and keeps its file cache to the 384 MB Android uses. With a paid
  team, `-DWOWEE_IOS_INCREASED_MEMORY_LIMIT=ON` signs the app with the
  Increased Memory Limit entitlement, which raises the ceiling.
- **Textures.** Devices whose GPU has no BC formats get their DXT textures
  unpacked to RGBA8, through the path written for Android.

## How it is put together

Where the port is code, and why each piece is the way it is.

| Piece | Where | Why |
|---|---|---|
| Platform split | `include/core/platform.hpp` | `__APPLE__` is macOS and iOS both, and most `__APPLE__` code is macOS only. `WOWEE_IOS`, `WOWEE_MACOS` and `WOWEE_MOBILE` (Android or iOS) say which is meant |
| Sandbox roots | `src/core/ios_platform.mm`, called first in `main()` | Android sets its three roots from Java before loading the library. iOS has no such seam, so `main()` sets `WOWEE_RESOURCE_ROOT` to the bundle, `WOW_DATA_PATH` to `Documents/Data` and `WOWEE_CONFIG_ROOT` to `Documents/config` |
| Entry point | `SDL_main.h` in `main.cpp` | SDL supplies the real `main`, which starts `UIApplicationMain` and calls the client once UIKit has launched |
| Vulkan | `CMakeLists.txt`, the `IOS` branch | No loader exists on iOS. MoltenVK's static XCFramework is linked in and made the `Vulkan::Vulkan` target. SDL finds `vkGetInstanceProcAddr` among the executable's own symbols, so the link keeps it exported and Xcode's strip keeps global symbols; vk-bootstrap is handed SDL's pointer rather than going looking for `libvulkan.dylib` |
| Background | `VkContext::pausePresentation` | iOS kills an app that submits GPU work from the background. Android releases its surface there, but SDL makes a new Metal view for every surface it creates and cannot give the old one back, so iOS stops drawing without tearing anything down |
| Audio | `src/audio/miniaudio_impl.mm` | miniaudio's iOS backend is Objective-C and has no switch to leave that out, so its implementation is compiled as Objective-C++ on iOS only |
| Lua | `ios/lua_ios_compat.h` | The iOS SDK makes `system()` an error; `os.execute` is its only caller, and answers as a platform with no shell |
| Interface scale | `ui_manager.cpp` | iOS measures in points. A phone in landscape is 375 to 440 points tall and the client's tallest dialog needs 620, so the scale there goes below 1 |
| Bundle | `ios/Info.plist.in`, `ios/Assets.xcassets` | Landscape, full screen, file sharing into Documents, the local-network prompt a LAN realm needs, 120 Hz on ProMotion phones, and the icon |
| Packaging | `tools/ios/package_ipa.sh` | An `.ipa` is the app under `Payload/`, zipped |
