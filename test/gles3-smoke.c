/*
 * Native GLES3 smoke test for lightmapper.h.
 *
 * Creates an EGL surfaceless GLES3 context on Mesa/llvmpipe, checks the float
 * render-target extension, bakes a small triangle scene and verifies that the
 * ES shader and FBO readback paths are selected and produce finite, non-zero
 * output.
 */

#define _POSIX_C_SOURCE 200809L

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIGHTMAPPER_IMPLEMENTATION
#include "lightmapper.h"

#if !LM_GLES3
#error "GLES3 headers did not select the ES path"
#endif

#ifndef EGL_PLATFORM_SURFACELESS_MESA
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

static void fail(const char *message)
{
	fprintf(stderr, "gles3-smoke: %s\n", message);
	exit(1);
}

static EGLDisplay open_display(void)
{
	PFNEGLGETPLATFORMDISPLAYEXTPROC get_platform_display =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
	if (get_platform_display)
	{
		EGLDisplay display = get_platform_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
		if (display != EGL_NO_DISPLAY)
			return display;
	}

	setenv("EGL_PLATFORM", "surfaceless", 1);
	return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

static EGLConfig choose_config(EGLDisplay display)
{
	const EGLint attributes[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_NONE
	};
	EGLConfig config = NULL;
	EGLint count = 0;

	if (!eglChooseConfig(display, attributes, &config, 1, &count) || count < 1)
		fail("eglChooseConfig found no GLES3 config");
	return config;
}

static void make_current(EGLDisplay display, EGLConfig config, EGLContext context)
{
	if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context))
		return;

	const EGLint pbuffer_attributes[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
	EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attributes);
	if (surface == EGL_NO_SURFACE || !eglMakeCurrent(display, surface, surface, context))
		fail("eglMakeCurrent failed for surfaceless and pbuffer contexts");
}

static void check_capabilities(void)
{
	const char *version = (const char *)glGetString(GL_VERSION);
	const char *extensions = (const char *)glGetString(GL_EXTENSIONS);

	if (!version || strncmp(version, "OpenGL ES 3", 11) != 0)
		fail("context is not OpenGL ES 3");
	if (!extensions || !strstr(extensions, "GL_EXT_color_buffer_float"))
		fail("GL_EXT_color_buffer_float is required for RGBA32F readback");
}

static void bake(lm_context *ctx)
{
	enum { LIGHTMAP_WIDTH = 128, LIGHTMAP_HEIGHT = 128 };
	static float lightmap[LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 4];
	static const float positions[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f
	};
	static const float lightmapCoords[] = {
		0.0f, 0.0f,
		0.5f, 0.0f,
		0.5f, 0.5f,
		0.0f, 0.5f
	};
	static const unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };
	int viewport[4];
	float view[16];
	float projection[16];
	int passes = 0;
	int nonzero = 0;
	int i;

	memset(lightmap, 0, sizeof(lightmap));
	lmSetTargetLightmap(ctx, lightmap, LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, 4);
	lmSetGeometry(
		ctx,
		NULL,
		LM_FLOAT, positions, 3 * (int)sizeof(float),
		LM_NONE, NULL, 0,
		LM_FLOAT, lightmapCoords, 2 * (int)sizeof(float),
		6, LM_UNSIGNED_SHORT, indices);

	while (lmBegin(ctx, viewport, view, projection))
	{
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lmEnd(ctx);

		passes++;
		if (passes > 100000)
			fail("bake did not finish");
	}

	if (passes == 0)
		fail("bake produced no hemisphere passes");

	for (i = 0; i < LIGHTMAP_WIDTH * LIGHTMAP_HEIGHT * 4; i++)
	{
		if (!isfinite(lightmap[i]))
			fail("lightmap contains a non-finite value");
		if (lightmap[i] > 0.0f)
			nonzero++;
	}

	if (nonzero == 0)
		fail("lightmap is all zero");

	printf("PASS: passes=%d nonzero=%d\n", passes, nonzero);
}

int main(void)
{
	EGLDisplay display = open_display();
	EGLConfig config;
	EGLContext context;
	lm_context *ctx;
	GLenum error;
	EGLint major = 0;
	EGLint minor = 0;
	const EGLint context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

	if (display == EGL_NO_DISPLAY)
		fail("no EGL display");
	if (!eglInitialize(display, &major, &minor))
		fail("eglInitialize failed");
	if (!eglBindAPI(EGL_OPENGL_ES_API))
		fail("eglBindAPI(EGL_OPENGL_ES_API) failed");

	config = choose_config(display);
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
	if (context == EGL_NO_CONTEXT)
		fail("eglCreateContext failed");
	make_current(display, config, context);
	check_capabilities();

	while (glGetError() != GL_NO_ERROR)
		;

	ctx = lmCreate(16, 0.001f, 100.0f, 1.0f, 1.0f, 1.0f, 0, 0.01f, 0.0f);
	if (!ctx)
		fail("lmCreate failed; the ES shader path was not selected or the context is unsupported");

	bake(ctx);
	lmDestroy(ctx);

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		fprintf(stderr, "gles3-smoke: GL error 0x%04X after the bake\n", (unsigned int)error);
		return 1;
	}

	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(display, context);
	eglTerminate(display);
	return 0;
}
