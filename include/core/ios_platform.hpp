#pragma once

#include "core/platform.hpp"

#ifdef WOWEE_IOS

namespace wowee {
namespace core {

/**
 * Name the client's three roots for an iOS sandbox, before anything reads them.
 *
 * The Android build does this in WoweeActivity, in Java, before the library is
 * loaded. iOS has no such seam, so main() calls this first thing:
 *
 *   WOWEE_RESOURCE_ROOT  the app bundle. It holds assets/, the client's own
 *                        Data/ tables and addons/, the layout of a desktop
 *                        install, and it is read-only on a device.
 *   WOW_DATA_PATH        Documents/Data. The player's extraction goes here,
 *                        copied in through the Files app or a Mac's Finder, so
 *                        it has to be somewhere they can reach. With nothing
 *                        there, the bundle's Data/, if the build put an
 *                        extraction into the app.
 *   WOWEE_CONFIG_ROOT    Documents/config, for settings and logs. Beside the
 *                        data for the same reason: a log is only useful to a
 *                        bug report if the player can get at it.
 *
 * A variable already set - by an Xcode scheme, say - is left alone. Both
 * Documents directories are created, so the app's folder shows up in Files
 * before anything has been copied into it, and Data/ is kept out of iCloud
 * backups: an extraction is gigabytes the player can always copy again.
 *
 * Logs nothing: the logger picks its file from WOWEE_CONFIG_ROOT the first
 * time it writes, so a line from here would put the log somewhere else.
 */
void prepareIosSandbox();

} // namespace core
} // namespace wowee

#endif // WOWEE_IOS
