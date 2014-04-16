find_package(ProtoBuf-C REQUIRED)

set(SRC
	hash.c 
	dlutils.c 
	dbgprint.c 
	pfm.c 
	sync.c 
	notify.c
	types.c
    socket.c
    server.c
    connection.c
    queue.c    
    glenumerants.c    
    ${PROJECT_SOURCE_DIR}/proto/protocol.c
    "${PROJECT_SOURCE_DIR}/proto/debugger.pb-c.c"
)

if(GLSLDB_WIN)
	set(SRC ${SRC} getopt.c)
endif()

find_package(Threads REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}
    ${PROTOBUF_INCLUDE_DIR}    
)
add_definitions(${PROTOBUF_DEFINITIONS})
add_library(utils STATIC ${SRC})
target_link_libraries(utils
    ${CMAKE_THREAD_LIBS_INIT}
    ${PROTOBUF_LIBRARIES}
)
