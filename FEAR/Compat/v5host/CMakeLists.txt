project(FEAR_V5Host)

set(FEAR_V5HOST_SOURCES
    fear_host.cpp
    fear_client_facade.cpp
    fear_server_host.cpp
    fear_resourcemgr_stub.cpp
    fear_common_stub.cpp
    fear_physics_stub.cpp
    fear_physicssim_stub.cpp
    fear_model_stub.cpp
    fear_timer_stub.cpp
    fear_soundmgr_stub.cpp
    fear_cursor_stub.cpp
    fear_filemgr_stub.cpp
    fear_contenttransfer_stub.cpp
    fear_renderer_stub.cpp
    fear_drawprim_stub.cpp
    fear_texturemgr_stub.cpp
    fear_texturestring_stub.cpp
    fear_input_stub.cpp
    fear_gameutil_stub.cpp
    fear_loadingprogress_stub.cpp
    fear_messagewrite_stub.cpp
    fear_messageread_stub.cpp
    ${FEAR_SDK_INC}/ltmodule.cpp
    ${FEAR_SDK_INC}/ltmemorystub.cpp
    ${FEAR_SDK_INC}/ltobjref.cpp)

add_library(${PROJECT_NAME} SHARED ${FEAR_V5HOST_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${FEAR_SDK_INC})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32
    _WINDOWS
    _CLIENTBUILD
    USE_DX8_SOFTWARE_FILTERS
    USE_EAX20_HARDWARE_FILTERS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /FIctype.h /U_FINAL)
endif(MSVC)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME FearHost
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_link_libraries(${PROJECT_NAME} PRIVATE user32 gdi32)
