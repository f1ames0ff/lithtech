set(FEAR_COMPAT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/fear_holder_shim.cpp")

if(EXISTS "${FEAR_COMPAT_SOURCE}")
    add_library(FEAR_Compat STATIC "${FEAR_COMPAT_SOURCE}")
    set_target_properties(FEAR_Compat PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_compile_features(FEAR_Compat INTERFACE cxx_std_14)
    target_include_directories(FEAR_Compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(FEAR_Compat PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(FEAR_Compat PUBLIC $<$<COMPILE_LANGUAGE:CXX>:/permissive>)
    endif()
else()
    add_library(FEAR_Compat INTERFACE)
    target_compile_features(FEAR_Compat INTERFACE cxx_std_14)
    target_include_directories(FEAR_Compat INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(FEAR_Compat INTERFACE NOMINMAX _CRT_SECURE_NO_WARNINGS)
    if(MSVC)
        target_compile_options(FEAR_Compat INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/permissive>)
    endif()
endif()

unset(FEAR_COMPAT_SOURCE)

add_subdirectory(v5host)
add_subdirectory(cshell)
add_subdirectory(clientfx)
add_subdirectory(cres)
add_subdirectory(sres)
