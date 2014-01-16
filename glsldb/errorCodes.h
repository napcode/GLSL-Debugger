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

#ifndef _ERROR_CODES_H_
#define _ERROR_CODES_H_

enum ErrorCode {
	EC_NONE = 0,

	/* general debugger errors */
	EC_FORK,
	EC_EXEC,
	EC_EXIT,
	EC_UNKNOWN_ERROR,
	EC_MEMORY_ALLOCATION_FAILED,
	EC_STOPPED, 

	/* debuglib errors */
	EC_DBG_NO_ACTIVE_SHADER,
	EC_DBG_NO_SUCH_DBG_FUNC,
	EC_DBG_MEMORY_ALLOCATION_FAILED,
	EC_DBG_DBG_SHADER_COMPILE_FAILED,
	EC_DBG_DBG_SHADER_LINK_FAILED,
	EC_DBG_NO_STORED_SHADER,
	EC_DBG_READBACK_INVALID_COMPONENTS,
	EC_DBG_READBACK_INVALID_FORMAT,
	EC_DBG_READBACK_NOT_ALLOWED,
	EC_DBG_OPERATION_NOT_ALLOWED,
	EC_DBG_INVALID_OPERATION,
	EC_DBG_INVALID_VALUE,

	/* gl errors */
	EC_GL_INVALID_ENUM,
	EC_GL_INVALID_VALUE,
	EC_GL_INVALID_OPERATION,
	EC_GL_STACK_OVERFLOW,
	EC_GL_STACK_UNDERFLOW,
	EC_GL_OUT_OF_MEMORY,
	EC_GL_TABLE_TOO_LARGE,
	EC_GL_INVALID_FRAMEBUFFER_OPERATION_EXT
};

bool isErrorCritical(ErrorCode error);
bool isOpenGLError(ErrorCode error);
const char* getErrorDescription(ErrorCode error);
const char* getErrorInfo(ErrorCode error);

#endif
