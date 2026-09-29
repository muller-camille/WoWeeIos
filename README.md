# WoWee for iPhone and iPad

<p align="center">
  <img src="assets/Wowee.png" alt="WoWee logo" width="240" />
</p>

<p align="center">
  A native World of Warcraft client for iPhone and iPad, drawn through Metal.
</p>

WoWee is a C++ client for **Vanilla 1.12**, **TBC 2.4.3** and **WotLK 3.3.5a**
servers, tested against AzerothCore/ChromieCraft, TrinityCore, MaNGOS and
Turtle WoW 1.18. This fork of [Kelsidavis/WoWee](https://github.com/Kelsidavis/WoWee)
builds it for iOS: a UIKit app through SDL3, touch controls, and a renderer
rewritten natively on Metal.

<p align="center">
  <img src="assets/orgrimmar-entrance.png" alt="WoWee rendering Orgrimmar" width="100%" />
</p>

> [!IMPORTANT]
> WoWee is an educational and research project. It contains no Blizzard
> Entertainment assets, data, or proprietary code. You must supply your own
> legally obtained game data and comply with the laws in your jurisdiction.
> WoWee is not affiliated with or endorsed by Blizzard Entertainment.

## Status

It plays on the test device, an **iPad Air (4th generation, A14, 4 GB)**: it
logs in, enters the world and plays - terrain, buildings, doodads, characters,
water with refraction and reflection, the sky, weather, shadows, spell effects,
the minimap and the world map, drawn through Metal and upscaled by MetalFX.
Nothing has been tried on an iPhone yet.

The Metal port is at its parity milestone: everything the earlier Vulkan
renderer draws on iOS is written for Metal - ray traced lighting excepted, by
decision - and the most recent of it is being checked on the device. [docs/plan-metal.md](docs/plan-metal.md) has the milestones,
what the iPad has shown, what is left, and how to test each part.

This is a work in progress, not a replacement for the official client. See
[Known limitations](#known-limitations) before reporting a bug.

## What you need

- A Mac with **Xcode 16** or newer (it is built with Xcode 26) and **CMake
  3.28** or newer
- An iPhone or iPad on **iOS 16** or newer with a Metal 3 GPU: **A13 or
  later** - iPhone 11 onward, iPad Air 4 onward, every M-series iPad
- An **Apple ID**. A free one signs for your own devices, for 7 days at a
  time; a paid developer account signs for a year
- Your own World of Warcraft client, of the version your server runs, to
  extract game data from - and a server to play on

The iOS Simulator is not supported.

## Building and installing

```bash
git clone --recurse-submodules https://github.com/muller-camille/WoWeeIos.git
cd WoWeeIos

tools/build-ios-deps.sh              # OpenSSL for iphoneos, and MoltenVK, once

cmake -S . -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DWOWEE_IOS_TEAM_ID=ABCDE12345 \
  -DWOWEE_IOS_BUNDLE_ID=com.yourname.wowee

open build-ios/wowee.xcodeproj
```

In Xcode pick the **wowee** scheme and your device, then Run. The first run
asks for two things on the device itself:

- **Developer Mode**: Settings > Privacy & Security > Developer Mode
- **Trust the developer**: Settings > General > VPN & Device Management

`WOWEE_IOS_TEAM_ID` is the ten-character ID in Xcode > Settings > Accounts.
`WOWEE_IOS_BUNDLE_ID` has to be unique to your team: a free account cannot sign
`com.wowee.client`, the default, because somebody else already has.

From the command line instead of Xcode:

```bash
cmake --build build-ios --config Release --target wowee -- -allowProvisioningUpdates
tools/ios/package_ipa.sh build-ios/bin/Release/WoWee.app WoWee.ipa
```

Debug builds carry this project's own music; Release builds leave it out.

**Without a Mac build.** CI builds an unsigned `.ipa`, because there is no team
every player belongs to. A sideloading tool signs it with your own Apple ID as
it installs it: [AltStore](https://altstore.io), [SideStore](https://sidestore.io)
or [Sideloadly](https://sideloadly.io). The same 7-day limit applies to a free
Apple ID.

**The earlier renderer.** `-DWOWEE_METAL=OFF` builds the Vulkan renderer
through MoltenVK instead, which draws everything the desktop client does and
runs on any iOS 16 device. It stays until the Metal port reaches parity.
MoltenVK is linked by both builds until the Vulkan code leaves the tree, which
is why `build-ios-deps.sh` fetches it.

## Game data

WoWee does not read the game's MPQ archives at runtime. It reads a loose-file
tree with a generated `manifest.json`, extracted on the Mac from your own
client, cut down to fit, and copied onto the device. There is no extractor on
the device itself.

**1. Extract.** The extractor needs StormLib, and is built from the same tree
for the Mac:

```bash
brew install cmake pkg-config sdl3 glm openssl@3 zlib ffmpeg \
  vulkan-loader vulkan-headers molten-vk shaderc stormlib
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target asset_extract
./extract_assets.sh /path/to/WoW/Data wotlk      # classic, turtle, tbc or wotlk
```

That writes `Data/expansions/<expansion>/`. Several expansions can sit side by
side; the login screen's **Assets** selector appears once there is more than
one. The `wowee_assets` window does the same with a picker, and
[the asset manager](docs/asset-manager.md) covers upgrades and packs.

**2. Cut it down.** A full extraction is about 18 GB. A profile keeps what a
device needs:

```bash
tools/android/make_minimal_data.py --source Data --out ~/Data-ipad \
    --profile world --maps all
```

| Profile | Size | Reaches |
|---|---|---|
| `login` | 787 MB | Login, character selection and creation. No world |
| `world --maps azeroth` | 7.5 GB | One continent |
| `world --maps all` | 12 GB | Every map |
| `full` | 18 GB | Everything, sound included |

Nothing about the profiles is Android specific despite the directory name;
[tools/android/README.md](tools/android/README.md) describes them.

**3. Copy it across.** Open WoWee once first, so iOS creates its folder. Then
copy the **contents** of `~/Data-ipad` into WoWee's `Data` folder, so that
`manifest.json` sits directly inside it:

- **Finder**, with the device connected by cable: select the device, open the
  **Files** tab, and drag the contents onto **WoWee > Data**. This is the one
  to use for gigabytes.
- **Files app** on the device: **On My iPhone** (or iPad) > **WoWee** > **Data**.

With nothing there, the login screen says so and says where.

**Or build it into the app**, for a device of your own:

```bash
cmake -S . -B build-ios -DWOWEE_IOS_GAME_DATA=$PWD/Data \
      -DWOWEE_IOS_GAME_DATA_EXPANSIONS=wotlk
```

The app then starts with its data, at the cost of signing and installing
gigabytes on every build - and it is yours alone: never hand it to anyone else.
Data copied into `Documents/Data` wins over it.

## Playing

Choose a server from the login screen's **Server** list, which carries
ChromieCraft and every server you have logged into. **Somewhere else...** opens
the address, port and expansion under **more options**. A realm on your own
network makes iOS ask for local network access the first time.

### Touch controls

| Input | Action |
|---|---|
| Left thumb, lower left | Move and strafe |
| Right thumb, drag | Turn the view; the character faces where it looks |
| Two fingers | Zoom the camera |
| Tap | Target, and click anything in the interface |
| Tap the target again | Talk, trade, loot or attack: what a right-click does |
| Touch and hold | Show the tooltip; lifting the finger clicks nothing |
| Hold, then drag | Pick up an item or a spell and carry it to a bag, a bar or a player |
| Round buttons, lower right | Action slots 1 to 6 and jump |
| Buttons along the top | Bags, character, spellbook, talents, quests, map and the game menu |

The round buttons and the top row step aside for the interface: one with a
window under it, such as an open bag in that corner, is hidden until the window
closes. Enter `/unstuck` in chat if the character is caught in terrain or a
building.

A game controller works through SDL: the left stick moves, the right stick
looks, the triggers zoom and the bottom face button jumps. The scheme is listed
and can be rebound in the game's **Key Bindings** panel. An iPad's keyboard,
trackpad or mouse work as on a desktop.

### Settings

The game menu holds the client's own settings under a **WoWee** heading in the
game's **Video**, **Sound** and **Interface** panels. On iOS they start light:
the world is drawn at two thirds of the screen and brought up by MetalFX (FSR 1
where the device has no MetalFX), with volumetric fog, sun shafts, grass and
MSAA off, and ground clutter at 25. View distance and shadow distance cost the
most; **Water reflections** under Detail turns off the second picture water
draws. A new MSAA choice takes effect the next time the game starts.

## How iOS differs

- **Leaving the app ends the session.** iOS suspends a backgrounded app within
  seconds and its socket with it, so the server times the character out.
- **Memory is the constraint.** iOS kills an app that passes a per-app ceiling
  well below the device's memory - about 2.9 GB on a 4 GB iPad, graphics
  counted - and says nothing in the app's own log when it does; the device's
  crash reports show a `JetsamEvent`. The client measures its room under that
  ceiling and starts with smaller caches and view distance than on a desktop.
  `WOWEE_MEMORY_REPORT=1` logs the footprint every few seconds. With a paid
  team, `-DWOWEE_IOS_INCREASED_MEMORY_LIMIT=ON` signs the app with the
  Increased Memory Limit entitlement, which raises the ceiling.
- **No Warden emulation.** Unicorn is a JIT, and an iOS app may not map
  executable memory.
- **No cinematics.** There is no FFmpeg in the app.
- **Landscape only**, and full screen on an iPad.

## Logs and files

Inside the app's sandbox, which the Files app shows as the WoWee folder:

| Path | What |
|---|---|
| `Documents/Data/` | Your game data. Kept out of iCloud backups |
| `Documents/config/` | Settings, saved servers and characters |
| `Documents/config/logs/wowee.log` | The log, beside the settings so a bug report can include it |

Log lines also go to the unified log, so Xcode's console shows them live, and
so does Console.app on a Mac with the device selected. Set `WOWEE_LOG_LEVEL` to
`info` or `debug` under the scheme's Run > Arguments > Environment Variables to
see more than warnings.

## The Metal renderer

The renderer speaks Metal through Apple's C++ bindings, metal-cpp, so it stays
in the codebase's C++. The GLSL shaders stay the source of every shader: they
compile to SPIR-V, `tools/metal/convert_shaders.py` translates that to Metal
Shading Language and writes a manifest of where each binding went, and Xcode
builds the result into the app's `default.metallib`. The translation is
committed beside the SPIR-V, so building the app needs only Xcode;
`convert_shaders.py --check` says when it is stale after a shader change.

For testing on a device, from the scheme's environment variables:
`WOWEE_AUTO_ENTER=1` enters the world unattended, `WOWEE_FRAME_PROFILE=1` logs
the frame's CPU and GPU time pass by pass, `WOWEE_METAL_SKIP=<passes>` leaves
passes out to tell their cost apart, and `WOWEE_SCREENSHOT=<file.png>` writes a
frame to the config folder. [docs/plan-metal.md](docs/plan-metal.md) lists the
rest, with the decisions behind the port.

`tools/metal/syntax_check/check.sh` parses the Metal build's C++ on Linux,
against metal-cpp and stand-ins for Apple's headers: it finds what does not
compile before a Mac does, though not what does not work.

The tree still builds the upstream client for Linux, Windows, macOS and
Android, whose instructions are in [the upstream README](https://github.com/Kelsidavis/WoWee#readme).
This fork removes them once the Metal renderer reaches parity.

## Known limitations

- Nothing has been tried on an iPhone.
- The latest Metal work is written and checked on Linux but not yet seen on the
  iPad; [docs/plan-metal.md](docs/plan-metal.md) says which parts.
- At night in Tirisfal the nearest tree's canopy can draw solid black.
- An Undead player character has no hair, and a pale slab on its back.
- Terrain and building transitions are a longstanding regression area: the
  character can prefer the wrong floor or get stuck. `/unstuck` recovers.
- Warden modules are checked against Blizzard's signing key. A realm that signs
  its own module needs that key set as `wardenRsaModulus` in the expansion's
  `expansion.json`.

When reporting a bug, include the relevant lines of `wowee.log`, the device,
the expansion, the server core, and how to reproduce it.

## Documentation

- [iOS in detail](docs/ios.md) - signing, the sandbox, and how the port is put together
- [The Metal renderer plan](docs/plan-metal.md) - milestones, status and device testing
- [Server setup](docs/server-setup.md) and [realm list](docs/realm-list.md)
- [Asset manager](docs/asset-manager.md)
- [Project status](docs/status.md) and [architecture](docs/architecture.md)

## License and references

WoWee source code is available under the [MIT License with an additional
restriction](LICENSE): it may not be used, in whole or in part, as the basis
for or a component of a commercial video game or other commercial game product
without written permission. Original music and audio assets are not covered by
the MIT terms at all and are reserved - see [LICENSE](LICENSE) and
[NOTICE](NOTICE). World of Warcraft and its assets are property of Blizzard
Entertainment, Inc.

This fork builds on [WoWee by Kelsidavis](https://github.com/Kelsidavis/WoWee);
[ATTRIBUTION.md](ATTRIBUTION.md) credits the community work behind it.

- [WoWDev Wiki](https://wowdev.wiki/) - file-format documentation
- [TrinityCore](https://github.com/TrinityCore/TrinityCore) - server reference
- [MaNGOS](https://github.com/cmangos/mangos-wotlk) - server reference
- [StormLib](https://github.com/ladislav-zezula/StormLib) - MPQ library
- [metal-cpp](https://developer.apple.com/metal/cpp/) - Apple's C++ interface to Metal
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) - the shader translation
