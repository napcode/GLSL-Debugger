set(QT_MIN_VERSION 4.4.0)
find_package(Qt4 REQUIRED COMPONENTS QtCore QtNetwork)
include(${QT_USE_FILE})
file(GLOB SRC_CPP *.cpp)

file(GLOB MOC_HEADERS *.qt.h)
file(GLOB RES_FILES *.qrc)
file(GLOB UI_FILES ui/*.ui)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})
qt4_wrap_ui(UI_OUTFILES ${UI_FILES})
qt4_add_resources(RES_OUTFILES ${RES_FILES})

find_package(Protobuf REQUIRED)

set(SRC
    ${SRC_C}
    ${SRC_CPP}
    ${MOC_OUTFILES}
    ${UI_OUTFILES}
    ${RES_OUTFILES}
    ${PROJECT_SOURCE_DIR}/proto/debugger.pb.cc
)

include_directories(${PROJECT_SOURCE_DIR}
    ${X11_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    )
add_library(client STATIC ${SRC})
target_link_libraries(client
    utils 
    ${QT_LIBRARIES}
    ${PROTOBUF_LIBRARIES}
    )
