set(QT_MIN_VERSION 4.4.0)
find_package(Qt4 REQUIRED COMPONENTS QtCore)
include(${QT_USE_FILE})
file(GLOB SRC_CPP *.cpp)

file(GLOB MOC_HEADERS *.qt.h)
file(GLOB RES_FILES *.qrc)
file(GLOB UI_FILES ui/*.ui)
qt4_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})
qt4_wrap_ui(UI_OUTFILES ${UI_FILES})
qt4_add_resources(RES_OUTFILES ${RES_FILES})

set(SRC
    ${SRC_C}
    ${SRC_CPP}
    ${MOC_OUTFILES}
    ${UI_OUTFILES}
    ${RES_OUTFILES}
)

include_directories(${PROJECT_SOURCE_DIR}
    ${X11_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    )
add_executable(cli ${SRC})
target_link_libraries(cli 
    utils
    client
    ${QT_LIBRARIES}
    )
