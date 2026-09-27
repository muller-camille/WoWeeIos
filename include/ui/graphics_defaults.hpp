#pragma once

/// The graphics defaults a fresh install starts at.
///
/// Spelled in three places before: the settings panel's pending fields, the
/// login screen's copy of the same struct, and the reset button's constant. A
/// default that disagrees with itself is a setting that changes when you open
/// a different window.
///
/// They live here rather than in settings_panel.hpp because that header
/// includes vulkan.h, and a test that only wants to know the numbers should
/// not need the Vulkan SDK to find out - which it does not have on macOS.

#include "core/platform.hpp"

namespace wowee::ui {

#ifdef WOWEE_IOS
// iOS kills an app that passes its per-app ceiling - about 2.9 GB on a 4 GB
// iPad - and at 1900 the terrain radius is five tiles, around eighty tiles of
// terrain, models and textures, which was past it before the world had
// finished loading. 700 is a radius of three.
inline constexpr float kDefaultViewDistance = 700.0f;
#else
inline constexpr float kDefaultViewDistance = 1900.0f;
#endif
#ifdef WOWEE_IOS
// Ground clutter is placed per tile as model instances: at 70 a dense zone
// such as Elwynn put some 18,000 of them round the player, past the model
// renderer's instance buffer and a good part of the memory ceiling.
inline constexpr int   kDefaultGroundClutter = 25;
#else
inline constexpr int   kDefaultGroundClutter = 70;
#endif

// The effects a fresh install starts with. On iOS the world is drawn at two
// thirds of the screen's resolution and upscaled with FSR 1, and the effects
// that cost the most for the least are off: an iPad Air drew Goldshire at
// about 15 frames a second with the desktop set, the GPU the whole limit, and
// every full-resolution target also counts against the app's memory ceiling.
// All of them are still in the settings.
#ifdef WOWEE_IOS
inline constexpr int   kDefaultUpscalingMode = 1;      // FSR 1
inline constexpr int   kDefaultFsrQuality = 1;         // 0.67 of the screen
inline constexpr int   kDefaultVolumetricFog = 0;
inline constexpr bool  kDefaultSunShafts = false;
inline constexpr float kDefaultShadowDistance = 100.0f;
inline constexpr bool  kDefaultWaterRefraction = false;
inline constexpr int   kDefaultAntiAliasing = 0;
inline constexpr int   kDefaultTextureFiltering = 2;
#else
inline constexpr int   kDefaultUpscalingMode = 0;
inline constexpr int   kDefaultFsrQuality = 3;         // native
inline constexpr int   kDefaultVolumetricFog = 2;
inline constexpr bool  kDefaultSunShafts = true;
inline constexpr float kDefaultShadowDistance = 300.0f;
inline constexpr bool  kDefaultWaterRefraction = true;
inline constexpr int   kDefaultAntiAliasing = 1;
inline constexpr int   kDefaultTextureFiltering = 4;
#endif

}  // namespace wowee::ui
