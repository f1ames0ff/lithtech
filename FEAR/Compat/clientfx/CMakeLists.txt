project(FEAR_ClientFxStub)

add_library(${PROJECT_NAME} SHARED ${CMAKE_SOURCE_DIR}/tools/clientfx_stub.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME clientfx
    SUFFIX ".fxd"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
