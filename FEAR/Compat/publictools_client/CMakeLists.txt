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
string(REPLACE [=[bool CPlayerBodyMgr::Init()
{]=] [=[bool CPlayerBodyMgr::Init()
{
	printf("FEARBODY: init begin\n");
	fflush(stdout);]=] PLAYERBODYMGR_SOURCE_TEXT "${PLAYERBODYMGR_SOURCE_TEXT}")
string(REPLACE [=[	if( !m_hPlayerBody )
		return false;

	const CAnimationContext *apContexts[kNumContexts]]=] [=[	if( !m_hPlayerBody || !m_hModel || !m_pLowerAnimationContext )
		return false;

	const CAnimationContext *apContexts[kNumContexts]]=] PLAYERBODYMGR_SOURCE_TEXT "${PLAYERBODYMGR_SOURCE_TEXT}")
string(REPLACE [=[bool CPlayerBodyMgr::OnModelKey( HLOCALOBJ hObj, ANIMTRACKERID hTrackerID, ArgList* pArgList )
{]=] [=[bool CPlayerBodyMgr::OnModelKey( HLOCALOBJ hObj, ANIMTRACKERID hTrackerID, ArgList* pArgList )
{
	if( hObj == m_hPlayerBody && pArgList && pArgList->argc > 0 &&
		pArgList->argv && pArgList->argv[0] &&
		stricmp( pArgList->argv[0], "FIRE_KEY" ) == 0 )
	{
		printf("FEARBODY: fire-key tracker=%u upper=%u lower=%u enabled=%d args=%d\n",
			(unsigned)hTrackerID,
			m_pUpperAnimationContext ? (unsigned)m_pUpperAnimationContext->GetTrackerID() : 0xFFFFFFFFu,
			m_pLowerAnimationContext ? (unsigned)m_pLowerAnimationContext->GetTrackerID() : 0xFFFFFFFFu,
			m_bEnabled ? 1 : 0,
			pArgList->argc );
		fflush(stdout);
	}]=] PLAYERBODYMGR_SOURCE_TEXT "${PLAYERBODYMGR_SOURCE_TEXT}")
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

set(CLIENTWEAPON_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/ClientWeapon.cpp")
file(READ "${CLIENTWEAPON_ORIGINAL}" CLIENTWEAPON_SOURCE_TEXT)
string(REPLACE [=[	HATTRIBUTE hAttrib = NULL;
	HWEAPONDATA hWpnData = g_pWeaponDB->GetWeaponData( m_hWeaponRecord, !USE_AI_DATA );
	
	if( g_vtPlayerBodyWeapons.GetFloat() >= 1.0f )
	{
		hAttrib = g_pWeaponDB->GetStructAttribute( m_hWeaponModelStruct, 0, WDB_WEAPON_sHHModel );
		rOCS.SetFileName( g_pWeaponDB->GetString( hAttrib ));]=] [=[	HATTRIBUTE hAttrib = NULL;
	HWEAPONDATA hWpnData = g_pWeaponDB->GetWeaponData( m_hWeaponRecord, !USE_AI_DATA );
	printf("FEARMODELDATA: populate vt=%.3f initted=%d record=%p struct=%p struct_name='%s'\n",
		g_vtPlayerBodyWeapons.GetFloat(),
		g_vtPlayerBodyWeapons.IsInitted() ? 1 : 0,
		(void *)m_hWeaponRecord,
		(void *)m_hWeaponModelStruct,
		m_hWeaponModelStruct ? g_pLTDatabase->GetAttributeName( m_hWeaponModelStruct ) : "");
	fflush(stdout);
	
	if( g_vtPlayerBodyWeapons.GetFloat() >= 1.0f )
	{
		hAttrib = g_pWeaponDB->GetStructAttribute( m_hWeaponModelStruct, 0, WDB_WEAPON_sHHModel );
		printf("FEARMODELDATA: hHModel=%p attr_name='%s' filename='%s'\n",
			(void *)hAttrib,
			hAttrib ? g_pLTDatabase->GetAttributeName( hAttrib ) : "",
			g_pWeaponDB->GetString( hAttrib ));
		fflush(stdout);
		rOCS.SetFileName( g_pWeaponDB->GetString( hAttrib ));]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[bool CWeaponModelData::CreateWeaponModel( const char *pszSocket )
{
	if( m_hObject )]=] [=[bool CWeaponModelData::CreateWeaponModel( const char *pszSocket )
{
	printf("FEARMODELDATA: begin socket='%s' object=%p weapon=%p model_struct=%p\n",
		pszSocket ? pszSocket : "",
		(void *)m_hObject,
		(void *)m_hWeaponRecord,
		(void *)m_hWeaponModelStruct);
	fflush(stdout);
	if( m_hObject )]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[	ObjectCreateStruct ocs;
	PopulateCreateStruct( ocs );
	
	m_hObject = CreateModelObject( m_hObject, ocs );]=] [=[	ObjectCreateStruct ocs;
	PopulateCreateStruct( ocs );
	printf("FEARMODELDATA: populated filename='%s' type=%d flags=%08x material0='%s'\n",
		ocs.m_Filename,
		(int)ocs.m_ObjectType,
		ocs.m_Flags,
		ocs.m_Materials[0]);
	fflush(stdout);
	
	m_hObject = CreateModelObject( m_hObject, ocs );
	printf("FEARMODELDATA: created object=%p\n", (void *)m_hObject);
	fflush(stdout);]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[WeaponState CClientWeapon::Update( )
{]=] [=[WeaponState CClientWeapon::Update( )
{
	static int s_nDebugWeaponUpdates = 0;
	const bool bDebugCanFire = g_pPlayerMgr && g_pPlayerMgr->CanFireWeapon();
	if (bDebugCanFire || ((++s_nDebugWeaponUpdates % 120) == 1))
	{
		printf("FEARWEAPON: update disabled=%d paused=%d state=%d canfire=%d handled=%d semi=%d\n",
			m_bDisabled ? 1 : 0,
			m_bPaused ? 1 : 0,
			(int)GetState(),
			bDebugCanFire ? 1 : 0,
			m_bFireHandled ? 1 : 0,
			m_bSemiAutoLock ? 1 : 0);
		fflush(stdout);
	}]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[	bool bFire = g_pPlayerMgr->CanFireWeapon( ) && !m_bSemiAutoLock && !m_bReloadInterrupted;
	bool bAltFire = g_pPlayerMgr->CanAltFireWeapon( ) | m_bAltFireRequested;
	bool bGrenade = g_pPlayerMgr->CanThrowGrenade( ) | m_bGrenadeRequested;]=] [=[	bool bFire = g_pPlayerMgr->CanFireWeapon( ) && !m_bSemiAutoLock && !m_bReloadInterrupted;
	bool bAltFire = g_pPlayerMgr->CanAltFireWeapon( ) | m_bAltFireRequested;
	bool bGrenade = g_pPlayerMgr->CanThrowGrenade( ) | m_bGrenadeRequested;
	if (bFire || bAltFire || bGrenade)
	{
		printf("FEARWEAPON: model-state fire=%d alt=%d grenade=%d handled=%d paused=%d\n",
			bFire ? 1 : 0, bAltFire ? 1 : 0, bGrenade ? 1 : 0,
			m_bFireHandled ? 1 : 0, m_bPaused ? 1 : 0);
		fflush(stdout);
	}]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[bool CClientWeapon::HandleFireKey( HLOCALOBJ hObj, ArgList* pArgList )
{]=] [=[bool CClientWeapon::HandleFireKey( HLOCALOBJ hObj, ArgList* pArgList )
{
	printf("FEARWEAPON: fire-key state=%d enabled=%d disabled=%d visible=%d\n",
		(int)GetState(),
		CPlayerBodyMgr::Instance().IsEnabled() ? 1 : 0,
		m_bDisabled ? 1 : 0,
		m_bVisible ? 1 : 0);
	fflush(stdout);]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
string(REPLACE [=[	if ( m_bFire )
	{
		// doesn't actually fire, just updates the ammo 
		eRet = UpdateAmmoFromFire( true );]=] [=[	if ( m_bFire )
	{
		printf("FEARWEAPON: consume-fire clip=%d reserve=%d\n",
			m_RightHandWeapon.m_nAmmoInClip,
			(int)g_pPlayerStats->GetAmmoCount( m_hAmmo ));
		fflush(stdout);
		// doesn't actually fire, just updates the ammo 
		eRet = UpdateAmmoFromFire( true );]=] CLIENTWEAPON_SOURCE_TEXT "${CLIENTWEAPON_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ClientWeapon.cpp" "${CLIENTWEAPON_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${CLIENTWEAPON_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ClientWeapon.cpp")

set(CLIENTWEAPONMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/ClientWeaponMgr.cpp")
file(READ "${CLIENTWEAPONMGR_ORIGINAL}" CLIENTWEAPONMGR_SOURCE_TEXT)
string(REPLACE [=[	if( bChange )
	{
		// Force a change to the appropriate weapon...]=] [=[	if( bChange )
	{
		if( g_pPlayerStats && g_pPlayerStats->HaveWeapon( hWeapon ) &&
			(!g_pPlayerMgr || !g_pPlayerMgr->GetMoveMgr() ||
			!g_pPlayerMgr->GetMoveMgr()->GetCharacterFX( ) ||
			!CanChangeToWeapon( hWeapon )) )
		{
			m_hRequestedWeapon = hWeapon;
			m_hRequestedAmmo = hDefaultAmmo;
			return;
		}

		// Force a change to the appropriate weapon...]=] CLIENTWEAPONMGR_SOURCE_TEXT "${CLIENTWEAPONMGR_SOURCE_TEXT}")
string(REPLACE [=[			ChangeWeapon( m_hRequestedWeapon, m_hRequestedAmmo, -1, sk_bPlaySelect, !sk_bPlayDeselect );

			m_hRequestedWeapon = NULL;
			m_hRequestedAmmo = NULL;]=] [=[			if( ChangeWeapon( m_hRequestedWeapon, m_hRequestedAmmo, -1, sk_bPlaySelect, !sk_bPlayDeselect ))
			{
				m_hRequestedWeapon = NULL;
				m_hRequestedAmmo = NULL;
			}]=] CLIENTWEAPONMGR_SOURCE_TEXT "${CLIENTWEAPONMGR_SOURCE_TEXT}")
string(REPLACE [=[	if( !hWeapon )
		return;
	]=] [=[	if( !hWeapon )
		return;

	printf("FEARWEAPON: change-message name='%s' force=%d select=%d deselect=%d ammo='%s' fx=%p alive=%d have=%d can=%d\n",
		g_pLTDatabase->GetRecordName( hWeapon ),
		bForce ? 1 : 0,
		bPlaySelect ? 1 : 0,
		bPlayDeselect ? 1 : 0,
		(g_pLTDatabase && hAmmo) ? g_pLTDatabase->GetRecordName( hAmmo ) : "",
		g_pPlayerMgr && g_pPlayerMgr->GetMoveMgr() ? (void *)g_pPlayerMgr->GetMoveMgr()->GetCharacterFX( ) : NULL,
		g_pPlayerMgr && g_pPlayerMgr->IsPlayerAlive() ? 1 : 0,
		g_pPlayerStats && g_pPlayerStats->HaveWeapon( hWeapon ) ? 1 : 0,
		CanChangeToWeapon( hWeapon ) ? 1 : 0);
	fflush(stdout);
	]=] CLIENTWEAPONMGR_SOURCE_TEXT "${CLIENTWEAPONMGR_SOURCE_TEXT}")
string(REPLACE [=[			m_nCurClientWeaponIndex = g_pWeaponDB->GetPlayerWeaponIndex( hWeapon );
			if( m_nCurClientWeaponIndex != CWM_NO_WEAPON )
			{
				m_pCurrentWeapon = m_apClientWeapon[ m_nCurClientWeaponIndex ];
				m_pCurrentWeapon->Activate();
				m_pCurrentWeapon->Select(!bPlaySelect);]=] [=[			m_nCurClientWeaponIndex = g_pWeaponDB->GetPlayerWeaponIndex( hWeapon );
			printf("FEARWEAPON: activate begin index=%u array=%p current=%p target=%p target_record=%p right=%p left=%p\n",
				m_nCurClientWeaponIndex,
				(void *)m_apClientWeapon,
				(void *)m_pCurrentWeapon,
				m_nCurClientWeaponIndex != CWM_NO_WEAPON ? (void *)m_apClientWeapon[m_nCurClientWeaponIndex] : NULL,
				m_nCurClientWeaponIndex != CWM_NO_WEAPON ? (void *)m_apClientWeapon[m_nCurClientWeaponIndex]->GetWeaponRecord() : NULL,
				m_nCurClientWeaponIndex != CWM_NO_WEAPON ? (void *)m_apClientWeapon[m_nCurClientWeaponIndex]->GetRightHandModel() : NULL,
				m_nCurClientWeaponIndex != CWM_NO_WEAPON ? (void *)m_apClientWeapon[m_nCurClientWeaponIndex]->GetLeftHandModel() : NULL);
			fflush(stdout);
			if( m_nCurClientWeaponIndex != CWM_NO_WEAPON )
			{
				m_pCurrentWeapon = m_apClientWeapon[ m_nCurClientWeaponIndex ];
				const bool bActivated = m_pCurrentWeapon->Activate();
				printf("FEARWEAPON: activate end ok=%d current=%p record=%p right=%p left=%p\n",
					bActivated ? 1 : 0,
					(void *)m_pCurrentWeapon,
					(void *)m_pCurrentWeapon->GetWeaponRecord(),
					(void *)m_pCurrentWeapon->GetRightHandModel(),
					(void *)m_pCurrentWeapon->GetLeftHandModel());
				fflush(stdout);
				m_pCurrentWeapon->Select(!bPlaySelect);]=] CLIENTWEAPONMGR_SOURCE_TEXT "${CLIENTWEAPONMGR_SOURCE_TEXT}")
string(REPLACE [=[				if( !m_bWeaponsEnabled )
					m_pCurrentWeapon->SetDisable( true );]=] [=[				m_pCurrentWeapon->SetDisable( !m_bWeaponsEnabled );
				printf("FEARWEAPON: disable-sync manager=%d weapon=%d\n",
					m_bWeaponsEnabled ? 1 : 0, !m_bWeaponsEnabled ? 1 : 0);
				fflush(stdout);]=] CLIENTWEAPONMGR_SOURCE_TEXT "${CLIENTWEAPONMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ClientWeaponMgr.cpp" "${CLIENTWEAPONMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${CLIENTWEAPONMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ClientWeaponMgr.cpp")

set(PLAYERMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/PlayerMgr.cpp")
file(READ "${PLAYERMGR_ORIGINAL}" PLAYERMGR_SOURCE_TEXT)
string(REPLACE [=[void CPlayerMgr::UpdatePlayerInfo(bool bPlaying, bool bForceSend)
{]=] [=[void CPlayerMgr::UpdatePlayerInfo(bool bPlaying, bool bForceSend)
{
	static int s_nPlayerInfoLogs = 0;
	if ((s_nPlayerInfoLogs++ % 120) == 1)
	{
		printf("FEARPORT: updateplayerinfo playing=%d force=%d inworld=%d\n", bPlaying?1:0, bForceSend?1:0, (m_pMoveMgr && m_pMoveMgr->IsInWorld())?1:0);
		fflush(stdout);
	}]=] PLAYERMGR_SOURCE_TEXT "${PLAYERMGR_SOURCE_TEXT}")
string(REPLACE [=[void CPlayerMgr::UpdateCamera()
{]=] [=[void CPlayerMgr::UpdateCamera()
{
	static int s_nCamCalls = 0;
	if ((++s_nCamCalls % 60) == 1) { printf("FEARPORT: updatecamera n=%d paused=%d charfx=%d allowcam=%d\n", s_nCamCalls, g_pGameClientShell->IsServerPaused()?1:0, m_pMoveMgr->GetCharacterFX()?1:0, g_pInterfaceMgr->AllowCameraMovement()?1:0); }]=] PLAYERMGR_SOURCE_TEXT "${PLAYERMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PlayerMgr.cpp" "${PLAYERMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${PLAYERMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/PlayerMgr.cpp")

set(SCREENDISPLAY_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/ScreenDisplay.cpp")
file(READ "${SCREENDISPLAY_ORIGINAL}" SCREENDISPLAY_SOURCE_TEXT)
string(REPLACE [=[void CScreenDisplay::Escape()
{]=] [=[void CScreenDisplay::Escape()
{
	printf("FEARPORT: ScreenDisplay::Escape\n");
	fflush(stdout);]=] SCREENDISPLAY_SOURCE_TEXT "${SCREENDISPLAY_SOURCE_TEXT}")
string(REPLACE [=[void CScreenDisplay::OnFocus(bool bFocus)
{]=] [=[void CScreenDisplay::OnFocus(bool bFocus)
{
	printf("FEARPORT: ScreenDisplay::OnFocus focus=%d escape=%d\n",
		bFocus ? 1 : 0, m_bEscape ? 1 : 0);
	fflush(stdout);]=] SCREENDISPLAY_SOURCE_TEXT "${SCREENDISPLAY_SOURCE_TEXT}")
string(REPLACE [=[			pProfile->ApplyDisplay();

			pProfile->Save();]=] [=[			printf("FEARPORT: ScreenDisplay ApplyDisplay %dx%dx%d\n",
				(int)pProfile->m_nScreenWidth, (int)pProfile->m_nScreenHeight,
				(int)pProfile->m_nScreenDepth);
			fflush(stdout);
			pProfile->ApplyDisplay();
			printf("FEARPORT: ScreenDisplay ApplyDisplay returned\n");
			fflush(stdout);

			pProfile->Save();]=] SCREENDISPLAY_SOURCE_TEXT "${SCREENDISPLAY_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ScreenDisplay.cpp" "${SCREENDISPLAY_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${SCREENDISPLAY_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ScreenDisplay.cpp")

set(PROFILEMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/ProfileMgr.cpp")
file(READ "${PROFILEMGR_ORIGINAL}" PROFILEMGR_SOURCE_TEXT)
string(REPLACE [=[void CUserProfile::ApplyDisplay()
{]=] [=[void CUserProfile::ApplyDisplay()
{
	printf("FEARPORT: ApplyDisplay target=%dx%dx%d renderer=%d\n",
		(int)m_nScreenWidth, (int)m_nScreenHeight, (int)m_nScreenDepth,
		g_pGameClientShell->IsRendererInitted() ? 1 : 0);
	fflush(stdout);
	const char *pszLockResolution = getenv("FEAR_LOCK_RESOLUTION");
	if (pszLockResolution && pszLockResolution[0] == '1' &&
		g_pGameClientShell->IsRendererInitted())
	{
		RMode lockedMode;
		if (g_pLTClient->GetRenderMode(&lockedMode) == LT_OK)
		{
			m_nScreenWidth = lockedMode.m_Width;
			m_nScreenHeight = lockedMode.m_Height;
			m_nScreenDepth = lockedMode.m_BitDepth;
			printf("FEARPORT: locked profile display to %ux%ux%u\n",
				m_nScreenWidth, m_nScreenHeight, m_nScreenDepth);
			fflush(stdout);
		}
	}]=] PROFILEMGR_SOURCE_TEXT "${PROFILEMGR_SOURCE_TEXT}")
string(REPLACE [=[		if (pMode && !IsRendererEqual(pMode,&currentMode))
		{
			//switch renderers]=] [=[		printf("FEARPORT: ApplyDisplay current=%ux%ux%u hw=%d mode=%p found=%d\n",
			currentMode.m_Width, currentMode.m_Height, currentMode.m_BitDepth,
			currentMode.m_bHWTnL ? 1 : 0, (void *)pMode,
			(pMode && !IsRendererEqual(pMode, &currentMode)) ? 1 : 0);
		fflush(stdout);
		if (pMode && !IsRendererEqual(pMode,&currentMode))
		{
			//switch renderers]=] PROFILEMGR_SOURCE_TEXT "${PROFILEMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ProfileMgr.cpp" "${PROFILEMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${PROFILEMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/ProfileMgr.cpp")

set(NODECONTROLLER_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/NodeController.cpp")
file(READ "${NODECONTROLLER_ORIGINAL}" NODECONTROLLER_SOURCE_TEXT)
string(REPLACE [=[void CNodeController::HandleNodeControlLipSyncMessage(ILTMessage_Read *pMsg)
{]=] [=[void CNodeController::HandleNodeControlLipSyncMessage(ILTMessage_Read *pMsg)
{
	printf("FEARDLG: client lip message charfx=%p\n", (void *)m_pCharacterFX);
	fflush(stdout);]=] NODECONTROLLER_SOURCE_TEXT "${NODECONTROLLER_SOURCE_TEXT}")
string(REPLACE [=[	if( bSendNotification && m_pCharacterFX )
	{]=] [=[	printf("FEARDLG: client lip kill notify=%d charfx=%p server=%p\n",
		bSendNotification ? 1 : 0, (void *)m_pCharacterFX,
		m_pCharacterFX ? (void *)m_pCharacterFX->GetServerObj() : NULL);
	fflush(stdout);
	if( bSendNotification && m_pCharacterFX )
	{]=] NODECONTROLLER_SOURCE_TEXT "${NODECONTROLLER_SOURCE_TEXT}")
string(REPLACE [=[		g_pLTClient->SendToServer( cMsg.Read(), MESSAGE_GUARANTEED );
	}]=] [=[		LTRESULT nSendResult = g_pLTClient->SendToServer( cMsg.Read(), MESSAGE_GUARANTEED );
		printf("FEARDLG: client lip completion send=%d\n", (int)nSendResult);
		fflush(stdout);
	}]=] NODECONTROLLER_SOURCE_TEXT "${NODECONTROLLER_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/NodeController.cpp" "${NODECONTROLLER_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${NODECONTROLLER_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/NodeController.cpp")

set(SFXMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/SFXMgr.cpp")
file(READ "${SFXMGR_ORIGINAL}" SFXMGR_SOURCE_TEXT)
string(REPLACE [=[#include "stdafx.h"
#include "SFXMgr.h"]=] [=[#include "stdafx.h"
#include "SFXMgr.h"
extern "C" int FearClient_EnsureDialogueCharacter(void *hObject);]=] SFXMGR_SOURCE_TEXT "${SFXMGR_SOURCE_TEXT}")
string(REPLACE [=[    uint8 nFXType   = pMsg->Readuint8();
	HOBJECT hObj	= pMsg->ReadObject();]=] [=[    uint8 nFXType   = pMsg->Readuint8();
	uint32 nObjectPos = pMsg->Tell();
	uint32 nRawObjectId = pMsg->ReadBits(16);
	pMsg->SeekTo(nObjectPos);
	HOBJECT hObj	= pMsg->ReadObject();
	printf("FEARDLG: sfx message type=%u rawid=%u object=%p\n",
		(unsigned)nFXType, (unsigned)nRawObjectId, (void *)hObj);
	fflush(stdout);]=] SFXMGR_SOURCE_TEXT "${SFXMGR_SOURCE_TEXT}")
string(REPLACE [=[		CSpecialFX* pFX = FindSpecialFX(nFXType, hObj);

		if (pFX)]=] [=[		CSpecialFX* pFX = FindSpecialFX(nFXType, hObj);
		if (!pFX && nFXType == SFX_CHARACTER_ID)
		{
			uint32 nCfxPos = pMsg->Tell();
			uint8 nCfxMsg = (uint8)pMsg->ReadBits(FNumBitsExclusive<CFX_COUNT>::k_nValue);
			pMsg->SeekTo(nCfxPos);
			printf("FEARDLG: missing character rawid=%u object=%p cfx=%u\n",
				(unsigned)nRawObjectId, (void *)hObj, (unsigned)nCfxMsg);
			fflush(stdout);
			if (nCfxMsg == CFX_NODECONTROL_LIP_SYNC || nCfxMsg == CFX_DIALOGUE_MSG)
			{
				int nCreated = FearClient_EnsureDialogueCharacter((void *)hObj);
				pFX = FindSpecialFX(nFXType, hObj);
				printf("FEARDLG: dialogue character ensure object=%p cfx=%u created=%d fx=%p\n",
					(void *)hObj, (unsigned)nCfxMsg, nCreated, (void *)pFX);
				fflush(stdout);
			}
		}
		printf("FEARDLG: sfx lookup type=%u object=%p fx=%p\n",
			(unsigned)nFXType, (void *)hObj, (void *)pFX);
		fflush(stdout);

		if (pFX)]=] SFXMGR_SOURCE_TEXT "${SFXMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/SFXMgr.cpp" "${SFXMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${SFXMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/SFXMgr.cpp")

set(CREDITS_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/Credits.cpp")
file(READ "${CREDITS_ORIGINAL}" CREDITS_SOURCE_TEXT)
string(REPLACE [=[	HATTRIBUTE hAtt = g_pLTDatabase->GetAttribute(m_hOrder,"Entries");
	if (m_nIndex >= g_pLTDatabase->GetNumValues(hAtt))]=] [=[	HATTRIBUTE hAtt = g_pLTDatabase->GetAttribute(m_hOrder,"Entries");
	if (!hAtt)
		return false;
	if (m_nIndex > 128)]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	m_hEntry = g_pLTDatabase->GetRecordLink(hAtt,m_nIndex,NULL);]=] [=[	HRECORD hResolvedEntry = g_pLTDatabase->GetRecordLink(hAtt,m_nIndex,NULL);
	if (!hResolvedEntry)
		return false;
	m_hEntry = hResolvedEntry;]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	m_hEntry = hResolvedEntry;]=] [=[	m_hEntry = hResolvedEntry;
	HCATEGORY hEntryCat = g_pLTDatabase->GetRecordParent(m_hEntry);
	printf("FEARCRED: entry index=%u record=%p cat='%s' name='%s'\n",
		(unsigned)m_nIndex, (void *)m_hEntry,
		hEntryCat ? g_pLTDatabase->GetCategoryName(hEntryCat) : "(null)",
		g_pLTDatabase->GetRecordName(m_hEntry));]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	HATTRIBUTE hLines = g_pLTDatabase->GetAttribute(m_hEntry,"Lines");
	m_nLines = g_pLTDatabase->GetNumValues(hLines);
	if (!m_nLines)]=] [=[	HATTRIBUTE hLines = g_pLTDatabase->GetAttribute(m_hEntry,"Lines");
	m_nLines = 0;
	for (uint32 nProbe = 0; nProbe < 128; ++nProbe)
	{
		HATTRIBUTE hProbe = DATABASE_CATEGORY( CreditEntry ).GetStructAttribute(hLines, nProbe, "Line");
		const char *pszProbe = hProbe ? g_pLTDatabase->GetString(hProbe, 0, "") : "";
		if (!pszProbe || !pszProbe[0])
			break;
		++m_nLines;
	}
	printf("FEARCRED: entry lines attr=%p count=%u\n", (void *)hLines, (unsigned)m_nLines);
	if (!m_nLines)]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[		HATTRIBUTE hLineLayoutAttrib = DATABASE_CATEGORY( CreditEntry ).GetStructAttribute( hLines, n, "LineLayout" );
		HRECORD hLineLayout = DATABASE_CATEGORY( CreditEntry ).GetRecordLinkToLocalizedDB( hLineLayoutAttrib,
			0, hLineLayoutCat );]=] [=[		HATTRIBUTE hLineLayoutAttrib = DATABASE_CATEGORY( CreditEntry ).GetStructAttribute( hLines, n, "LineLayout" );
		printf("FEARCRED: line=%u layoutAttr=%p\n", (unsigned)n, (void *)hLineLayoutAttrib);
		HRECORD hLineLayout = DATABASE_CATEGORY( CreditEntry ).GetRecordLinkToLocalizedDB( hLineLayoutAttrib,
			0, hLineLayoutCat );
		printf("FEARCRED: line=%u layout=%p\n", (unsigned)n, (void *)hLineLayout);]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[		HRECORD hFont = DATABASE_CATEGORY( CreditLineLayout ).GETRECORDATTRIB( hLineLayout, Font );
		HATTRIBUTE hFontAtt = g_pLTDatabase->GetAttribute(hFont,"Face");]=] [=[		HRECORD hFont = DATABASE_CATEGORY( CreditLineLayout ).GETRECORDATTRIB( hLineLayout, Font );
		HCATEGORY hFontCat = hFont ? g_pLTDatabase->GetRecordParent(hFont) : NULL;
		printf("FEARCRED: line=%u font=%p cat='%s' name='%s'\n", (unsigned)n, (void *)hFont,
			hFontCat ? g_pLTDatabase->GetCategoryName(hFontCat) : "(null)",
			hFont ? g_pLTDatabase->GetRecordName(hFont) : "(null)");
		HATTRIBUTE hFontAtt = g_pLTDatabase->GetAttribute(hFont,"Face");]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[void Credits::Start(HRECORD hOrder)
{]=] [=[void Credits::Start(HRECORD hOrder)
{
	static int s_nCreditsMode = -1;
	if (s_nCreditsMode < 0)
	{
		char szMode[8] = {0};
		s_nCreditsMode = (GetEnvironmentVariableA("FEAR_CREDITS", szMode, sizeof(szMode)) > 0 && szMode[0] && szMode[0] != '0') ? 1 : 0;
	}
	if (!s_nCreditsMode)
	{
		static int s_nSkipped = 0;
		if (++s_nSkipped <= 5)
		{
			printf("FEARPORT: Credits::Start skipped order=%p\n", (void *)hOrder);
			fflush(stdout);
		}
		return;
	}]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	static int s_nCreditsMode = -1;]=] [=[	static int s_nCreditsLogs = 0;
	if (hOrder && s_nCreditsLogs < 16)
	{
		HCATEGORY hParent = g_pLTDatabase ? g_pLTDatabase->GetRecordParent(hOrder) : NULL;
		const char *pszCategory = (g_pLTDatabase && hParent) ? g_pLTDatabase->GetCategoryName(hParent) : "(null)";
		const char *pszRecord = g_pLTDatabase ? g_pLTDatabase->GetRecordName(hOrder) : "(null)";
		printf("FEARCRED: order=%p cat='%s' name='%s'\n", (void *)hOrder,
			pszCategory ? pszCategory : "(null)", pszRecord ? pszRecord : "(null)");
		fflush(stdout);
		++s_nCreditsLogs;
	}

	static int s_nCreditsMode = -1;]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	HRECORD hRec = DATABASE_CATEGORY( CreditOrder ).GetRecordByName( szOrder );]=] [=[	printf("FEARPORT: CreditOrder::Start '%s'\n", szOrder ? szOrder : "(null)");
	fflush(stdout);
	HRECORD hRec = DATABASE_CATEGORY( CreditOrder ).GetRecordByName( szOrder );]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
string(REPLACE [=[	m_cTextColors.reserve(m_nLines);]=] [=[	m_cTextColors.resize(m_nLines);]=] CREDITS_SOURCE_TEXT "${CREDITS_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Credits.cpp" "${CREDITS_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${CREDITS_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/Credits.cpp")

set(CMOVEMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/CMoveMgr.cpp")
file(READ "${CMOVEMGR_ORIGINAL}" CMOVEMGR_SOURCE_TEXT)
string(REPLACE [=[	if (changeFlags & PSTATE_PHYSICS_MODEL)
	{
		PlayerPhysicsModel eModel = (PlayerPhysicsModel) pMsg->Readuint8();]=] [=[	if (changeFlags & PSTATE_PHYSICS_MODEL)
	{
		PlayerPhysicsModel eModel = (PlayerPhysicsModel) pMsg->Readuint8();
		printf("FEARPORT: physics update change=%04x model=%d\n", changeFlags, (int)eModel);
		fflush(stdout);]=] CMOVEMGR_SOURCE_TEXT "${CMOVEMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/CMoveMgr.cpp" "${CMOVEMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${CMOVEMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/CMoveMgr.cpp")

set(INTERFACEMGR_ORIGINAL "${FEAR_PUBLIC_TOOLS_SOURCE}/Game/ClientShellDLL/InterfaceMgr.cpp")
file(READ "${INTERFACEMGR_ORIGINAL}" INTERFACEMGR_SOURCE_TEXT)
string(REPLACE [=[#include "Credits.h"]=] [=[#include "Credits.h"
#include "CreditsDB.h"]=] INTERFACEMGR_SOURCE_TEXT "${INTERFACEMGR_SOURCE_TEXT}")
string(REPLACE [=[			HRECORD hText = DATABASE_CATEGORY( Objective ).GETRECORDATTRIB( hRec, TextDisplay );
				if (hText)]=] [=[			HATTRIBUTE hTextAtt = g_pLTDatabase->GetAttribute(hRec, "TextDisplay");
			EAttributeType eTextType = g_pLTDatabase->GetAttributeType(hTextAtt);
			const char *pszTextRecord = g_pLTDatabase->GetString(hTextAtt, 0, "");
			HRECORD hText = g_pLTDatabase->GetRecordLink(hTextAtt, 0, NULL);
			if (!hText && eTextType == eAttributeType_String && pszTextRecord[0])
				hText = DATABASE_CATEGORY( CreditOrder ).GetRecordByName(pszTextRecord);
			uint32 nTextValues = g_pLTDatabase->GetNumValues(hTextAtt);
			const char *pszObjectiveName = g_pLTDatabase->GetRecordName(hRec);
			HRECORD hByObjectiveName = DATABASE_CATEGORY( CreditOrder ).GetRecordByName(pszObjectiveName);
			if (!hText)
				hText = hByObjectiveName;
			printf("FEARCRED: objective record=%p name='%s' textAtt=%p type=%d values=%u order=%p byName=%p\n",
				(void *)hRec, pszObjectiveName ? pszObjectiveName : "(null)",
				(void *)hTextAtt, (int)eTextType, (unsigned)nTextValues,
				(void *)hText, (void *)hByObjectiveName);
			fflush(stdout);
			for (uint32 nValue = 0; nValue < nTextValues; ++nValue)
			{
				HRECORD hValue = g_pLTDatabase->GetRecordLink(hTextAtt, nValue, NULL);
				HCATEGORY hValueParent = hValue ? g_pLTDatabase->GetRecordParent(hValue) : NULL;
				const char *pszValueCat = hValueParent ? g_pLTDatabase->GetCategoryName(hValueParent) : "(null)";
				const char *pszValueName = hValue ? g_pLTDatabase->GetRecordName(hValue) : "(null)";
				printf("FEARCRED: text value=%u record=%p cat='%s' name='%s'\n",
					(unsigned)nValue, (void *)hValue, pszValueCat ? pszValueCat : "(null)",
					pszValueName ? pszValueName : "(null)");
			}
				if (hText)]=] INTERFACEMGR_SOURCE_TEXT "${INTERFACEMGR_SOURCE_TEXT}")
string(REPLACE [=[bool CInterfaceMgr::ChangeState(GameState eNewState, eScreenID screenID/*=SCREEN_ID_UNASSIGNED*/)
{]=] [=[bool CInterfaceMgr::ChangeState(GameState eNewState, eScreenID screenID/*=SCREEN_ID_UNASSIGNED*/)
{
	printf("FEARPORT: ChangeState %d -> %d screen=%d\n",
		(int)m_eGameState, (int)eNewState, (int)screenID);
	fflush(stdout);]=] INTERFACEMGR_SOURCE_TEXT "${INTERFACEMGR_SOURCE_TEXT}")
string(REPLACE [=[bool CInterfaceMgr::SwitchToScreen(eScreenID screenID)
{]=] [=[bool CInterfaceMgr::SwitchToScreen(eScreenID screenID)
{
	printf("FEARPORT: SwitchToScreen id=%d from=%d\n", (int)screenID, (int)m_eGameState);
	fflush(stdout);]=] INTERFACEMGR_SOURCE_TEXT "${INTERFACEMGR_SOURCE_TEXT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/InterfaceMgr.cpp" "${INTERFACEMGR_SOURCE_TEXT}")
list(REMOVE_ITEM CLIENT_SHELL_SOURCES "${INTERFACEMGR_ORIGINAL}")
list(APPEND CLIENT_SHELL_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/InterfaceMgr.cpp")

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
