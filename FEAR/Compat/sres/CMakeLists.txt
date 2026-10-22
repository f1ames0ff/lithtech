project(FEAR_Sres)

add_library(${PROJECT_NAME} SHARED ../cres/cres_stub.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME sres)
