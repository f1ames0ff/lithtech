cmake_minimum_required(VERSION 3.20)
project(FEARToolsServer CXX C)

function(fear_write_if_changed output content_var)
    set(content "${${content_var}}")
    if(EXISTS "${output}")
        file(READ "${output}" existing)
        if(existing STREQUAL content)
            return()
        endif()
    endif()
    file(WRITE "${output}" "${content}")
endfunction()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")

set(FEAR_PUBLIC_TOOLS_SOURCE "" CACHE PATH "Path to the F.E.A.R. Public Tools 1.08 Source directory")
if(NOT FEAR_PUBLIC_TOOLS_SOURCE)
    message(FATAL_ERROR "Set -DFEAR_PUBLIC_TOOLS_SOURCE=<path to 'FEAR Public Tools 1.08/Source'>")
endif()
if(NOT EXISTS "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/Game_ServerShell.vcproj")
    message(FATAL_ERROR "FEAR_PUBLIC_TOOLS_SOURCE does not contain Game/ObjectDLL/Game_ServerShell.vcproj")
endif()

set(FEAR_SDK_INC "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/inc")

include("${CMAKE_CURRENT_SOURCE_DIR}/../publictools_client/sources.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/sources.cmake")

file(READ "${FEAR_SDK_INC}/resourceextensions.h" RESEXT_HEADER_TEXT)
string(REGEX MATCHALL "#[ \t]*define[ \t]+RESEXT_[A-Z0-9_]+[ \t]+[^\r\n]+" RESEXT_DEFINES "${RESEXT_HEADER_TEXT}")
set(RESEXT_FALLBACK_TEXT "#pragma once\n")
foreach(RESEXT_DEF IN LISTS RESEXT_DEFINES)
    string(REGEX MATCH "RESEXT_[A-Z0-9_]+" RESEXT_NAME "${RESEXT_DEF}")
    string(APPEND RESEXT_FALLBACK_TEXT "#ifndef ${RESEXT_NAME}\n${RESEXT_DEF}\n#endif\n")
endforeach()
fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/fear_resext_fallback.h" RESEXT_FALLBACK_TEXT)

set(RESEXT_CPP_FILES
    AI.cpp AINode.cpp AIStimulusMgr.cpp AmmoBox.cpp CharacterHitBox.cpp GearItems.cpp
    KeyFramer.cpp LightEditor.cpp Projectile.cpp Prop.cpp ServerSpecialFX.cpp WeaponItems.cpp)

set(RESEXT_MACRO_VALUES
    "RESEXT_EFFECT_PACKED|\"Fx00p\""
    "RESEXT_KEY_COMPRESSED|\"Key00c\""
    "RESEXT_MATERIAL|\"Mat00\""
    "RESEXT_MODEL_COMPRESSED|\"Model00c\""
    "RESEXT_MODEL_PACKED|\"Model00p\""
    "RESEXT_WORLD_PACKED|\"World00p\"")

foreach(SRC_NAME IN LISTS RESEXT_CPP_FILES)
    set(SRC_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/${SRC_NAME}")
    file(READ "${SRC_ORIGINAL}" SRC_TEXT)
    foreach(PAIR IN LISTS RESEXT_MACRO_VALUES)
        string(FIND "${PAIR}" "|" PIPE_POS)
        string(SUBSTRING "${PAIR}" 0 ${PIPE_POS} MACRO_NAME)
        math(EXPR VALUE_POS "${PIPE_POS}+1")
        string(SUBSTRING "${PAIR}" ${VALUE_POS} -1 MACRO_VALUE)
        string(REPLACE "${MACRO_NAME}" "${MACRO_VALUE}" SRC_TEXT "${SRC_TEXT}")
    endforeach()
    fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}" SRC_TEXT)
    list(REMOVE_ITEM SERVER_SHELL_SOURCES "${SRC_ORIGINAL}")
    list(APPEND SERVER_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${SRC_NAME}")
endforeach()

set(CHARACTER_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/Character.cpp")
file(READ "${CHARACTER_ORIGINAL}" CHARACTER_SOURCE_TEXT)
string(REPLACE [=[	m_eCurDlgSndType = eType;

	if( m_eCurDlgSndType == CST_AI_SOUND )]=] [=[	m_eCurDlgSndType = eType;

	float fDialogueTimeout = 4.0f;
	const char *pszDialogueTimeout = getenv("FEAR_DIALOGUE_TIMEOUT");
	if (pszDialogueTimeout && pszDialogueTimeout[0])
		fDialogueTimeout = (float)atof(pszDialogueTimeout);
	m_tmrDialogue.Start(LTMAX(0.25f, fDialogueTimeout));
	printf("FEARDLG: start sound='%s' type=%d timeout=%.2f\n", pSound, (int)eType, fDialogueTimeout);
	fflush(stdout);

	if( m_eCurDlgSndType == CST_AI_SOUND )]=] CHARACTER_SOURCE_TEXT "${CHARACTER_SOURCE_TEXT}")
string(REPLACE [=[	if ( m_tmrDialogue.IsStarted() && m_tmrDialogue.IsTimedOut( ))
	{
		m_tmrDialogue.Stop( );
		m_PlayerTrackerDialogue.Term( );
		KillDlgSnd( );
	}]=] [=[	if ( m_tmrDialogue.IsStarted() && m_tmrDialogue.IsTimedOut( ))
	{
		printf("FEARDLG: timeout type=%d\n", (int)m_eCurDlgSndType);
		fflush(stdout);
		m_tmrDialogue.Stop( );
		m_PlayerTrackerDialogue.Term( );
		KillDlgSnd( );
	}]=] CHARACTER_SOURCE_TEXT "${CHARACTER_SOURCE_TEXT}")
string(REPLACE [=[void CCharacter::KillDlgSnd()
{]=] [=[void CCharacter::KillDlgSnd()
{
	printf("FEARDLG: server kill sound id=%u type=%d\n",
		(unsigned)m_nUniqueDialogueId, (int)m_eCurDlgSndType);
	fflush(stdout);]=] CHARACTER_SOURCE_TEXT "${CHARACTER_SOURCE_TEXT}")
string(REPLACE [=[					uint8 nDialogueUniqueId = pMsg->Readuint8( );
					if( nDialogueUniqueId == m_nUniqueDialogueId )]=] [=[					uint8 nDialogueUniqueId = pMsg->Readuint8( );
					printf("FEARDLG: server completion sender=%p id=%u expected=%u\n",
						(void *)hSender, (unsigned)nDialogueUniqueId, (unsigned)m_nUniqueDialogueId);
					fflush(stdout);
					if( nDialogueUniqueId == m_nUniqueDialogueId )]=] CHARACTER_SOURCE_TEXT "${CHARACTER_SOURCE_TEXT}")
fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/Character.cpp" CHARACTER_SOURCE_TEXT)
list(REMOVE_ITEM SERVER_SHELL_SOURCES "${CHARACTER_ORIGINAL}")
list(APPEND SERVER_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/Character.cpp")

set(DIALOGUE_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/Dialogue.cpp")
file(READ "${DIALOGUE_ORIGINAL}" DIALOGUE_SOURCE_TEXT)
string(REPLACE [=[	StringArray( m_saIcon ).swap( m_saIcon );
}]=] [=[	StringArray( m_saIcon ).swap( m_saIcon );
	printf("FEARDLG: props obj=%p lines=%u cleanup='%s' finished='%s'\n",
		(void *)m_hObject, (unsigned)m_saDialogue.size(),
		m_sCleanUpCommand.c_str(), m_sFinishedCommand.c_str());
	fflush(stdout);
}]=] DIALOGUE_SOURCE_TEXT "${DIALOGUE_SOURCE_TEXT}")
string(REPLACE [=[bool Dialogue::StartDialogue( )
{]=] [=[bool Dialogue::StartDialogue( )
{
	printf("FEARDLG: StartDialogue obj=%p index=%u\n",
		(void *)m_hObject, (unsigned)m_nCurDialogue);
	fflush(stdout);]=] DIALOGUE_SOURCE_TEXT "${DIALOGUE_SOURCE_TEXT}")
string(REPLACE [=[	HOBJECT hSpeaker;
	if( LT_OK == FindNamedObject( szSpeaker, hSpeaker) )
	{]=] [=[	HOBJECT hSpeaker = NULL;
	LTRESULT lrSpeaker = FindNamedObject( szSpeaker, hSpeaker);
	printf("FEARDLG: resolve sound='%s' speaker='%s' result=%d object=%p\n",
		szSound, szSpeaker, (int)lrSpeaker, (void *)hSpeaker);
	fflush(stdout);
	if( LT_OK == lrSpeaker )
	{]=] DIALOGUE_SOURCE_TEXT "${DIALOGUE_SOURCE_TEXT}")
string(REPLACE [=[			if( pChar )
			{
				pChar->PlayDialogue( szSound, m_saIcon[m_nCurDialogue].c_str() , bUseRadioSound);]=] [=[			if( pChar )
			{
				printf("FEARDLG: play sound='%s' character=%p radio=%d\n",
					szSound, (void *)pChar, bUseRadioSound ? 1 : 0);
				fflush(stdout);
				pChar->PlayDialogue( szSound, m_saIcon[m_nCurDialogue].c_str() , bUseRadioSound);]=] DIALOGUE_SOURCE_TEXT "${DIALOGUE_SOURCE_TEXT}")
string(REPLACE [=[void Dialogue::HandleOnMsg( HOBJECT hSender, const CParsedMsg &crParsedMsg )
{]=] [=[void Dialogue::HandleOnMsg( HOBJECT hSender, const CParsedMsg &crParsedMsg )
{
	printf("FEARDLG: ON obj=%p\n", (void *)m_hObject);
	fflush(stdout);]=] DIALOGUE_SOURCE_TEXT "${DIALOGUE_SOURCE_TEXT}")
fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/Dialogue.cpp" DIALOGUE_SOURCE_TEXT)
list(REMOVE_ITEM SERVER_SHELL_SOURCES "${DIALOGUE_ORIGINAL}")
list(APPEND SERVER_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/Dialogue.cpp")

set(SERVERMISSIONMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/ServerMissionMgr.cpp")
file(READ "${SERVERMISSIONMGR_ORIGINAL}" SERVERMISSIONMGR_SOURCE_TEXT)
string(REPLACE [=[void CServerMissionMgr::OnPlayerInWorld( CPlayerObj& player )
{
	if( IsMultiplayerGameServer( ))]=] [=[void CServerMissionMgr::OnPlayerInWorld( CPlayerObj& player )
{
	if( getenv("FEAR_LOCAL_CLIENT") )
	{
		if( !m_bLevelStarted && GetServerGameState() == EServerGameState_Loading )
		{
			printf("FEARSERVER: local single-player level started\n");
			fflush(stdout);
			LevelStarted( );
		}
	}
	else if( IsMultiplayerGameServer( ))]=] SERVERMISSIONMGR_SOURCE_TEXT "${SERVERMISSIONMGR_SOURCE_TEXT}")
fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/ServerMissionMgr.cpp" SERVERMISSIONMGR_SOURCE_TEXT)
list(REMOVE_ITEM SERVER_SHELL_SOURCES "${SERVERMISSIONMGR_ORIGINAL}")
list(APPEND SERVER_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ServerMissionMgr.cpp")

set(PLAYEROBJ_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/PlayerObj.cpp")
file(READ "${PLAYEROBJ_ORIGINAL}" PLAYEROBJ_SOURCE_TEXT)
string(REPLACE [=[bool CPlayerObj::SetPhysicsModel( PlayerPhysicsModel eModel /* = PPM_NORMAL */, 
								 ModelsDB::HMODEL hModel /* = NULL */,
								 bool bUpdateClient /* = true  */ )
{]=] [=[bool CPlayerObj::SetPhysicsModel( PlayerPhysicsModel eModel /* = PPM_NORMAL */, 
								 ModelsDB::HMODEL hModel /* = NULL */,
								 bool bUpdateClient /* = true  */ )
{
	printf("FEARSERVER: SetPhysicsModel old=%d new=%d update=%d model=%p\n",
		(int)m_ePPhysicsModel, (int)eModel, bUpdateClient ? 1 : 0, (void *)hModel);
	fflush(stdout);]=] PLAYEROBJ_SOURCE_TEXT "${PLAYEROBJ_SOURCE_TEXT}")
string(REPLACE [=[bool CPlayerObj::FollowLure( PlayerLure& playerLure )
{]=] [=[bool CPlayerObj::FollowLure( PlayerLure& playerLure )
{
	printf("FEARSERVER: FollowLure obj=%p\n", (void *)playerLure.m_hObject);
	fflush(stdout);]=] PLAYEROBJ_SOURCE_TEXT "${PLAYEROBJ_SOURCE_TEXT}")
string(REPLACE [=[	case MID_DAMAGE:
		{
			DamageStruct damage;
			damage.InitFromMessage(pMsg);

			HandleDamage(damage);
		}
		break;]=] [=[	case MID_DAMAGE:
		{
			DamageStruct damage;
			damage.InitFromMessage(pMsg);
			printf("FEARSERVER: player damage type=%d amount=%.3f damager=%p victim=%p can=%d state=%d\n",
				(int)damage.eType, damage.fDamage, (void *)damage.hDamager, (void *)m_hObject,
				m_damage.GetCanDamage() ? 1 : 0, (int)GetPlayerState());
			fflush(stdout);

			HandleDamage(damage);
		}
		break;]=] PLAYEROBJ_SOURCE_TEXT "${PLAYEROBJ_SOURCE_TEXT}")
string(REPLACE [=[void CPlayerObj::StartDeath()
{]=] [=[void CPlayerObj::StartDeath()
{
	printf("FEARSERVER: StartDeath object=%p state=%d can=%d\n",
		(void *)m_hObject, (int)GetPlayerState(), m_damage.GetCanDamage() ? 1 : 0);
	fflush(stdout);]=] PLAYEROBJ_SOURCE_TEXT "${PLAYEROBJ_SOURCE_TEXT}")
string(REPLACE [=[void CPlayerObj::SetPlayerState(PlayerState eState, bool bForceReset )
{
	if( !bForceReset && eState == m_ePlayerState )]=] [=[void CPlayerObj::SetPlayerState(PlayerState eState, bool bForceReset )
{
	printf("FEARSERVER: SetPlayerState old=%d new=%d force=%d object=%p\n",
		(int)m_ePlayerState, (int)eState, bForceReset ? 1 : 0, (void *)m_hObject);
	fflush(stdout);
	if( !bForceReset && eState == m_ePlayerState )]=] PLAYEROBJ_SOURCE_TEXT "${PLAYEROBJ_SOURCE_TEXT}")
fear_write_if_changed("${CMAKE_CURRENT_BINARY_DIR}/PlayerObj.cpp" PLAYEROBJ_SOURCE_TEXT)
list(REMOVE_ITEM SERVER_SHELL_SOURCES "${PLAYEROBJ_ORIGINAL}")
list(APPEND SERVER_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/PlayerObj.cpp")

set(FEAR_PLATFORM_INCLUDES
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${FEAR_SDK_INC}"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/stdlith"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/platform")

set(FEAR_COMMON_INCLUDES
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${FEAR_SDK_INC}"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/stdlith"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/libs/platform")

set(FEAR_COMMON_DEFINES
    WIN32
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

add_compile_options(/bigobj /EHsc /Zc:forScope- ${FEAR_WARNINGS})

include("${CMAKE_CURRENT_SOURCE_DIR}/sources_shared.cmake")
list(APPEND GAME_SHARED_SERVER_SOURCES
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/SoundFilterDB.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/ForceVolume.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/TriggerTypeDB.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/SoundOcclusionDB.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/AttachmentDB.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/ActivateTypeHandler.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/Shared/SurfaceDB.cpp")
list(APPEND SERVER_SHELL_SOURCES
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/DEditHook.cpp"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ObjectDLL/DestructibleModel.cpp")
add_library(Game_Shared_Server STATIC ${GAME_SHARED_SERVER_SOURCES})
target_include_directories(Game_Shared_Server PUBLIC ${FEAR_COMMON_INCLUDES})
target_compile_definitions(Game_Shared_Server PRIVATE ${FEAR_COMMON_DEFINES} _SERVERBUILD)
target_compile_options(Game_Shared_Server PRIVATE "/FI" "${CMAKE_CURRENT_SOURCE_DIR}/fear_server_prelude.h")
add_library(Shared_StdLith STATIC ${STDLITH_SOURCES})
target_include_directories(Shared_StdLith PUBLIC ${FEAR_PLATFORM_INCLUDES})
target_compile_definitions(Shared_StdLith PRIVATE ${FEAR_COMMON_DEFINES})
target_compile_options(Shared_StdLith PRIVATE "/FI" "${CMAKE_CURRENT_SOURCE_DIR}/fear_platform_prelude.h")

add_library(Shared_Platform STATIC ${PLATFORM_SOURCES})
target_include_directories(Shared_Platform PUBLIC ${FEAR_PLATFORM_INCLUDES})
target_compile_definitions(Shared_Platform PRIVATE ${FEAR_COMMON_DEFINES})
target_compile_options(Shared_Platform PRIVATE "/FI" "${CMAKE_CURRENT_SOURCE_DIR}/fear_platform_prelude.h")

list(FILTER SERVER_SHELL_SOURCES EXCLUDE REGEX "/Game/Shared/")

add_library(Game_ServerShell SHARED
    ${SERVER_SHELL_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/fear_server_bridge.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../publictools_client/crt_compat.cpp")
set_target_properties(Game_ServerShell PROPERTIES OUTPUT_NAME GameServer PREFIX "")
set_target_properties(Game_ServerShell PROPERTIES PDB_NAME GameServerPort)
target_include_directories(Game_ServerShell PRIVATE ${FEAR_COMMON_INCLUDES} "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(Game_ServerShell PRIVATE ${FEAR_COMMON_DEFINES} _SERVERBUILD MEMTRACK_SERVER)
target_compile_options(Game_ServerShell PRIVATE "/FI" "${CMAKE_CURRENT_SOURCE_DIR}/fear_server_prelude.h")
target_link_libraries(Game_ServerShell PRIVATE
    Shared_Platform
    Shared_StdLith
    Game_Shared_Server
    "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/lib/win/Debug/Shared_CRC.lib"
    "${FEAR_PUBLIC_TOOLS_SOURCE}/engine/sdk/lib/win/Debug/Shared_Assert.lib"
    shell32.lib winmm.lib ws2_32.lib shlwapi.lib user32.lib gdi32.lib advapi32.lib
    ole32.lib oleaut32.lib version.lib dinput8.lib dxguid.lib
)
target_link_options(Game_ServerShell PRIVATE /DEBUG /INCREMENTAL:NO)
