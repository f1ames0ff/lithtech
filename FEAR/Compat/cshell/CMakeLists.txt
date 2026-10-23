project(FEAR_CShell)

set(FEAR_CSHELL_SOURCES
    cshell_proxy.cpp
    cshell_filemgr.cpp
    cshell_object.cpp
    cshell_render.cpp
    fear_world.cpp
    fear_model.cpp
    dds_loader.cpp
    ${CMAKE_SOURCE_DIR}/sdk/inc/ltmodule.cpp
    ${CMAKE_SOURCE_DIR}/sdk/inc/ltobjref.cpp)

add_library(${PROJECT_NAME} SHARED ${FEAR_CSHELL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/sdk/inc
    ${CMAKE_SOURCE_DIR}/runtime/client/src
    ${CMAKE_SOURCE_DIR}/runtime/kernel/src
    ${CMAKE_SOURCE_DIR}/runtime/kernel/io/src
    ${CMAKE_SOURCE_DIR}/runtime/kernel/mem/src
    ${CMAKE_SOURCE_DIR}/runtime/shared/src
    ${CMAKE_SOURCE_DIR}/runtime/world/src
    ${CMAKE_SOURCE_DIR}/runtime/server/src
    ${CMAKE_SOURCE_DIR}/runtime/model/src
    ${CMAKE_SOURCE_DIR}/runtime/sound/src
    ${CMAKE_SOURCE_DIR}/libs/stdlith
    ${CMAKE_SOURCE_DIR}/libs/lith
    ${CMAKE_SOURCE_DIR}/libs/rezmgr
    ${CMAKE_SOURCE_DIR}/libs/ltmem
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../v5host)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32
    _WINDOWS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LIB_Lith
    LIB_StdLith
    LIB_RezMgr
    LIB_LTMem
    LIB_ZLib
    winmm
    d3d9
    dbghelp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME CShell
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
