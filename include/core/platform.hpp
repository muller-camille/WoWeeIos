#pragma once

/**
 * platform.hpp - the platform families this client tells apart, where the
 * compiler's own macros do not say it directly.
 *
 * __APPLE__ is macOS and iOS alike, and most of what this client does under it
 * is macOS only: AppKit, the Homebrew or LunarG Vulkan loader, the Contents/
 * layout of a desktop app bundle. So Apple is split in two here, and the code
 * says which half it means.
 *
 * WOWEE_MOBILE is a phone or a tablet: one fullscreen landscape window, touch
 * first, a per-app memory limit far below the machine's, and a surface the
 * system takes away whenever the app leaves the foreground. Android and iOS
 * share all of that, so the code written for the one serves the other.
 *
 * Each is defined or not, never 0: test them with #ifdef / defined().
 */

#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define WOWEE_IOS 1
#else
#define WOWEE_MACOS 1
#endif
#endif

#if defined(__ANDROID__) || defined(WOWEE_IOS)
#define WOWEE_MOBILE 1
#endif
