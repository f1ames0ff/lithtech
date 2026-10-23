cmake_minimum_required(VERSION 3.20)
project(FEARToolsClient CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")

set(FEAR_PUBLIC_TOOLS_SOURCE "" CACHE PATH "Path to the F.E.A.R. Public Tools 1.08 Source directory")
if(NOT FEAR_PUBLIC_TOOLS_SOURCE)
    message(FATAL_ERROR "Set -DFEAR_PUBLIC_TOOLS_SOURCE=<path to 'FEAR Public Tools 1.08/Source'>")
endif()
if(NOT EXISTS "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/Game_ClientShell.vcproj")
    message(FATAL_ERROR "FEAR_PUBLIC_TOOLS_SOURCE does not contain Game/ClientShellDLL/Game_ClientShell.vcproj")
endif()

set(FEAR_SDK_INC "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/inc")

include("${CMAKE_CURRENT_SOURCE_DIR}/sources.cmake")

set(HUDCHAT_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/HUDChatInput.cpp")
file(READ "${HUDCHAT_ORIGINAL}" HUDCHAT_SOURCE_TEXT)
string(REPLACE "L''" "L'\\0'" HUDCHAT_SOURCE_TEXT "${HUDCHAT_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/HUDChatInput.cpp" "${HUDCHAT_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${HUDCHAT_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/HUDChatInput.cpp")

set(SCREENMAIN_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/ScreenMain.cpp")
file(READ "${SCREENMAIN_ORIGINAL}" SCREENMAIN_SOURCE_TEXT)
string(REPLACE "#include \"ltgamecfg.h\"" "#include \"ltgamecfg.h\"\n#include \"ltquaduvutils.h\"\n#include \"iltdrawprim.h\"" SCREENMAIN_SOURCE_TEXT "${SCREENMAIN_SOURCE_TEXT}")
string(REPLACE "if (!CBaseScreen::Build()) return false;" [=[	if (m_hBackTexture == (HTEXTURE)NULL)
	{
		m_hBackTexture.Load("Interface\\menu\\skins\\screen_background_2.dds");
		if (m_hBackTexture != (HTEXTURE)NULL)
		{
			SetupQuadUVs(m_BackPoly, m_hBackTexture, 0.0f, 0.0f, 1.0f, 1.0f);
			DrawPrimSetRGBA(m_BackPoly, 0xFF606060);
			DrawPrimSetXYWH(m_BackPoly, 0.0f, 0.0f, 640.0f, 480.0f);
		}
	}

	if (!CBaseScreen::Build()) return false;]=] SCREENMAIN_SOURCE_TEXT "${SCREENMAIN_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ScreenMain.cpp" "${SCREENMAIN_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${SCREENMAIN_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ScreenMain.cpp")

# Port-side hardening. The retail client always has a fully resolved player body
# model, so these paths assume non-NULL animation contexts and profile data.
set(PLAYERBODYMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/PlayerBodyMgr.cpp")
file(READ "${PLAYERBODYMGR_ORIGINAL}" PLAYERBODYMGR_SOURCE_TEXT)
string(REPLACE [=[	if( !m_hPlayerBody )
		return false;

	const CAnimationContext *apContexts[kNumContexts]]=] [=[	if( !m_hPlayerBody || !m_hModel || !m_pLowerAnimationContext )
		return false;

	const CAnimationContext *apContexts[kNumContexts]]=] PLAYERBODYMGR_SOURCE_TEXT "${PLAYERBODYMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PlayerBodyMgr.cpp" "${PLAYERBODYMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${PLAYERBODYMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/PlayerBodyMgr.cpp")

set(CHARACTERFX_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/CharacterFX.cpp")
file(READ "${CHARACTERFX_ORIGINAL}" CHARACTERFX_SOURCE_TEXT)
string(REPLACE [=[if (m_bSeverBody && !g_pProfileMgr->GetCurrentProfile()->m_bGore)]=] [=[if (m_bSeverBody && g_pProfileMgr->GetCurrentProfile() && !g_pProfileMgr->GetCurrentProfile()->m_bGore)]=] CHARACTERFX_SOURCE_TEXT "${CHARACTERFX_SOURCE_TEXT}")
string(REPLACE [=[if (!g_pProfileMgr->GetCurrentProfile()->m_bAllowBroadcast)]=] [=[if (g_pProfileMgr->GetCurrentProfile() && !g_pProfileMgr->GetCurrentProfile()->m_bAllowBroadcast)]=] CHARACTERFX_SOURCE_TEXT "${CHARACTERFX_SOURCE_TEXT}")
string(REPLACE [=[if (!g_pProfileMgr->GetCurrentProfile()->m_bGore)]=] [=[if (g_pProfileMgr->GetCurrentProfile() && !g_pProfileMgr->GetCurrentProfile()->m_bGore)]=] CHARACTERFX_SOURCE_TEXT "${CHARACTERFX_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CharacterFX.cpp" "${CHARACTERFX_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${CHARACTERFX_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/CharacterFX.cpp")

set(FEAR_COMMON_INCLUDES
    "${FEAR_SDK_INC}"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Libs/LTGUIMgr"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/stdlith"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/platform"
)

set(FEAR_COMMON_DEFINES
    WIN32
    _DEBUG
    _WINDOWS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
    _WIN32_WINNT=0x0501
    _WIN32_IE=0x0500
    USE_DX8_SOFTWARE_FILTERS
    USE_EAX20_HARDWARE_FILTERS
    _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
)

set(FEAR_WARNINGS
    /wd4996 /wd4100 /wd4244 /wd4267 /wd4305 /wd4018 /wd4838 /wd4505
    /wd4189 /wd4127 /wd4701 /wd4702 /wd4703 /wd4706 /wd4201 /wd4204
    /wd4291 /wd4324 /wd4456 /wd4457 /wd4458 /wd4459 /wd4214
)

add_compile_options(/bigobj /EHsc /Zc:forScope- "/FI${CMAKE_CURRENT_SOURCE_DIR}/fear_prelude.h" ${FEAR_WARNINGS})

add_library(Shared_StdLith STATIC ${STDLITH_SOURCES})
target_include_directories(Shared_StdLith PUBLIC ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Shared_StdLith PRIVATE ${FEAR_COMMON_DEFINES})

add_library(Shared_Platform STATIC ${PLATFORM_SOURCES})
target_include_directories(Shared_Platform PUBLIC ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Shared_Platform PRIVATE ${FEAR_COMMON_DEFINES})

add_library(Game_Shared_Client STATIC ${SHARED_CLIENT_SOURCES})
target_include_directories(Game_Shared_Client PUBLIC ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Game_Shared_Client PRIVATE ${FEAR_COMMON_DEFINES} _LIB _CLIENTBUILD)
target_link_libraries(Game_Shared_Client PUBLIC Shared_Platform Shared_StdLith)

add_library(Game_GUIMgr STATIC ${GUI_MGR_SOURCES})
target_include_directories(Game_GUIMgr PUBLIC ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Game_GUIMgr PRIVATE ${FEAR_COMMON_DEFINES})
target_link_libraries(Game_GUIMgr PUBLIC Shared_Platform Shared_StdLith)

add_library(Game_ClientShell SHARED ${CLIENT_SHELL_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/crt_compat.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/client_input_bridge.cpp")
set_target_properties(Game_ClientShell PROPERTIES OUTPUT_NAME GameClient PREFIX "")
target_include_directories(Game_ClientShell PRIVATE ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Game_ClientShell PRIVATE ${FEAR_COMMON_DEFINES} _CLIENTBUILD MEMTRACK_CLIENT)
target_link_libraries(Game_ClientShell PRIVATE
    Game_Shared_Client
    Game_GUIMgr
    Shared_StdLith
    Shared_Platform
    "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/lib/win/Debug/Shared_CRC.lib"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/lib/win/Debug/Shared_Assert.lib"
    shell32.lib winmm.lib ws2_32.lib shlwapi.lib user32.lib gdi32.lib advapi32.lib
    ole32.lib oleaut32.lib version.lib dinput8.lib dxguid.lib
)
target_link_options(Game_ClientShell PRIVATE /DEBUG /INCREMENTAL:NO)
