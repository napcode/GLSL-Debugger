#!/bin/zsh
GLSL_DEBUGGER_INTERACTIVE=1 GLSL_DEBUGGER_LOGLEVEL=5 LD_PRELOAD=lib/libglsldebug.so GLSL_DEBUGGER_DBGFCTNS_PATH=lib/plugins GLSL_DEBUGGER_LIBDLSYM=lib/libdlsym.so valgrind /usr/bin/glxgears
