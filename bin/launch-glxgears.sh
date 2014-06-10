#!/bin/zsh
GLSL_DEBUGGER_INTERACTIVE=1 GLSL_DEBUGGER_LOGLEVEL=5 LD_PRELOAD=/home/buhr/projects/GLSLDBG/bin/../lib/libglsldebug.so GLSL_DEBUGGER_DBGFCTNS_PATH=/home/buhr/projects/GLSLDBG/bin/../lib/plugins GLSL_DEBUGGER_LIBDLSYM=/home/buhr/projects/GLSLDBG/bin/../lib/libdlsym.so /usr/bin/glxgears
