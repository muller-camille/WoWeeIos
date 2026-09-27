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
inline constexpr int   kDefaultGroundClutter = 70;

}  // namespace wowee::ui
