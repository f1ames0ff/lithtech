#ifndef __FEAR_PHYSICSSIM_STUB_H__
#define __FEAR_PHYSICSSIM_STUB_H__

#include "ltbasedefs.h"
#include "iltphysicssim.h"

class CFearPhysicsSim : public ILTPhysicsSim
{
public:
    declare_interface(CFearPhysicsSim)
    CFearPhysicsSim() = default;
    ~CFearPhysicsSim() override = default;

    HPHYSICSSHAPE GetWorldModelShape(const char* pszWorldModel, LTRigidTransform& tTransform) override;
    HPHYSICSSHAPE CreateSphereShape(float fRadius, float fMassKg, float fDensityG) override;
    HPHYSICSSHAPE CreateBoxShape(const LTVector& vHalfDims, float fMassKg, float fDensityG, float fCollisionRadius) override;
    HPHYSICSSHAPE CreateCapsuleShape(const LTVector& vEndPt1, const LTVector& vEndPt2, float fRadius, float fMassKg, float fDensityG) override;
    HPHYSICSSHAPE CreateTransformShape(const LTRigidTransform& tTransform, HPHYSICSSHAPE hChildShape) override;
    HPHYSICSSHAPE CreateListShape(uint32 nNumShapes, HPHYSICSSHAPE* pChildren) override;
    HPHYSICSSHAPE CreateBoundingVolumeShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSSHAPE hChild) override;
    HPHYSICSSHAPE CreateContainerShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSCONTAINER hContainer) override;
    LTRESULT AddShapeRef(HPHYSICSSHAPE hShape) override;
    LTRESULT ReleaseShape(HPHYSICSSHAPE hShape) override;
    LTRESULT GetShapeMass(HPHYSICSSHAPE hShape, float& fMassKg) override;
    LTRESULT GetShapeVolume(HPHYSICSSHAPE hShape, float& fVolume) override;
    LTRESULT TraverseShapeHeirarchy(HPHYSICSSHAPE hShape, IShapeTraversalCallback* pICallback, const LTRigidTransform& tInitialTransform) override;
    HPHYSICSRIGIDBODY CreateRigidBody(HPHYSICSSHAPE hShape, const LTRigidTransform& tStartAt, bool bPinned, EPhysicsGroup eGroup, uint32 nSystem, float fFriction, float fCOR) override;
    LTRESULT AddRigidBodyRef(HPHYSICSRIGIDBODY hRigidBody) override;
    LTRESULT ReleaseRigidBody(HPHYSICSRIGIDBODY hRigidBody) override;
    LTRESULT GetRigidBodyShape(HPHYSICSRIGIDBODY hRigidBody, HPHYSICSSHAPE& hShape) override;
    LTRESULT PinRigidBody(HPHYSICSRIGIDBODY hRigidBody, bool bPin) override;
    LTRESULT IsRigidBodyPinned(HPHYSICSRIGIDBODY hRigidBody, bool& bPinned) override;
    LTRESULT EnableRigidBodyPinnedCollisions(HPHYSICSRIGIDBODY hRigidBody, bool bEnable) override;
    LTRESULT TeleportRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTeleportTo) override;
    LTRESULT GetRigidBodyTransform(HPHYSICSRIGIDBODY hRigidBody, LTRigidTransform& tTransform) override;
    LTRESULT GetRigidBodyCenterOfMassInWorld(HPHYSICSRIGIDBODY hRigidBody, LTVector& vCOM) override;
    LTRESULT GetRigidBodyInertiaWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaWorld) override;
    LTRESULT GetRigidBodyInertiaInvWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaInvWorld) override;
    LTRESULT GetRigidBodyObject(HPHYSICSRIGIDBODY hRigidBody, HOBJECT& hObject) override;
    LTRESULT SetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup, uint32 nSystem) override;
    LTRESULT GetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup& eGroup, uint32& nSystem) override;
    LTRESULT SetRigidBodyCollisionGroup(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup) override;
    LTRESULT SetRigidBodyCollisionSystem(HPHYSICSRIGIDBODY hRigidBody, uint32 nSystem) override;
    LTRESULT GetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool& bSolid) override;
    LTRESULT SetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool bSolid) override;
    LTRESULT SetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float fFriction) override;
    LTRESULT GetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float& fFriction) override;
    LTRESULT SetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float fCOR) override;
    LTRESULT GetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float& fCOR) override;
    LTRESULT SetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vVel) override;
    LTRESULT GetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vVel) override;
    LTRESULT SetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngVel) override;
    LTRESULT GetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vAngVel) override;
    LTRESULT ApplyRigidBodyForceWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSForce) override;
    LTRESULT ApplyRigidBodyImpulseWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSImpulse) override;
    LTRESULT ApplyRigidBodyTorque(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vTorqueNm) override;
    LTRESULT ApplyRigidBodyAngularImpulse(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngImpulse) override;
    LTRESULT IsRigidBodyActive(HPHYSICSRIGIDBODY hRigidBody, bool& bActive) override;
    HPHYSICSCOLLISIONNOTIFIER RegisterCollisionNotifier(HPHYSICSRIGIDBODY hRigidBody, TFnRigidBodyCollisionCB pfnCallback, void* pUserData) override;
    LTRESULT ReleaseCollisionNotifier(HPHYSICSCOLLISIONNOTIFIER hNotifier) override;
    LTRESULT RemoveRigidBodyFromSimulation(HPHYSICSRIGIDBODY hRigidBody) override;
    LTRESULT KeyframeRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTrans, float fTimeInterval) override;
    LTRESULT GetClosestPoint(HPHYSICSRIGIDBODY hRigidBody1, HPHYSICSRIGIDBODY hRigidBody2, LTVector& vClosestPt, float& fClosestDist) override;
    HPHYSICSCONSTRAINT CreateBallAndSocket(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2) override;
    HPHYSICSCONSTRAINT CreateStiffSpring(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, float fDistance) override;
    HPHYSICSCONSTRAINT CreateHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2) override;
    HPHYSICSCONSTRAINT CreateLimitedHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2, const LTVector& vPivotPerp1, const LTVector& vPivotPerp2, float fAngleMin, float fAngleMax, float fFriction) override;
    HPHYSICSCONSTRAINT CreateRagdoll(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vTwist1, const LTVector& vTwist2, const LTVector& vPerp1, const LTVector& vPerp2, float fConeAngleRad, float fPosRestrictAngleRad, float fNegRestrictAngleRad, float fTwistMinRad, float fTwistMaxRad, float fFriction) override;
    HPHYSICSCONSTRAINT CreateWheel(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vRotation1, const LTVector& vRotation2, const LTVector& vSuspension2, float fSuspensionMin, float fSuspensionMax, float fSuspensionStrength, float fSuspensionDamping) override;
    HPHYSICSCONSTRAINT CreatePrismatic(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vAxis1, const LTVector& vAxis2, const LTVector& vAxisPerp1, const LTVector& vAxisPerp2, float fMovementMin, float fMovementMax, float fFriction) override;
    LTRESULT AddConstraintRef(HPHYSICSCONSTRAINT hConstraint) override;
    LTRESULT ReleaseConstraint(HPHYSICSCONSTRAINT hConstraint) override;
    LTRESULT AddConstraintToSimulation(HPHYSICSCONSTRAINT hConstraint) override;
    LTRESULT RemoveConstraintFromSimulation(HPHYSICSCONSTRAINT hConstraint) override;
    HPHYSICSBREAKABLE CreateBreakable(HPHYSICSCONSTRAINT hConstraint, float fBreakableForce) override;
    LTRESULT AddBreakableRef(HPHYSICSBREAKABLE hBreakable) override;
    LTRESULT ReleaseBreakable(HPHYSICSBREAKABLE hBreakable) override;
    LTRESULT IsBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool& bBroken) override;
    LTRESULT SetBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool bBroken) override;
    LTRESULT RegisterBreakableCallback(HPHYSICSBREAKABLE hBreakable, TFnBreakableBrokenCB pfnCallback, void* pUser) override;
    LTRESULT AddBreakableToSimulation(HPHYSICSBREAKABLE hBreakable) override;
    LTRESULT RemoveBreakableFromSimulation(HPHYSICSBREAKABLE hBreakable) override;
    HPHYSICSCONTAINER CreateContainer() override;
    LTRESULT AddContainerRef(HPHYSICSCONTAINER hContainer) override;
    LTRESULT ReleaseContainer(HPHYSICSCONTAINER hContainer) override;
    LTRESULT GetNumRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, uint32& nNumRigidBodies) override;
    LTRESULT GetRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, HPHYSICSRIGIDBODY* pList, uint32 nListSize, uint32& nNumFilledInObjects) override;
    HPHYSICSACTION CreateAction(HPHYSICSRIGIDBODY hBody) override;
    LTRESULT AddActionRef(HPHYSICSACTION hAction) override;
    LTRESULT ReleaseAction(HPHYSICSACTION hAction) override;
    LTRESULT SetActionCallback(HPHYSICSACTION hAction, TFNPhysicsActionCB pfnCallback, void* pUserData) override;
    LTRESULT EnableCollisions(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool bEnable) override;
    LTRESULT EnableAllCollisions(EPhysicsGroup eGroup, bool bEnable) override;
    LTRESULT IsCollisionEnabled(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool& bEnabled) override;
    LTRESULT ApplyCollisionGroupChanges() override;
    LTRESULT GetUniqueSystemGroup(uint32& nID) override;
    LTRESULT SetGlobalCollisionListener(TFnRigidBodyCollisionCB pfnCallback, void* pUserData) override;
    LTRESULT StepSimulationForward(float fElapsedTime, float fTargetUpdateRate, uint32 nMaxSteps) override;
    LTRESULT GetWorldModelRigidBody(HOBJECT hWorldModel, HPHYSICSRIGIDBODY& hRigidBody) override;
    LTRESULT GetNumModelRigidBodies(HOBJECT hModel, uint32& nNumRigidBodies) override;
    LTRESULT GetModelRigidBody(HOBJECT hModel, uint32 nRigidBody, HPHYSICSRIGIDBODY& hRigidBody) override;
    LTRESULT GetModelNodeRigidBody(HOBJECT hModel, HMODELNODE hNode, HPHYSICSRIGIDBODY& hRigidBody) override;
    LTRESULT SetObjectPhysicsGroup(HOBJECT hObject, EPhysicsGroup eGroup) override;
    CPhysicsSimulation* GetSimulation() override;
};

extern CFearPhysicsSim g_FearPhysicsSim;

#endif
