include_guard(GLOBAL)

if(NOT FEAR_SDK_DIR)
    message(FATAL_ERROR "FEAR_SDK_DIR is not set. Point it to the F.E.A.R. SDK root that contains engine/sdk/inc/iclientshell.h.")
endif()

file(TO_CMAKE_PATH "${FEAR_SDK_DIR}" FEAR_SDK_DIR)
get_filename_component(FEAR_SDK_DIR "${FEAR_SDK_DIR}" ABSOLUTE BASE_DIR "${CMAKE_SOURCE_DIR}")

if(NOT IS_DIRECTORY "${FEAR_SDK_DIR}")
    message(FATAL_ERROR "FEAR_SDK_DIR is not a directory: ${FEAR_SDK_DIR}")
endif()

set(FEAR_SDK_INC "${FEAR_SDK_DIR}/engine/sdk/inc")

if(NOT EXISTS "${FEAR_SDK_INC}/iclientshell.h")
    message(FATAL_ERROR "FEAR_SDK_DIR does not contain engine/sdk/inc/iclientshell.h: ${FEAR_SDK_DIR}")
endif()

set(FEAR_SDK_LIBS "")
foreach(FEAR_SDK_LIB platform stdlith)
    if(EXISTS "${FEAR_SDK_DIR}/libs/${FEAR_SDK_LIB}")
        list(APPEND FEAR_SDK_LIBS "${FEAR_SDK_DIR}/libs/${FEAR_SDK_LIB}")
    endif()
endforeach()
unset(FEAR_SDK_LIB)
