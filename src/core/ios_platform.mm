#include "core/ios_platform.hpp"

#include "core/env.hpp"

#import <Foundation/Foundation.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace wowee {
namespace core {

namespace fs = std::filesystem;

namespace {

fs::path documentsDirectory() {
    @autoreleasepool {
        NSArray<NSString*>* found = NSSearchPathForDirectoriesInDomains(
            NSDocumentDirectory, NSUserDomainMask, YES);
        if (found.count > 0) {
            return fs::path([found.firstObject fileSystemRepresentation]);
        }
    }
    // The sandbox's home is the container, and Documents is always under it.
    if (const char* home = std::getenv("HOME"); home && *home) {
        return fs::path(home) / "Documents";
    }
    return {};
}

fs::path bundleDirectory() {
    @autoreleasepool {
        NSString* path = [[NSBundle mainBundle] bundlePath];
        if (path.length > 0) return fs::path([path fileSystemRepresentation]);
    }
    return {};
}

void excludeFromBackup(const fs::path& dir) {
    @autoreleasepool {
        NSString* path = [NSString stringWithUTF8String:dir.c_str()];
        if (!path) return;
        NSURL* url = [NSURL fileURLWithPath:path isDirectory:YES];
        // Best effort. A backup that does include the data costs the player
        // iCloud space, not a working client.
        [url setResourceValue:@YES forKey:NSURLIsExcludedFromBackupKey error:nil];
    }
}

} // namespace

void prepareIosSandbox() {
    std::error_code ec;

    const fs::path bundle = bundleDirectory();
    if (!bundle.empty()) {
        setEnvVar("WOWEE_RESOURCE_ROOT", bundle.c_str(), /*overwrite=*/false);
    }

    const fs::path documents = documentsDirectory();
    if (documents.empty()) return;

    const fs::path data = documents / "Data";
    fs::create_directories(data, ec);
    excludeFromBackup(data);
    setEnvVar("WOW_DATA_PATH", data.c_str(), /*overwrite=*/false);

    const fs::path config = documents / "config";
    fs::create_directories(config, ec);
    setEnvVar("WOWEE_CONFIG_ROOT", config.c_str(), /*overwrite=*/false);
}

} // namespace core
} // namespace wowee
