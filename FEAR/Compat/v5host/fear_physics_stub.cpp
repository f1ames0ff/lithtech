#include "fear_physics_stub.h"
#include <stdio.h>

template <typename T> struct SvcDefault { static T Get() { return T(); } };
template <> struct SvcDefault<LTRESULT> { static LTRESULT Get() { return LT_UNSUPPORTED; } };
template <> struct SvcDefault<const char*> { static const char* Get() { return ""; } };

CFearPhysics g_FearPhysics;

LTRESULT CFearPhysics::IsWorldObject(HOBJECT hObj)
{
    printf("STUB ILTPhysics::IsWorldObject\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetStairHeight(float &fHeight)
{
    printf("STUB ILTPhysics::GetStairHeight\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetStairHeight(float fHeight)
{
    printf("STUB ILTPhysics::SetStairHeight\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetMass(HOBJECT hObj, float *m)
{
    printf("STUB ILTPhysics::GetMass\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetMass(HOBJECT hObj, float m)
{
    printf("STUB ILTPhysics::SetMass\n");
    return SvcDefault<LTRESULT>::Get();
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
    printf("STUB ILTPhysics::GetObjectDims\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetObjectDims(HOBJECT hObj, LTVector *d, uint32 flag)
{
    printf("STUB ILTPhysics::SetObjectDims\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetVelocity(HOBJECT hObj, LTVector *v)
{
    printf("STUB ILTPhysics::GetVelocity\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetVelocity(HOBJECT hObj, const LTVector& v)
{
    printf("STUB ILTPhysics::SetVelocity\n");
    return SvcDefault<LTRESULT>::Get();
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
    printf("STUB ILTPhysics::GetAcceleration\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetAcceleration(HOBJECT hObj, const LTVector& a)
{
    printf("STUB ILTPhysics::SetAcceleration\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::MoveObject(HOBJECT hObj, const LTVector& p, uint32 flag)
{
    printf("STUB ILTPhysics::MoveObject\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetStandingOn(HOBJECT hObj, CollisionInfo *info)
{
    printf("STUB ILTPhysics::GetStandingOn\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::GetGlobalForce(LTVector &a)
{
    printf("STUB ILTPhysics::GetGlobalForce\n");
    return SvcDefault<LTRESULT>::Get();
}

LTRESULT CFearPhysics::SetGlobalForce(const LTVector &a)
{
    printf("STUB ILTPhysics::SetGlobalForce\n");
    return SvcDefault<LTRESULT>::Get();
}
