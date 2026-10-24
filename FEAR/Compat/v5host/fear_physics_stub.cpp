#include "fear_physics_stub.h"
#include "fear_object_map.h"
#include "fear_host_api.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearPhysics g_FearPhysics;

namespace {

const int kMaxFearPhysicsObjects = 256;

struct FearObjectPhysics
{
    HOBJECT m_hObject;
    LTVector m_vVelocity;
    LTVector m_vAcceleration;
    float m_fMass;
    float m_fFriction;
    float m_fForceIgnoreLimit;
    bool m_bOnGround;
};

FearObjectPhysics g_ObjectPhysics[kMaxFearPhysicsObjects];
int g_nObjectPhysics = 0;
LTVector g_vGlobalForce(0.0f, 0.0f, 0.0f);
float g_fStairHeight = 0.0f;

FearObjectPhysics *FearFindObjectPhysics(HOBJECT hObject)
{
    if (!hObject)
        return NULL;

    for (int i = 0; i < g_nObjectPhysics; ++i)
    {
        if (g_ObjectPhysics[i].m_hObject == hObject)
            return &g_ObjectPhysics[i];
    }

    if (g_nObjectPhysics >= kMaxFearPhysicsObjects)
        return NULL;

    FearObjectPhysics *pState = &g_ObjectPhysics[g_nObjectPhysics++];
    memset(pState, 0, sizeof(*pState));
    pState->m_hObject = hObject;
    pState->m_fMass = 1.0f;
    pState->m_fFriction = 1.0f;
    pState->m_bOnGround = true;
    return pState;
}

}

LTRESULT CFearPhysics::UpdateMovement(MoveInfo *pInfo)
{
    if (!pInfo || !pInfo->m_hObject)
        return LT_INVALIDPARAMS;

    pInfo->m_Offset.Init();

    FearObjectPhysics *pState = FearFindObjectPhysics(pInfo->m_hObject);
    if (!pState)
        return LT_OK;

    float fDt = FearHost_GetFrameDelta();
    if (fDt < (1.0f / 1000.0f))
        fDt = 1.0f / 1000.0f;
    if (fDt > 0.1f)
        fDt = 0.1f;
    pInfo->m_dt = fDt;

    if (!_finite(pState->m_vAcceleration.x) || !_finite(pState->m_vAcceleration.y) || !_finite(pState->m_vAcceleration.z))
        pState->m_vAcceleration.Init();
    if (!_finite(pState->m_vVelocity.x) || !_finite(pState->m_vVelocity.y) || !_finite(pState->m_vVelocity.z))
        pState->m_vVelocity.Init();

    if (pState->m_bOnGround)
    {
        if (pState->m_vAcceleration.y < 0.0f)
            pState->m_vAcceleration.y = 0.0f;
        if (pState->m_vVelocity.y < 0.0f)
            pState->m_vVelocity.y = 0.0f;
    }

    pState->m_vVelocity += pState->m_vAcceleration * fDt;
    pState->m_vAcceleration.Init();

    if (!pState->m_bOnGround && _finite(g_vGlobalForce.y))
        pState->m_vVelocity.y += g_vGlobalForce.y * fDt;

    pInfo->m_Offset = pState->m_vVelocity * fDt;

    static int s_nMoveLogs = 0;
    if (++s_nMoveLogs <= 5 || (s_nMoveLogs % 240) == 0)
    {
        float vPosition[3] = {0.0f, 0.0f, 0.0f};
        FearClient_GetObjectTransform(pInfo->m_hObject, vPosition, NULL);
        printf("PHYS: move dt=%.4f pos=%.1f %.1f %.1f vel=%.1f %.1f %.1f off=%.2f %.2f %.2f grav=%.1f ground=%d\n",
            fDt, vPosition[0], vPosition[1], vPosition[2],
            pState->m_vVelocity.x, pState->m_vVelocity.y, pState->m_vVelocity.z,
            pInfo->m_Offset.x, pInfo->m_Offset.y, pInfo->m_Offset.z,
            g_vGlobalForce.y, pState->m_bOnGround ? 1 : 0);
        fflush(stdout);
    }

    return LT_OK;
}

LTRESULT CFearPhysics::MovePushObjects(HOBJECT hToMove, const LTVector& p, HOBJECT* hPushObjects, uint32 nPushObjects)
{
    return LT_OK;
}

LTRESULT CFearPhysics::RotatePushObjects(HOBJECT hToMove, const LTRotation& q, HOBJECT* hPushObjects, uint32 nPushObjects)
{
    return LT_OK;
}

LTRESULT CFearPhysics::IsWorldObject(HOBJECT hObj)
{
    return LT_NO;
}

LTRESULT CFearPhysics::GetStairHeight(float &fHeight)
{
    fHeight = g_fStairHeight;
    return LT_OK;
}

LTRESULT CFearPhysics::SetStairHeight(float fHeight)
{
    g_fStairHeight = fHeight;
    return LT_OK;
}

LTRESULT CFearPhysics::GetMass(HOBJECT hObj, float *m)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState || !m)
        return LT_INVALIDPARAMS;

    *m = pState->m_fMass;
    return LT_OK;
}

LTRESULT CFearPhysics::SetMass(HOBJECT hObj, float m)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState)
        return LT_INVALIDPARAMS;

    pState->m_fMass = m;
    return LT_OK;
}

LTRESULT CFearPhysics::GetFrictionCoefficient(HOBJECT hObj, float* u)
{
    printf("STUB ILTPhysics::GetFrictionCoefficient\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetFrictionCoefficient(HOBJECT hObj, float u)
{
    printf("STUB ILTPhysics::SetFrictionCoefficient\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetObjectDims(HOBJECT hObj, LTVector *d)
{
    float dims[3] = {0.0f, 0.0f, 0.0f};
    if (!d)
        return LT_INVALIDPARAMS;
    if (!FearClient_GetObjectDims(hObj, dims))
        return LT_NOTFOUND;

    d->Init(dims[0], dims[1], dims[2]);
    return LT_OK;
}

LTRESULT CFearPhysics::SetObjectDims(HOBJECT hObj, LTVector *d, uint32 flag)
{
    float dims[3] = {0.0f, 0.0f, 0.0f};
    if (!d)
        return LT_INVALIDPARAMS;

    dims[0] = d->x;
    dims[1] = d->y;
    dims[2] = d->z;
    if (!FearClient_SetObjectDims(hObj, dims))
        return LT_NOTFOUND;

    return LT_OK;
}

LTRESULT CFearPhysics::GetVelocity(HOBJECT hObj, LTVector *v)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState || !v)
        return LT_INVALIDPARAMS;

    *v = pState->m_vVelocity;
    return LT_OK;
}

LTRESULT CFearPhysics::SetVelocity(HOBJECT hObj, const LTVector& v)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState)
        return LT_INVALIDPARAMS;

    pState->m_vVelocity = v;
    return LT_OK;
}

LTRESULT CFearPhysics::GetForceIgnoreLimit(HOBJECT hObj, float &f)
{
    printf("STUB ILTPhysics::GetForceIgnoreLimit\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetForceIgnoreLimit(HOBJECT hObj, float f)
{
    printf("STUB ILTPhysics::SetForceIgnoreLimit\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetAcceleration(HOBJECT hObj, LTVector *a)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState || !a)
        return LT_INVALIDPARAMS;

    *a = pState->m_vAcceleration;
    return LT_OK;
}

LTRESULT CFearPhysics::SetAcceleration(HOBJECT hObj, const LTVector& a)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState)
        return LT_INVALIDPARAMS;

    pState->m_vAcceleration = a;
    return LT_OK;
}

LTRESULT CFearPhysics::MoveObject(HOBJECT hObj, const LTVector& p, uint32 flag)
{
    float vPosition[3] = {0.0f, 0.0f, 0.0f};
    vPosition[0] = p.x;
    vPosition[1] = p.y;
    vPosition[2] = p.z;

    if (!_finite(p.x) || !_finite(p.y) || !_finite(p.z))
    {
        static int s_nBadMoveLogs = 0;
        if (s_nBadMoveLogs < 12)
        {
            ++s_nBadMoveLogs;
            printf("FEARPORT: bad physics MoveObject obj=%p pos=%.1f %.1f %.1f\n",
                (void *)hObj, p.x, p.y, p.z);
            fflush(stdout);
        }
        return LT_INVALIDPARAMS;
    }

    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    static int s_nMoveCalls = 0;
    static int s_nMoveHits = 0;
    ++s_nMoveCalls;
    if (s_nMoveCalls <= 3 || (s_nMoveCalls % 60) == 0)
    {
        printf("FEARPHYS: move calls=%d hits=%d\n", s_nMoveCalls, s_nMoveHits);
        fflush(stdout);
    }
    const FearEngineApi *pApi = FearHost_GetEngineApi();
    const bool bTeleport = (flag & MOVEOBJECT_TELEPORT) != 0;
    if (bTeleport && s_nMoveCalls <= 3)
    {
        printf("FEARPHYS: teleport obj=%p dst=%.1f %.1f %.1f\n",
            (void *)hObj, vPosition[0], vPosition[1], vPosition[2]);
        fflush(stdout);
    }
    if (pApi && pApi->IntersectWorld && !bTeleport)
    {
        float vCurrent[3] = { 0.0f, 0.0f, 0.0f };
        float vRotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        bool bHaveTransform = FearClient_GetObjectTransform(hObj, vCurrent, vRotation);
        if (s_nMoveCalls <= 3)
        {
            printf("FEARPHYS: call=%d obj=%p have=%d cur=%.1f %.1f %.1f dst=%.1f %.1f %.1f\n",
                s_nMoveCalls, (void *)hObj, bHaveTransform ? 1 : 0,
                vCurrent[0], vCurrent[1], vCurrent[2], vPosition[0], vPosition[1], vPosition[2]);
            fflush(stdout);
        }
        if (bHaveTransform)
        {
            float vHit[3] = { 0.0f, 0.0f, 0.0f };
            float vNormal[3] = { 0.0f, 1.0f, 0.0f };
            float fDist = 0.0f;
            if (pApi->IntersectWorld(vCurrent, vPosition, vHit, vNormal, &fDist))
            {
                ++s_nMoveHits;
                if (s_nMoveHits <= 5 || (s_nMoveHits % 300) == 0)
                {
                    printf("FEARPHYS: clamp n=%d from=%.1f %.1f %.1f to=%.1f %.1f %.1f hit=%.1f %.1f %.1f\n",
                        s_nMoveHits, vCurrent[0], vCurrent[1], vCurrent[2],
                        vPosition[0], vPosition[1], vPosition[2], vHit[0], vHit[1], vHit[2]);
                    fflush(stdout);
                }
                const float fEpsilon = 1.0f;
                vPosition[0] = vHit[0] + vNormal[0] * fEpsilon;
                vPosition[1] = vHit[1] + vNormal[1] * fEpsilon;
                vPosition[2] = vHit[2] + vNormal[2] * fEpsilon;

                if (pState)
                {
                    float fDot = pState->m_vVelocity.x * vNormal[0] +
                                 pState->m_vVelocity.y * vNormal[1] +
                                 pState->m_vVelocity.z * vNormal[2];
                    if (fDot < 0.0f)
                    {
                        pState->m_vVelocity.x -= vNormal[0] * fDot;
                        pState->m_vVelocity.y -= vNormal[1] * fDot;
                        pState->m_vVelocity.z -= vNormal[2] * fDot;
                    }
                }
            }
        }
    }

    if (!FearClient_SetObjectPosition(hObj, vPosition))
        return LT_NOTFOUND;

    return LT_OK;
}

LTRESULT CFearPhysics::GetStandingOn(HOBJECT hObj, CollisionInfo *info)
{
    FearObjectPhysics *pState = FearFindObjectPhysics(hObj);
    if (!pState || !info)
        return LT_INVALIDPARAMS;

    memset(info, 0, sizeof(*info));
    info->m_hObject = FearClient_GetGroundObject();
    info->m_hPoly = INVALID_HPOLY;
    info->m_Plane.m_Normal.Init(0.0f, 1.0f, 0.0f);
    info->m_Plane.m_Dist = 0.0f;

    const FearEngineApi *pApi = FearHost_GetEngineApi();
    float vPos[3] = { 0.0f, 0.0f, 0.0f };
    float vRot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (pApi && pApi->IntersectWorld && FearClient_GetObjectTransform(hObj, vPos, vRot))
    {
        float vFrom[3] = { vPos[0], vPos[1], vPos[2] };
        float vTo[3] = { vPos[0], vPos[1] - 64.0f, vPos[2] };
        float vHit[3] = { 0.0f, 0.0f, 0.0f };
        float vNormal[3] = { 0.0f, 1.0f, 0.0f };
        float fDist = 0.0f;
        if (pApi->IntersectWorld(vFrom, vTo, vHit, vNormal, &fDist))
        {
            info->m_Plane.m_Normal.Init(vNormal[0], vNormal[1], vNormal[2]);
            info->m_Plane.m_Dist = -(vNormal[0] * vHit[0] + vNormal[1] * vHit[1] + vNormal[2] * vHit[2]);
            pState->m_bOnGround = (vNormal[1] > 0.5f) ? true : false;
        }
        else
        {
            pState->m_bOnGround = info->m_hObject != NULL;
        }
    }

    return LT_OK;
}

LTRESULT CFearPhysics::GetGlobalForce(LTVector &a)
{
    a = g_vGlobalForce;
    return LT_OK;
}

LTRESULT CFearPhysics::SetGlobalForce(const LTVector &a)
{
    g_vGlobalForce = a;
    return LT_OK;
}
