#include "fear_physicssim_stub.h"

ILTPhysicsSim::~ILTPhysicsSim() {}

HPHYSICSSHAPE ILTPhysicsSim::GetWorldModelShape(const char* pszWorldModel, LTRigidTransform& tTransform) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateSphereShape(float fRadius, float fMassKg, float fDensityG) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateBoxShape(const LTVector& vHalfDims, float fMassKg, float fDensityG, float fCollisionRadius) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateCapsuleShape(const LTVector& vEndPt1, const LTVector& vEndPt2, float fRadius, float fMassKg, float fDensityG) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateTransformShape(const LTRigidTransform& tTransform, HPHYSICSSHAPE hChildShape) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateListShape(uint32 nNumShapes, HPHYSICSSHAPE* pChildren) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateBoundingVolumeShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSSHAPE hChild) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE ILTPhysicsSim::CreateContainerShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSCONTAINER hContainer) { return HPHYSICSSHAPE(); }

LTRESULT ILTPhysicsSim::AddShapeRef(HPHYSICSSHAPE hShape) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseShape(HPHYSICSSHAPE hShape) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetShapeMass(HPHYSICSSHAPE hShape, float& fMassKg) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetShapeVolume(HPHYSICSSHAPE hShape, float& fVolume) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::TraverseShapeHeirarchy(HPHYSICSSHAPE hShape, IShapeTraversalCallback* pICallback, const LTRigidTransform& tInitialTransform) { return LT_UNSUPPORTED; }

HPHYSICSRIGIDBODY ILTPhysicsSim::CreateRigidBody(HPHYSICSSHAPE hShape, const LTRigidTransform& tStartAt, bool bPinned, EPhysicsGroup eGroup, uint32 nSystem, float fFriction, float fCOR) { return HPHYSICSRIGIDBODY(); }

LTRESULT ILTPhysicsSim::AddRigidBodyRef(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseRigidBody(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyShape(HPHYSICSRIGIDBODY hRigidBody, HPHYSICSSHAPE& hShape) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::PinRigidBody(HPHYSICSRIGIDBODY hRigidBody, bool bPin) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::IsRigidBodyPinned(HPHYSICSRIGIDBODY hRigidBody, bool& bPinned) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::EnableRigidBodyPinnedCollisions(HPHYSICSRIGIDBODY hRigidBody, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::TeleportRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTeleportTo) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyTransform(HPHYSICSRIGIDBODY hRigidBody, LTRigidTransform& tTransform) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyCenterOfMassInWorld(HPHYSICSRIGIDBODY hRigidBody, LTVector& vCOM) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyInertiaWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaWorld) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyInertiaInvWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaInvWorld) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyObject(HPHYSICSRIGIDBODY hRigidBody, HOBJECT& hObject) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup, uint32 nSystem) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup& eGroup, uint32& nSystem) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyCollisionGroup(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyCollisionSystem(HPHYSICSRIGIDBODY hRigidBody, uint32 nSystem) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool& bSolid) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool bSolid) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float fFriction) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float& fFriction) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float fCOR) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float& fCOR) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vVel) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vVel) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngVel) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vAngVel) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ApplyRigidBodyForceWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSForce) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ApplyRigidBodyImpulseWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSImpulse) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ApplyRigidBodyTorque(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vTorqueNm) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ApplyRigidBodyAngularImpulse(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngImpulse) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::IsRigidBodyActive(HPHYSICSRIGIDBODY hRigidBody, bool& bActive) { return LT_UNSUPPORTED; }

HPHYSICSCOLLISIONNOTIFIER ILTPhysicsSim::RegisterCollisionNotifier(HPHYSICSRIGIDBODY hRigidBody, TFnRigidBodyCollisionCB pfnCallback, void* pUserData) { return HPHYSICSCOLLISIONNOTIFIER(); }

LTRESULT ILTPhysicsSim::ReleaseCollisionNotifier(HPHYSICSCOLLISIONNOTIFIER hNotifier) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::RemoveRigidBodyFromSimulation(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::KeyframeRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTrans, float fTimeInterval) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetClosestPoint(HPHYSICSRIGIDBODY hRigidBody1, HPHYSICSRIGIDBODY hRigidBody2, LTVector& vClosestPt, float& fClosestDist) { return LT_UNSUPPORTED; }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateBallAndSocket(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateStiffSpring(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, float fDistance) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateLimitedHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2, const LTVector& vPivotPerp1, const LTVector& vPivotPerp2, float fAngleMin, float fAngleMax, float fFriction) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateRagdoll(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vTwist1, const LTVector& vTwist2, const LTVector& vPerp1, const LTVector& vPerp2, float fConeAngleRad, float fPosRestrictAngleRad, float fNegRestrictAngleRad, float fTwistMinRad, float fTwistMaxRad, float fFriction) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreateWheel(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vRotation1, const LTVector& vRotation2, const LTVector& vSuspension2, float fSuspensionMin, float fSuspensionMax, float fSuspensionStrength, float fSuspensionDamping) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT ILTPhysicsSim::CreatePrismatic(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vAxis1, const LTVector& vAxis2, const LTVector& vAxisPerp1, const LTVector& vAxisPerp2, float fMovementMin, float fMovementMax, float fFriction) { return HPHYSICSCONSTRAINT(); }

LTRESULT ILTPhysicsSim::AddConstraintRef(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseConstraint(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::AddConstraintToSimulation(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::RemoveConstraintFromSimulation(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

HPHYSICSBREAKABLE ILTPhysicsSim::CreateBreakable(HPHYSICSCONSTRAINT hConstraint, float fBreakableForce) { return HPHYSICSBREAKABLE(); }

LTRESULT ILTPhysicsSim::AddBreakableRef(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseBreakable(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::IsBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool& bBroken) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool bBroken) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::RegisterBreakableCallback(HPHYSICSBREAKABLE hBreakable, TFnBreakableBrokenCB pfnCallback, void* pUser) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::AddBreakableToSimulation(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::RemoveBreakableFromSimulation(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

HPHYSICSCONTAINER ILTPhysicsSim::CreateContainer() { return HPHYSICSCONTAINER(); }

LTRESULT ILTPhysicsSim::AddContainerRef(HPHYSICSCONTAINER hContainer) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseContainer(HPHYSICSCONTAINER hContainer) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetNumRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, uint32& nNumRigidBodies) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, HPHYSICSRIGIDBODY* pList, uint32 nListSize, uint32& nNumFilledInObjects) { return LT_UNSUPPORTED; }

HPHYSICSACTION ILTPhysicsSim::CreateAction(HPHYSICSRIGIDBODY hBody) { return HPHYSICSACTION(); }

LTRESULT ILTPhysicsSim::AddActionRef(HPHYSICSACTION hAction) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ReleaseAction(HPHYSICSACTION hAction) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetActionCallback(HPHYSICSACTION hAction, TFNPhysicsActionCB pfnCallback, void* pUserData) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::EnableCollisions(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::EnableAllCollisions(EPhysicsGroup eGroup, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::IsCollisionEnabled(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool& bEnabled) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::ApplyCollisionGroupChanges() { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetUniqueSystemGroup(uint32& nID) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetGlobalCollisionListener(TFnRigidBodyCollisionCB pfnCallback, void* pUserData) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::StepSimulationForward(float fElapsedTime, float fTargetUpdateRate, uint32 nMaxSteps) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetWorldModelRigidBody(HOBJECT hWorldModel, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetNumModelRigidBodies(HOBJECT hModel, uint32& nNumRigidBodies) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetModelRigidBody(HOBJECT hModel, uint32 nRigidBody, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::GetModelNodeRigidBody(HOBJECT hModel, HMODELNODE hNode, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT ILTPhysicsSim::SetObjectPhysicsGroup(HOBJECT hObject, EPhysicsGroup eGroup) { return LT_UNSUPPORTED; }

CPhysicsSimulation* ILTPhysicsSim::GetSimulation() { return NULL; }

CFearPhysicsSim g_FearPhysicsSim;

HPHYSICSSHAPE CFearPhysicsSim::GetWorldModelShape(const char* pszWorldModel, LTRigidTransform& tTransform) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateSphereShape(float fRadius, float fMassKg, float fDensityG) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateBoxShape(const LTVector& vHalfDims, float fMassKg, float fDensityG, float fCollisionRadius) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateCapsuleShape(const LTVector& vEndPt1, const LTVector& vEndPt2, float fRadius, float fMassKg, float fDensityG) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateTransformShape(const LTRigidTransform& tTransform, HPHYSICSSHAPE hChildShape) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateListShape(uint32 nNumShapes, HPHYSICSSHAPE* pChildren) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateBoundingVolumeShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSSHAPE hChild) { return HPHYSICSSHAPE(); }

HPHYSICSSHAPE CFearPhysicsSim::CreateContainerShape(HPHYSICSSHAPE hBoundingShape, HPHYSICSCONTAINER hContainer) { return HPHYSICSSHAPE(); }

LTRESULT CFearPhysicsSim::AddShapeRef(HPHYSICSSHAPE hShape) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseShape(HPHYSICSSHAPE hShape) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetShapeMass(HPHYSICSSHAPE hShape, float& fMassKg) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetShapeVolume(HPHYSICSSHAPE hShape, float& fVolume) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::TraverseShapeHeirarchy(HPHYSICSSHAPE hShape, IShapeTraversalCallback* pICallback, const LTRigidTransform& tInitialTransform) { return LT_UNSUPPORTED; }

HPHYSICSRIGIDBODY CFearPhysicsSim::CreateRigidBody(HPHYSICSSHAPE hShape, const LTRigidTransform& tStartAt, bool bPinned, EPhysicsGroup eGroup, uint32 nSystem, float fFriction, float fCOR) { return HPHYSICSRIGIDBODY(); }

LTRESULT CFearPhysicsSim::AddRigidBodyRef(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseRigidBody(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyShape(HPHYSICSRIGIDBODY hRigidBody, HPHYSICSSHAPE& hShape) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::PinRigidBody(HPHYSICSRIGIDBODY hRigidBody, bool bPin) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::IsRigidBodyPinned(HPHYSICSRIGIDBODY hRigidBody, bool& bPinned) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::EnableRigidBodyPinnedCollisions(HPHYSICSRIGIDBODY hRigidBody, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::TeleportRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTeleportTo) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyTransform(HPHYSICSRIGIDBODY hRigidBody, LTRigidTransform& tTransform) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyCenterOfMassInWorld(HPHYSICSRIGIDBODY hRigidBody, LTVector& vCOM) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyInertiaWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaWorld) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyInertiaInvWorld(HPHYSICSRIGIDBODY hRigidBody, LTMatrix3x4& matInertiaInvWorld) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyObject(HPHYSICSRIGIDBODY hRigidBody, HOBJECT& hObject) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup, uint32 nSystem) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyCollisionInfo(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup& eGroup, uint32& nSystem) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyCollisionGroup(HPHYSICSRIGIDBODY hRigidBody, EPhysicsGroup eGroup) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyCollisionSystem(HPHYSICSRIGIDBODY hRigidBody, uint32 nSystem) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool& bSolid) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodySolid(HPHYSICSRIGIDBODY hRigidBody, bool bSolid) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float fFriction) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyFriction(HPHYSICSRIGIDBODY hRigidBody, float& fFriction) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float fCOR) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyCOR(HPHYSICSRIGIDBODY hRigidBody, float& fCOR) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vVel) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vVel) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngVel) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodyAngularVelocity(HPHYSICSRIGIDBODY hRigidBody, LTVector& vAngVel) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ApplyRigidBodyForceWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSForce) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ApplyRigidBodyImpulseWorldSpace(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vApplyAtWS, const LTVector& vWSImpulse) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ApplyRigidBodyTorque(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vTorqueNm) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ApplyRigidBodyAngularImpulse(HPHYSICSRIGIDBODY hRigidBody, const LTVector& vAngImpulse) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::IsRigidBodyActive(HPHYSICSRIGIDBODY hRigidBody, bool& bActive) { return LT_UNSUPPORTED; }

HPHYSICSCOLLISIONNOTIFIER CFearPhysicsSim::RegisterCollisionNotifier(HPHYSICSRIGIDBODY hRigidBody, TFnRigidBodyCollisionCB pfnCallback, void* pUserData) { return HPHYSICSCOLLISIONNOTIFIER(); }

LTRESULT CFearPhysicsSim::ReleaseCollisionNotifier(HPHYSICSCOLLISIONNOTIFIER hNotifier) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::RemoveRigidBodyFromSimulation(HPHYSICSRIGIDBODY hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::KeyframeRigidBody(HPHYSICSRIGIDBODY hRigidBody, const LTRigidTransform& tTrans, float fTimeInterval) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetClosestPoint(HPHYSICSRIGIDBODY hRigidBody1, HPHYSICSRIGIDBODY hRigidBody2, LTVector& vClosestPt, float& fClosestDist) { return LT_UNSUPPORTED; }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateBallAndSocket(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateStiffSpring(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, float fDistance) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateLimitedHinge(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vPivotDir1, const LTVector& vPivotDir2, const LTVector& vPivotPerp1, const LTVector& vPivotPerp2, float fAngleMin, float fAngleMax, float fFriction) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateRagdoll(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vTwist1, const LTVector& vTwist2, const LTVector& vPerp1, const LTVector& vPerp2, float fConeAngleRad, float fPosRestrictAngleRad, float fNegRestrictAngleRad, float fTwistMinRad, float fTwistMaxRad, float fFriction) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreateWheel(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vRotation1, const LTVector& vRotation2, const LTVector& vSuspension2, float fSuspensionMin, float fSuspensionMax, float fSuspensionStrength, float fSuspensionDamping) { return HPHYSICSCONSTRAINT(); }

HPHYSICSCONSTRAINT CFearPhysicsSim::CreatePrismatic(HPHYSICSRIGIDBODY hBody1, HPHYSICSRIGIDBODY hBody2, const LTVector& vPivot1, const LTVector& vPivot2, const LTVector& vAxis1, const LTVector& vAxis2, const LTVector& vAxisPerp1, const LTVector& vAxisPerp2, float fMovementMin, float fMovementMax, float fFriction) { return HPHYSICSCONSTRAINT(); }

LTRESULT CFearPhysicsSim::AddConstraintRef(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseConstraint(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::AddConstraintToSimulation(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::RemoveConstraintFromSimulation(HPHYSICSCONSTRAINT hConstraint) { return LT_UNSUPPORTED; }

HPHYSICSBREAKABLE CFearPhysicsSim::CreateBreakable(HPHYSICSCONSTRAINT hConstraint, float fBreakableForce) { return HPHYSICSBREAKABLE(); }

LTRESULT CFearPhysicsSim::AddBreakableRef(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseBreakable(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::IsBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool& bBroken) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetBreakableBroken(HPHYSICSBREAKABLE hBreakable, bool bBroken) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::RegisterBreakableCallback(HPHYSICSBREAKABLE hBreakable, TFnBreakableBrokenCB pfnCallback, void* pUser) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::AddBreakableToSimulation(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::RemoveBreakableFromSimulation(HPHYSICSBREAKABLE hBreakable) { return LT_UNSUPPORTED; }

HPHYSICSCONTAINER CFearPhysicsSim::CreateContainer() { return HPHYSICSCONTAINER(); }

LTRESULT CFearPhysicsSim::AddContainerRef(HPHYSICSCONTAINER hContainer) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseContainer(HPHYSICSCONTAINER hContainer) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetNumRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, uint32& nNumRigidBodies) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetRigidBodiesInContainer(HPHYSICSCONTAINER hContainer, HPHYSICSRIGIDBODY* pList, uint32 nListSize, uint32& nNumFilledInObjects) { return LT_UNSUPPORTED; }

HPHYSICSACTION CFearPhysicsSim::CreateAction(HPHYSICSRIGIDBODY hBody) { return HPHYSICSACTION(); }

LTRESULT CFearPhysicsSim::AddActionRef(HPHYSICSACTION hAction) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ReleaseAction(HPHYSICSACTION hAction) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetActionCallback(HPHYSICSACTION hAction, TFNPhysicsActionCB pfnCallback, void* pUserData) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::EnableCollisions(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::EnableAllCollisions(EPhysicsGroup eGroup, bool bEnable) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::IsCollisionEnabled(EPhysicsGroup eGroup1, EPhysicsGroup eGroup2, bool& bEnabled) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::ApplyCollisionGroupChanges() { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetUniqueSystemGroup(uint32& nID) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetGlobalCollisionListener(TFnRigidBodyCollisionCB pfnCallback, void* pUserData) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::StepSimulationForward(float fElapsedTime, float fTargetUpdateRate, uint32 nMaxSteps) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetWorldModelRigidBody(HOBJECT hWorldModel, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetNumModelRigidBodies(HOBJECT hModel, uint32& nNumRigidBodies) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetModelRigidBody(HOBJECT hModel, uint32 nRigidBody, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::GetModelNodeRigidBody(HOBJECT hModel, HMODELNODE hNode, HPHYSICSRIGIDBODY& hRigidBody) { return LT_UNSUPPORTED; }

LTRESULT CFearPhysicsSim::SetObjectPhysicsGroup(HOBJECT hObject, EPhysicsGroup eGroup) { return LT_OK; }

CPhysicsSimulation* CFearPhysicsSim::GetSimulation() { return NULL; }
