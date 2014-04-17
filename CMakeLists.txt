cmake_minimum_required(VERSION 2.8.10)
project(glsldb)

option(USE_MESA "Use mesa glsl compiler" OFF)
option(TESTS "Build tests" OFF)
option(BUILD_CLIENT_CLI "Build the commandline client" ON)
option(BUILD_CLIENT_GUI "Build the QT4 GUI client" OFF)

if(TESTS)
	enable_testing()
	if (CMAKE_CONFIGURATION_TYPES)
		add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
			--force-new-ctest-process --output-on-failure
			--build-config "$<CONFIGURATION>")
	else()
		add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
			--force-new-ctest-process --output-on-failure)
	endif()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

# path to custom modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

# Messages with level lower than OUTPUT_LEVEL will be printed
# Levels:  DBGLVL_ERROR = 0,	DBGLVL_WARNING,
#	   DBGLVL_INFO,		DBGLVL_INTERNAL_WARNING,
#	   DBGLVL_COMPILERINFO, DBGLVL_DEBUG,
#	   DBGLVL_ALL = 6
add_definitions(-DOUTPUT_LEVEL=6)

set(BINARY_DIR "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_DIR "${PROJECT_BINARY_DIR}/lib")
set(ARCHIVE_DIR "${PROJECT_BINARY_DIR}/lib")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")

# platform checks
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(GLSLDB_LINUX 1)
    add_definitions(-DGLSLDB_LINUX -D_GNU_SOURCE)
    if(CMAKE_SIZEOF_VOID_P MATCHES "8")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-fPIC" WITH_FPIC)
        if(WITH_FPIC)
            add_definitions(-fPIC)
        endif()
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -Wall -Wfatal-errors -std=gnu99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wfatal-errors -Wextra -std=c++11")
    set(CMAKE_EXE_FLAGS "${CMAKE_EXE_FLAGS} -Wl,--warn-unresolved-symbols,--warn-once")
    set(CMAKE_SHRD_FLAGS "${CMAKE_SHRD_FLAGS} -Wl,--warn-unresolved-symbols,--warn-once")
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(GLSLDB_OSX 1)
	add_definitions(-DGLSLDB_OSX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=gnu99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++0x")
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(GLSLDB_WIN 1)
	set(LIBRARY_DIR "${PROJECT_BINARY_DIR}/bin")
	set(ARCHIVE_DIR "${PROJECT_BINARY_DIR}/bin")
	# Add windows target & disable warnings
	add_definitions(/DDEBUG=1)
	add_definitions(/DGLSLDB_WIN /W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo)
endif()

# Update the documentation string of CMAKE_BUILD_TYPE for GUIs
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	add_definitions(-DGLSLDB_DEBUG)
endif()

set(DISTRIBUTION_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/Dist")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${ARCHIVE_DIR}")

if(USE_MESA)
	message(STATUS "GLSL compiler: mesa")
	add_definitions(-DUSE_MESA)
	add_subdirectory(mesa-glsl)
else()
	message(STATUS "GLSL compiler: native")
	add_subdirectory(GLSLCompiler)
endif()

set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")

# utility library
add_subdirectory(utils)
# debugging library
add_subdirectory(DebugLib)
# debug functions
add_subdirectory(DebugFunctions)
# debugger client library
add_subdirectory(client)

# debugger CLI
if(BUILD_CLIENT_CLI)
    add_subdirectory(client/cli)
endif()
# debugger GUI (change to client/gui)
if(BUILD_CLIENT_GUI)
    add_subdirectory(glsldb)
endif()

if(GLSLDB_WIN)
	add_subdirectory(3rdparty/mhook)
endif()
