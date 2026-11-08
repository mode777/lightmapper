# Native GLES3 verification

This directory contains the native GLES3 checks for `lightmapper.h`. They do
not use Emscripten and do not need a browser; the smoke test creates a
surfaceless EGL context on Mesa/llvmpipe.

## Requirements

- A C compiler (`cc` by default).
- GLES3 and EGL development headers (`GLES3/gl3.h`, `EGL/egl.h`).
- `libGLESv2`, `libEGL` and a software GLES3 driver (Mesa llvmpipe is
  sufficient).
- For the desktop selector check, desktop GL headers (`GL/gl.h`).

## Run

```sh
./run-native.sh
```

The runner:

1. Compiles and runs the selector check against GLES3 and desktop headers,
   including the explicit `LM_GLES3=0` and `LM_GLES3=1` overrides.
2. Preprocesses the implementation and checks that the GLES3 build contains
   only `#version 300 es` and the desktop build contains only
   `#version 150 core`.
3. Builds and runs `gles3-smoke`, which creates an EGL surfaceless GLES3
   context, checks `GL_EXT_color_buffer_float`, bakes a small triangle scene
   and asserts that the lightmap is finite and non-zero with no GL error.

Use `BUILD=/path/to/build` to choose the build directory and `CC`/`CFLAGS` to
override the compiler and flags.
