/*
 * lua_ios_compat.h - force-included into Lua's loslib.c on iOS only.
 *
 * The iOS SDK declares system() unavailable, and using an unavailable function
 * is an error, not a warning the vendored build's -w could silence. os.execute
 * is Lua's only caller, and nothing this client loads calls os.execute.
 *
 * A forced include rather than a -D: <stdlib.h> is read first, so the macro
 * cannot rewrite its declaration of system(), and every later include of it is
 * already guarded. With no command, os.execute asks whether a shell exists,
 * and 0 is how a platform without one answers; with one, it fails.
 */
#ifndef WOWEE_LUA_IOS_COMPAT_H
#define WOWEE_LUA_IOS_COMPAT_H

#include <stdlib.h>

static inline int wowee_ios_no_system(const char *command) {
    return command ? -1 : 0;
}

#define system(command) wowee_ios_no_system(command)

#endif
