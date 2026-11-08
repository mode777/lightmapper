/*
 * Compile-time selector check for lightmapper.h.
 *
 * Build with -DSELECTOR_GLES3 and -DEXPECT_GLES3=1 against GLES3 headers, or
 * with -DEXPECT_GLES3=0 against desktop GL headers. The explicit override is
 * checked by adding -DLM_GLES3=0 or -DLM_GLES3=1.
 */

#if defined(SELECTOR_GLES3)
#include <GLES3/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "lightmapper.h"

#ifndef EXPECT_GLES3
#error "define EXPECT_GLES3 to 0 or 1"
#endif

#if EXPECT_GLES3
#if !LM_GLES3
#error "expected LM_GLES3 to be 1"
#endif
#else
#if LM_GLES3
#error "expected LM_GLES3 to be 0"
#endif
#endif

#include <stdio.h>

int main(void)
{
	printf("LM_GLES3=%d (expected %d)\n", LM_GLES3, EXPECT_GLES3);
	return LM_GLES3 == EXPECT_GLES3 ? 0 : 1;
}
