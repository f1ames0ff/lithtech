#ifndef __FEAR_CLIENT_FACADE_H__
#define __FEAR_CLIENT_FACADE_H__

#include "ltbasedefs.h"
#include "iltclient.h"

extern bool g_bFearConnected;

class CFearClient : public ILTClient
{
public:
    declare_interface(CFearClient)
    CFearClient();
    ~CFearClient();

    LTRESULT SendTo(ILTMessage_Read *pMsg, const char *pAddr, uint16 nPort) override;
    LTRESULT StartPing(const char *pAddr, uint16 nPort, uint32 *pPingID) override;
    LTRESULT GetPingStatus(uint32 nPingID, uint32 *pStatus, uint32 *pLatency) override;
    LTRESULT RemovePing(uint32 nPingID) override;
    HOBJECT GetMainWorldModel() override;
    LTRESULT GetObjectRotation(HLOCALOBJ hObj, LTRotation *pRotation) override;
    LTRESULT SetObjectRotation(HOBJECT hObj, const LTRotation& rRotation) override;
    LTRESULT GetObjectPos(HLOCALOBJ hObj, LTVector *pPos) override;
    LTRESULT SetObjectPos(HLOCALOBJ hObj, const LTVector& vPos) override;
    LTRESULT ForceCurrentObjectPos(HLOCALOBJ hObj) override;
    LTRESULT GetObjectTransform(HLOCALOBJ hObj, LTRigidTransform *pTransform) override;
    LTRESULT SetObjectTransform(HLOCALOBJ hObj, const LTRigidTransform& tTransform) override;
    LTRESULT GetObjectTransform(HLOCALOBJ hObj, LTTransform *pTransform) override;
    ILTResourceMgr* ResourceMgr() override;
    ILTTimer * Timer() override;
    ILTModel * GetModelLT() override;
    ILTSoundMgr * SoundMgr() override;
    ILTFileMgr * FileMgr() override;
    ILTCommon * Common() override;
    ILTPhysics * Physics() override;
    ILTPhysicsSim * PhysicsSim() override;
    ILTLoadingProgress * LoadingProgress() override;
    LTRESULT SetObjectTransform(HLOCALOBJ hObj, const LTTransform& tTransform) override;
    LTRESULT GetObjectScale(HOBJECT hObj, float* pScale) override;
    LTRESULT SetObjectScale(HOBJECT hObj, float fScale) override;
    LTRESULT GetObjectColor(HLOCALOBJ hObject, float *r, float *g, float *b, float *a) override;
    LTRESULT SetObjectColor(HLOCALOBJ hObject, float r, float g, float b, float a) override;
    LTRESULT GetObjectShadowLOD(HLOCALOBJ hObject, EEngineLOD& eShadowLOD) override;
    LTRESULT SetObjectShadowLOD(HLOCALOBJ hObject, EEngineLOD eShadowLOD) override;
    HMODELANIM GetAnimIndex(HOBJECT hObj, const char *pAnimName) override;
    HMODELANIM GetModelAnimation(HOBJECT hObj) override;
    void SetModelLooping(HOBJECT hObj, bool bLoop) override;
    bool GetModelLooping(HOBJECT hObj) override;
    uint32 GetModelPlaybackState(HOBJECT hObj) override;
    LTRESULT SetLightType(HOBJECT hLight, EEngineLightType eLightType) override;
    LTRESULT GetLightType(HOBJECT hLight, EEngineLightType& eLightType) override;
    LTRESULT SetLightRadius(HOBJECT hLight, float fRadius) override;
    LTRESULT GetLightRadius(HOBJECT hLight, float& fRadius) override;
    LTRESULT RegisterConsoleProgram(const char *pName, ConsoleProgramFn fn) override;
    LTRESULT UnregisterConsoleProgram(const char *pName) override;
    bool GetContainerCode(HOBJECT hObj, uint16 *pCode) override;
    uint32 GetObjectContainers(HOBJECT hObj, HOBJECT *pContainerList, uint32 maxListSize) override;
    uint32 GetContainedObjects(HOBJECT hContainer, HOBJECT *pObjectList, uint32 maxListSize) override;
    LTRESULT OpenSocket(uint32 *phSocket) override;
    LTRESULT SetLightIntensityScale(HOBJECT hLight, float fIntensity) override;
    LTRESULT GetLightIntensityScale(HOBJECT hLight, float& fIntensity) override;
    LTRESULT SetLightTexture(HOBJECT hLight, const char* pszTexture) override;
    LTRESULT GetLightTexture(HOBJECT hLight, char* pszTexture, uint32 nBufferLen) override;
    LTRESULT SetLightAttenuationTexture(HOBJECT hLight, const char* pszTexture) override;
    LTRESULT GetLightAttenuationTexture(HOBJECT hLight, char* pszTexture, uint32 nBufferLen) override;
    LTRESULT SetLightTranslucentColor(HOBJECT hLight, const LTVector& vTransColor) override;
    LTRESULT DetachObjectStanding(HLOCALOBJ hObject) override;
    bool IntersectSegment(const IntersectQuery& Query, IntersectInfo *pInfo) override;
    bool IntersectSegmentAgainst(const IntersectQuery& Query, IntersectInfo *pInfo, HOBJECT hObject) override;
    LTRESULT GetLightTranslucentColor(HOBJECT hLight, LTVector& vTransColor) override;
    LTRESULT SetLightSpecularColor(HOBJECT hLight, const LTVector& vSpecularColor) override;
    LTRESULT GetLightSpecularColor(HOBJECT hLight, LTVector& vSpecularColor) override;
    LTRESULT SetLightDirectionalDims(HOBJECT hLight, const LTVector& vDims) override;
    LTRESULT GetLightDirectionalDims(HOBJECT hLight, LTVector& vDims) override;
    LTRESULT SetLightSpotInfo(HOBJECT hLight, float fFovX, float fFovY, float fNearClip) override;
    LTRESULT GetLightSpotInfo(HOBJECT hLight, float& fFovX, float& fFovY, float& fNearClip) override;
    LTRESULT SetLightDetailSettings(HOBJECT hLight, EEngineLOD eLightEnable, EEngineLOD eWorldShadows, EEngineLOD eObjectShadows) override;
    LTRESULT GetLightDetailSettings(HOBJECT hLight, EEngineLOD& eLightEnable, EEngineLOD& eWorldShadows, EEngineLOD& eObjectShadows) override;
    uint32 GetPointContainers(const LTVector *pPoint, HOBJECT *pList, uint32 maxListSize) override;
    void CPrintNoArgs(const char *pMsg) override;
    void CPrint(const char *pMsg, ...) override;
    HCONSOLEVAR GetConsoleVariable(const char* pszVarName) override;
    LTRESULT RunConsoleCommand(const char* pszCommand) override;
    float GetConsoleVariableFloat(HCONSOLEVAR hVar) override;
    LTRESULT SetConsoleVariableFloat(const char* pszVarName, float fValue) override;
    const char* GetConsoleVariableString(HCONSOLEVAR hVar) override;
    LTRESULT SetConsoleVariableString(const char* pszVarName, const char* pszValue) override;
    LTDOUBLE GetTime() override;
    uint32 GetTimeMS() override;
    LTRESULT GetObjectUnguaranteedData(HLOCALOBJ hObj, uint32 *pData) override;
    LTFLOAT GetFrameTime() override;
    LTRESULT GetSourceWorldOffset(LTVector& vVector) override;
    LTRESULT RemoveObject(HOBJECT hObj) override;
    LTRESULT GetSectorID(const char *pName, uint32 *pResult) const override;
    LTRESULT LinkObjRef(HOBJECT hObj, LTObjRef *pRef) override;
    LTRESULT GetBlindObjectData(uint32 nNum, uint32 nId, uint8*& pData, uint32& nSize) override;
    LTRESULT GetSkyCameraTransform(LTRigidTransform& tSkyCamera) override;
    LTRESULT OpenStreamFromBuffer(uint8* pData, const uint32 nDataSize, bool bCopyData, ILTInStream** ppStream) override;
    LTRESULT GetCompressedBufferMaxSize(const uint32 nSourceBufferSize, uint32& nCompressedBufferMaxSize) override;
    LTRESULT CompressBuffer(const uint8* pSourceData, const uint32 nDataLen, const uint8* pCompressedBuffer, const uint32 nCompressedBufferSize, uint32& nCompressedSize) override;
    LTRESULT DecompressBuffer(const uint8* pSourceData, const uint32 nDataLen, const uint8* pDecompressedBuffer, const uint32 nDeompressedBufferSize, uint32& nDecompressedSize) override;
    LTRESULT GetWorldCRC(uint32& nWorldCRC) override;
    LTRESULT GetSoundOffset(HLTSOUND hSound, uint32 * dwOffset, uint32 * dwSize) override;
    ILTVideoTexture * GetVideoTexture() override;
    ILTTextureMgr * GetTextureMgr() override;
    ILTRenderer * GetRenderer() override;
    ILTCustomRender * GetCustomRender() override;
    ILTTextureString * GetTextureString() override;
    ILTGraphMgr* GetGraphMgr() override;
    LTRESULT FindObjectsInBox(const LTVector& vCenter, const LTVector& vHalfDims, TQueryRegionObjectsCB pfnQueryCB, void* pUserData) override;
    LTRESULT FindPolysInSphere(HOBJECT hWorldModel, const LTVector& vSphereCenter, float fSphereRadius, uint32 nNumClipPlanes, const LTPlane* pClipPlanes, TQueryRegionPolysCB pfnPolyCB, void* pCBUser) override;
    ILTDrawPrim * GetDrawPrim() override;
    ILTCursor * Cursor() override;
    LTRESULT RenderConsoleToRenderTarget() override;
    LTRESULT GetServerIPAddress(uint8 pAddr[4], uint16 *pPort) override;
    LTRESULT ProcessAttachments(HOBJECT hObj) override;
    LTRESULT GetSectorVisibility(uint32 nID, bool *pSectorEnabled, bool *pPortalsEnabled) override;
    LTRESULT SetSectorVisibility(uint32 nID, bool bPortalsEnabled) override;
    LTRESULT EnableRegionSectors(HSTREAMINGREGION hRegion, bool bEnable) override;
    LTRESULT GetUDPSocket(SOCKET& socket) override;
    LTRESULT GetSConValueFloat(const char *pName, float &val) override;
    LTRESULT GetSConValueString(const char *pName, char *valBuf, uint32 bufLen) override;
    LTDOUBLE GetServerRealTime() override;
    LTRESULT SendToServer(ILTMessage_Read *pMsg, uint32 flags) override;
    LTRESULT StartQuery(const char *pInfo) override;
    LTRESULT UpdateQuery() override;
    LTRESULT GetQueryResults(NetSession* &pListHead) override;
    LTRESULT EndQuery() override;
    LTRESULT IsLocalToServer(bool *bLocal) override;
    LTRESULT GetHostUDPSocket(SOCKET& socket) override;
    LTRESULT GetEngineHook(char *pName, void **pData) override;
    LTRESULT QueryPerformanceStats(SPerformanceStats *pStats, bool bQuick) override;
    LTRESULT GetObjectGameData(HOBJECT hObj, void **pData) override;
    LTRESULT SetObjectGameData(HOBJECT hObj, void *pData) override;
    bool IsMultiplayerExe() override;
};

#endif
