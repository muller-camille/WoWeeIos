#!/bin/bash
# Compile the Metal build's C++ as far as a Linux machine can: every source
# given, with WOWEE_METAL on, parsed and type-checked by clang against the
# vendored metal-cpp - whose Apple system headers (the Objective-C runtime,
# CoreFoundation, dispatch and the rest) are the stand-ins under stubs/.
# Nothing is linked and nothing runs; a Mac still builds and runs every
# milestone (docs/plan-metal.md, 7). What this catches is what a cloud
# session would otherwise send to the Mac to find: a misspelt metal-cpp call,
# a type that does not convert, a member that is not there.
#
#   cmake -S . -B build              # once, for build/generated
#   tools/metal/syntax_check/check.sh src/rendering/renderer.cpp ...
#   tools/metal/syntax_check/check.sh $(git grep -l WOWEE_METAL -- 'src/*.cpp')
#
# BUILD_DIR names another build directory; CXX another clang. Prints each
# file with something to say, and exits non-zero if any had.
set -u
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
STUBS="$ROOT/tools/metal/syntax_check/stubs"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CXX="${CXX:-clang++}"
status=0
for f in "$@"; do
  out=$("$CXX" -std=gnu++20 -fblocks -fsyntax-only \
    -Wall -Wextra -Wno-missing-field-initializers -Wno-odr -Wno-unused-private-field \
    -DWOWEE_METAL=1 -DIMGUI_IMPL_METAL_CPP -DGLM_ENABLE_EXPERIMENTAL \
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DWOWEE_AMD_FFX_SDK_KITS=0 -DWOWEE_HAS_AMD_FSR2=0 \
    -DWOWEE_HAS_AMD_FSR3_FRAMEGEN=0 \
    -isystem "$STUBS" -isystem "$ROOT/extern/metal-cpp" \
    -I"$ROOT/include" -I"$ROOT/src" -I"$BUILD_DIR/generated" \
    -isystem "$ROOT/extern" -isystem "$ROOT/extern/vk-bootstrap/src" \
    -isystem "$ROOT/extern/lua-5.1.5/src" -isystem "$ROOT/extern/imgui" \
    -isystem "$ROOT/extern/imgui/backends" \
    "$ROOT/${f#"$ROOT"/}" 2>&1)
  if [ -n "$out" ]; then
    echo "== $f"
    echo "$out" | grep -E 'error|warning'
    status=1
  fi
done
exit $status
