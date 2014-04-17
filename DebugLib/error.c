/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
	list of conditions and the following disclaimer in the documentation and/or
	other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
	of its contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#endif /* _WIN32 */

#include "debuglibInternal.h"
#include "utils/notify.h"

#ifdef _WIN32
#include "../GL/wglext.h"
#include "generated/trampolines.h"
#endif /* _WIN32 */

void setErrorCode(int error)
{
    os_pid_t pid = os_getpid();
    thread_locals_t *rec = getThreadLocals(pid);

	UT_NOTIFY(LV_DEBUG, "STORE ERROR: %i", error);
	//FIXME
	//rec->result = DBG_ERROR_CODE;
	//rec->items[0] = (ALIGNED_DATA) error;
}

/* work-around for external debug functions */
/* TODO: do we need debug functions at all? */
DBGLIBEXPORT void DEBUGLIB_EXTERNAL_setErrorCode(int error)
{
	setErrorCode(error);
}

static const char *getErrorString(GLenum error)
{
	switch (error) {
	case GL_INVALID_ENUM:
		return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE:
		return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION:
		return "GL_INVALID_OPERATION";
	case GL_STACK_OVERFLOW:
		return "GL_STACK_OVERFLOW";
	case GL_STACK_UNDERFLOW:
		return "GL_STACK_UNDERFLOW";
	case GL_OUT_OF_MEMORY:
		return "GL_OUT_OF_MEMORY";
	case GL_TABLE_TOO_LARGE:
		return "GL_TABLE_TOO_LARGE";
	case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
		return "GL_INVALID_FRAMEBUFFER_OPERATION_EXT";
	default:
		return "UNKNOWN_ERROR";
	}
}

int glError(void)
{
	GLenum error = ORIG_GL(glGetError)();
	if (error != GL_NO_ERROR) {
		UT_NOTIFY(LV_WARN, "GL ERROR: %s (%i)", getErrorString(error), error);
		return error;
	}
	return GL_NO_ERROR;
}

int setGLErrorCode(void)
{
	int error;
	if ((error = glError())) {
		setErrorCode(error);
		return 1;
	}
	return 0;
}
