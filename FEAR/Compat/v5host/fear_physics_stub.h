#ifndef __FEAR_PHYSICS_STUB_H__
#define __FEAR_PHYSICS_STUB_H__

#include "ltbasedefs.h"
#include "iltphysics.h"

class CFearPhysics : public ILTPhysics
{
public:
    declare_interface(CFearPhysics)
    CFearPhysics() = default;
    ~CFearPhysics() {}

    LTRESULT IsWorldObject(HOBJECT hObj) override;
    LTRESULT GetStairHeight(float &fHeight) override;
    LTRESULT SetStairHeight(float fHeight) override;
    LTRESULT GetMass(HOBJECT hObj, float *m) override;
    LTRESULT SetMass(HOBJECT hObj, float m) override;
    LTRESULT GetFrictionCoefficient(HOBJECT hObj, float* u) override;
    LTRESULT SetFrictionCoefficient(HOBJECT hObj, float u) override;
    LTRESULT GetObjectDims(HOBJECT hObj, LTVector *d) override;
    LTRESULT SetObjectDims(HOBJECT hObj, LTVector *d, uint32 flag) override;
    LTRESULT GetVelocity(HOBJECT hObj, LTVector *v) override;
    LTRESULT SetVelocity(HOBJECT hObj, const LTVector& v) override;
    LTRESULT GetForceIgnoreLimit(HOBJECT hObj, float &f) override;
    LTRESULT SetForceIgnoreLimit(HOBJECT hObj, float f) override;
    LTRESULT GetAcceleration(HOBJECT hObj, LTVector *a) override;
    LTRESULT SetAcceleration(HOBJECT hObj, const LTVector& a) override;
    LTRESULT MoveObject(HOBJECT hObj, const LTVector& p, uint32 flag) override;
    LTRESULT GetStandingOn(HOBJECT hObj, CollisionInfo *info) override;
    LTRESULT GetGlobalForce(LTVector &a) override;
    LTRESULT SetGlobalForce(const LTVector &a) override;
};

extern CFearPhysics g_FearPhysics;

#endif
