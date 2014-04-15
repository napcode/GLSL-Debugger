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

#ifdef _WIN32
#include <windows.h>
#include "asprintf.h"
#endif /* _WIN32 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "GL/gl.h"
#include "GL/glext.h"
#include "utils/notify.h"
#include "FunctionCall.h"
#include "FunctionsMap.h"

extern "C" {
#include "utils/glenumerants.h"
#include "errno.h"
#include "utils/types.h"
}

FunctionCall::FunctionCall(const QString& name)
{
	FunctionCall::name(name);
}

FunctionCall::FunctionCall(const FunctionCall& rhs)
{
	name(rhs._name);
	for(auto &arg : rhs._arguments) {
		FunctionArgumentPtr p(new FunctionArgument(*arg));
		_arguments.push_back(p);
	}
}

FunctionCall::~FunctionCall()
{
	_arguments.clear();
}

void FunctionCall::name(const QString& name)
{
	_name = name;
	/* reload extension since name has changed */
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	_extension = QString::fromStdString(f.extname());
}

void FunctionCall::addArgument(DBG_TYPE type, void *data, void *address)
{
	FunctionArgumentPtr arg(new FunctionArgument(type, data, address));
	_arguments.push_back(arg);
}

void FunctionCall::addArgument(FunctionArgumentPtr arg)
{
	_arguments.push_back(arg);
}

bool FunctionCall::operator==(const FunctionCall &rhs)
{
	if (_arguments.size() != rhs._arguments.size()) {
		return false;
	}
	if (_name != rhs._name) {
		return false;
	}

	ArgumentVector::const_iterator i = _arguments.begin();
	ArgumentVector::const_iterator j = rhs._arguments.begin();
	while(i != _arguments.end()) {
		if((**i) != (**j))
			return false;
		++i;
		++j;
	}
	return true;
}

bool FunctionCall::operator!=(const FunctionCall &rhs)
{
	return !operator==(rhs);
}

QString FunctionCall::asString(void) const
{
	QString call(_name);
	call.append("(");

	ArgumentVector::const_iterator it = _arguments.begin();
	while(it != _arguments.end()) {
		call.append((*it)->asString());
		++it;
		if (it != _arguments.end()) 
			call.append(", ");
	}
	call.append(")");

	return call;
}

bool FunctionCall::isDebuggable() const
{
	/* So far we restrict it to draw calls */
	if (isDebuggableDrawCall())
		return true;

	return false;
}

bool FunctionCall::isDebuggable(int *primitiveMode) const
{
	/* So far we restrict it to draw calls */
	if (isDebuggableDrawCall(primitiveMode))
		return true;

	return false;
}

bool FunctionCall::isDebuggableDrawCall(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.is_debuggable())
		return true;
	return false;
}

bool FunctionCall::isDebuggableDrawCall(int *primitiveMode) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.is_debuggable()) {
		int idx = f.primitive_mode_index();
		*primitiveMode = *(GLenum*) _arguments[idx]->data();
		return true;
	}
	*primitiveMode = GL_NONE;
	return false;
}

bool FunctionCall::isEditable(void) const
{
	return !_arguments.isEmpty();
}

bool FunctionCall::isShaderSwitch(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.is_shader_switch())
		return true;
	return false;
}

bool FunctionCall::isGlFunc(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.prefix().compare("GL") == 0)
		return true;
	return false;
}

bool FunctionCall::isGlxFunc(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.prefix().compare("GLX") == 0)
		return true;
	return false;
}

bool FunctionCall::isWglFunc(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.prefix().compare("WGL") == 0)
		return true;
	return false;
}

bool FunctionCall::isFrameEnd(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.is_frame_end())
		return true;
	return false;
}

bool FunctionCall::isFramebufferChange(void) const
{
	const proto::GLFunction& f = FunctionsMap::instance()[_name];
	if (f.is_framebuffer_change())
		return true;
	return false;
}

bool FunctionCall::isDirty() const
{
	for(auto &i : _arguments) {
		if(i->isDirty())
			return true;
	}
	return false;
}
