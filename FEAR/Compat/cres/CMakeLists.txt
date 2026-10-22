project(FEAR_Cres)

add_library(${PROJECT_NAME} SHARED cres_stub.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME cres)
