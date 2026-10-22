project(Test_LTar)

add_executable(${PROJECT_NAME}
    test_ltar.cpp
    ltar_writer.cpp)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME testLtar
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/ltar
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    LIB_LTar)
