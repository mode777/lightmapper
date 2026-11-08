#!/bin/sh
set -eu

cd "$(dirname "$0")"

CC=${CC:-cc}
CFLAGS=${CFLAGS:--O2 -Wall -std=c99}
BUILD=${BUILD:-build}

mkdir -p "$BUILD"

echo "== selector checks =="
$CC $CFLAGS -I.. -DSELECTOR_GLES3 -DEXPECT_GLES3=1 selector-check.c -o "$BUILD/selector-gles3"
"$BUILD/selector-gles3"
$CC $CFLAGS -I.. -DEXPECT_GLES3=0 selector-check.c -o "$BUILD/selector-desktop"
"$BUILD/selector-desktop"
$CC $CFLAGS -I.. -DSELECTOR_GLES3 -DLM_GLES3=0 -DEXPECT_GLES3=0 selector-check.c -o "$BUILD/selector-force-desktop"
"$BUILD/selector-force-desktop"
$CC $CFLAGS -I.. -DLM_GLES3=1 -DEXPECT_GLES3=1 selector-check.c -o "$BUILD/selector-force-gles3"
"$BUILD/selector-force-gles3"

echo "== preprocessor path checks =="
cat > "$BUILD/path-gles3.c" <<'EOF'
#include <GLES3/gl3.h>
#define LIGHTMAPPER_IMPLEMENTATION
#include "lightmapper.h"
EOF
if ! $CC -E -I.. "$BUILD/path-gles3.c" | grep -q '#version 300 es'; then
	echo "GLES3 path does not contain the ES shaders" >&2
	exit 1
fi
if $CC -E -I.. "$BUILD/path-gles3.c" | grep -q '#version 150 core'; then
	echo "GLES3 path contains desktop shaders" >&2
	exit 1
fi

cat > "$BUILD/path-desktop.c" <<'EOF'
#include <GL/gl.h>
#define LIGHTMAPPER_IMPLEMENTATION
#include "lightmapper.h"
EOF
if ! $CC -E -I.. "$BUILD/path-desktop.c" | grep -q '#version 150 core'; then
	echo "desktop path does not contain the desktop shaders" >&2
	exit 1
fi
if $CC -E -I.. "$BUILD/path-desktop.c" | grep -q '#version 300 es'; then
	echo "desktop path contains ES shaders" >&2
	exit 1
fi

echo "== native GLES3 smoke =="
$CC $CFLAGS -I.. gles3-smoke.c -o "$BUILD/gles3-smoke" -lEGL -lGLESv2 -lm
EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 "$BUILD/gles3-smoke"

echo "PASS"
