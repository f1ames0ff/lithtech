project(TOOLS_LTarDump)

add_executable(${PROJECT_NAME}
    main.cpp)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME ltar_dump
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/ltar)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LIB_LTar)
