cmake_minimum_required(VERSION 3.10)
project(Lithtech)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_ENGINE "Build engine" ON)
option(BUILD_NOLF2 "Build No One Lives Forever 2 game code" ON)
option(BUILD_TRON "Build Tron 2.0 game code" OFF)
option(BUILD_BLOOD2 "Build Blood II game code" OFF)
option(BUILD_FEAR "Build F.E.A.R. game code" OFF)
option(BUILD_NOLF "Build No One Lives Forever game code" OFF)
option(BUILD_SHOGO "Build Shogo game code" OFF)
option(BUILD_TOOLS "Build tools" OFF)
option(BUILD_FEAR_PORT "Build the F.E.A.R. port" OFF)
option(BUILD_LTAR_TOOLS "Build ltar tools and tests" ON)
set(FEAR_SDK_DIR "" CACHE PATH "F.E.A.R. SDK root containing engine/sdk/inc")
set(FEAR_RETAIL_DIR "" CACHE PATH "F.E.A.R. retail installation root")
if(WIN32)
option(ENABLE_D3D "Enable Direct3D" ON)
option(ENABLE_DIRECTX "Enable DirectX" ON)
option(ENABLE_DIRECTSHOW "Enable DirectShow" ON)
else(WIN32)
option(ENABLE_OPENGL "Enable OpenGL 3.3" ON)
option(ENABLE_VULKAN "Enable Vulkan" ON)
endif(WIN32)
option(USE_INTERNAL_ZLIB "Use bundled zlib" ON)			# eventually want default to be OFF
option(USE_INTERNAL_FREETYPE "Use bundled freetype" ON)	# eventually want default to be OFF

if(BUILD_FEAR_PORT)
    set(BUILD_NOLF2 OFF)
    set(BUILD_FEAR OFF)
endif(BUILD_FEAR_PORT)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/OUT")
if(BUILD_FEAR_PORT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/OUT_FEAR")
endif(BUILD_FEAR_PORT)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

if(ENABLE_D3D)
    add_definitions(-D__D3D -D__D3DREND)
endif(ENABLE_D3D)


if(WIN32)
    add_definitions(-D_WINDOWS)
else(WIN32)
    add_definitions(-D_LINUX -D__LINUX)
    set(LINUX 1)
endif(WIN32)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-D_AMD64_)
    else()
        add_definitions(-D_X86_)
    endif()
endif(MSVC)

if(WIN32)
    if(NOT DXSDK_DIR)
        if(DEFINED ENV{DXSDK_DIR})
            set(DXSDK_DIR "$ENV{DXSDK_DIR}")
        elseif(EXISTS "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
            set(DXSDK_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)")
        elseif(EXISTS "C:/Program Files (x86)/Microsoft DirectX SDK (August 2007)")
            set(DXSDK_DIR "C:/Program Files (x86)/Microsoft DirectX SDK (August 2007)")
        endif()
    endif()
    if(DXSDK_DIR)
        file(TO_CMAKE_PATH "${DXSDK_DIR}" DXSDK_DIR)
        set(DXSDK_INCLUDE_DIR "${DXSDK_DIR}/Include")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(DXSDK_LIB_DIR "${DXSDK_DIR}/Lib/x64")
        else()
            set(DXSDK_LIB_DIR "${DXSDK_DIR}/Lib/x86")
        endif()
    else()
        message(FATAL_ERROR "DirectX SDK not found. Set DXSDK_DIR to the DirectX SDK root (for example C:/tools/dxsdk_jun10).")
    endif()
endif(WIN32)

if(ENABLE_DIRECTSHOW)
    add_definitions(-D__ENABLE_DIRECTSHOW)
endif(ENABLE_DIRECTSHOW)

if(DEBUG)
    add_definitions(-D_DEBUG)
else(DEBUG)
    add_definitions(-D_FINAL)
endif(DEBUG)

if(WIN32)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    link_directories(${SDL2_LIBRARY_DIRS})
endif(WIN32)


if(USE_INTERNAL_ZLIB)
    #set zlib vars
    add_subdirectory(libs/zlib)							# LIB_ZLib
else(USE_INTERNAL_ZLIB)
    find_package(ZLIB)
endif(USE_INTERNAL_ZLIB)

# move to Lib_UI CMakeLists.txt?
if(USE_INTERNAL_FREETYPE)
    #set freetype vars
else(USE_INTERNAL_FREETYPE)
    find_package(Freetype)
endif(USE_INTERNAL_FREETYPE)


add_subdirectory(libs/lith)								# LIB_Lith
add_subdirectory(libs/stdlith)							# LIB_StdLith
add_subdirectory(libs/ltmem)							# LIB_LTMem
add_subdirectory(libs/rezmgr)							# LIB_RezMgr
add_subdirectory(libs/ltar)

if(LINUX)
    set(JSON_BuildTests off)
    add_subdirectory(libs/DynRes)
    add_subdirectory(json)
endif(LINUX)

if(BUILD_ENGINE)
    add_subdirectory(runtime/ui)						# LIB_UI
    add_subdirectory(runtime/info)						# LIB_Info
    add_subdirectory(runtime/sound)						# LIB_ILTSound
    add_subdirectory(runtime/server)					# DLL_Server
    add_subdirectory(runtime/client)					# EXE_Lithtech

    if(WIN32)
        if(ENABLE_D3D)
            add_subdirectory(runtime/render_a/src/sys/d3d)	# LIB_D3DRender
        endif(ENABLE_D3D)

        if(ENABLE_DIRECTX)
            add_subdirectory(runtime/sound/src/sys/s_dx8)	# DLL_SndDrvDX8
        endif(ENABLE_DIRECTX)

        if(ENABLE_DIRECTSHOW)
            add_subdirectory(libs/dshow)					# LIB_DShow
        endif(ENABLE_DIRECTSHOW)

        add_subdirectory(runtime/winbuild/ltmsg)		# DLL_LTMsg
    endif(WIN32)
endif(BUILD_ENGINE)

if(BUILD_NOLF2)
    add_subdirectory(libs/MFCStub)						# LIB_MFCStub
    add_subdirectory(libs/ButeMgr)						# LIB_ButeMgr
    add_subdirectory(libs/CryptMgr)						# LIB_CryptMgr
    add_subdirectory(libs/RegMgr)						# LIB_RegMgr
    add_subdirectory(libs/LTGUIMgr)						# LIB_LTGUIMgr
    add_subdirectory(libs/ServerDir)					# ServerDir
    add_subdirectory(libs/WONAPI)						# LIB_WONAPI
    add_subdirectory(NOLF2/ClientFxDLL)					# ClientFXDLL
    add_subdirectory(NOLF2/ClientRes/TO2)				# ClientRes
    add_subdirectory(NOLF2/ClientShellDLL/TO2)			# ClientShellDLL
    add_subdirectory(NOLF2/ObjectDLL/TO2)				# ObjectDLL
    add_subdirectory(NOLF2/ServerRes/TO2)				# ServerRes
endif(BUILD_NOLF2)

if(BUILD_FEAR)
    if(NOT BUILD_NOLF2)
        add_subdirectory(libs/MFCStub)						# LIB_MFCStub
        add_subdirectory(libs/ButeMgr)						# LIB_ButeMgr
        add_subdirectory(libs/CryptMgr)						# LIB_CryptMgr
        add_subdirectory(libs/RegMgr)						# LIB_RegMgr
        add_subdirectory(libs/LTGUIMgr)						# LIB_LTGUIMgr
        add_subdirectory(libs/ServerDir)					# ServerDir
        add_subdirectory(libs/WONAPI)						# LIB_WONAPI
    endif(NOT BUILD_NOLF2)
    add_subdirectory(FEAR/ClientFxDLL)					# ClientFXDLL
#	add_subdirectory(NOLF2/ClientRes/TO2)				# ClientRes
    add_subdirectory(FEAR/ClientShellDLL)				# ClientShellDLL
#	add_subdirectory(NOLF2/ObjectDLL/TO2)				# ObjectDLL
#	add_subdirectory(NOLF2/ServerRes/TO2)				# ServerRes
endif(BUILD_FEAR)

if(BUILD_FEAR_PORT)
    add_subdirectory(FEAR)
endif(BUILD_FEAR_PORT)

if(BUILD_TRON)
    if(NOT BUILD_NOLF2)
        add_subdirectory(libs/MFCStub)
        add_subdirectory(libs/ButeMgr)
        add_subdirectory(libs/CryptMgr)
        add_subdirectory(libs/RegMgr)
        add_subdirectory(libs/LTGUIMgr)
        add_subdirectory(libs/ServerDir)
        add_subdirectory(libs/WONAPI)
        add_subdirectory(NOLF2/ClientFxDLL)		# ClientFXDLL
    endif(NOT BUILD_NOLF2)
    add_subdirectory(NOLF2/ClientRes/TRON)		# ClientRes
    add_subdirectory(NOLF2/ClientShellDLL/TRON)	# ClientShellDLL
    add_subdirectory(NOLF2/ObjectDLL/TRON)		# ObjectDLL
    add_subdirectory(NOLF2/ServerRes/TRON)		# ServerRes
endif(BUILD_TRON)

if(BUILD_TOOLS)
    add_subdirectory(tools/DtxView)			# TOOLS_DtxView
    add_subdirectory(tools/LithRez)			# TOOLS_LithRez
endif(BUILD_TOOLS)

if(BUILD_FEAR_PORT OR BUILD_LTAR_TOOLS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ltar_dump/CMakeLists.txt")
        add_subdirectory(tools/ltar_dump)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/ltar/CMakeLists.txt")
        add_subdirectory(tests/ltar)
    endif()
endif(BUILD_FEAR_PORT OR BUILD_LTAR_TOOLS)

if(BUILD_NOLF2 OR BUILD_FEAR)
    add_subdirectory(tests/RegMgr)
endif(BUILD_NOLF2 OR BUILD_FEAR)
if(LINUX)
    add_subdirectory(tests/CShell)
    add_subdirectory(tests/DynRes)
    add_subdirectory(tests/DynResDLL)
endif(LINUX)
