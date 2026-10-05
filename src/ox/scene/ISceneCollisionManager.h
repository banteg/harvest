// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneCollisionManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The virtual order follows
// the Mac 1.18 vtable of daisy::scene::CSceneCollisionManager.

#ifndef OX_SCENE_ISCENECOLLISIONMANAGER_H
#define OX_SCENE_ISCENECOLLISIONMANAGER_H

#include "../IUnknown.h"
#include "../core/CPosition2d.h"
#include "../core/CVector3d.h"

namespace ox {
namespace core {
template <class T> class CLine3d;
template <class T> class CTriangle3d;
} // end namespace core
namespace scene {

class ICameraSceneNode;
class ISceneNode;
class ITriangleSelector;

//! Collision tests against the scene.
class ISceneCollisionManager : public IUnknown
{
public:
    virtual bool getCollisionPoint(const core::CLine3d<float>& ray, ITriangleSelector* selector,
        core::CVector3d<float>& outCollisionPoint, core::CTriangle3d<float>& outTriangle) = 0;
    virtual core::CVector3d<float> getCollisionResultPosition(ITriangleSelector* selector,
        const core::CVector3d<float>& ellipsoidPosition, const core::CVector3d<float>& ellipsoidRadius,
        const core::CVector3d<float>& ellipsoidDirectionAndSpeed, core::CTriangle3d<float>& triout,
        bool& outFalling, float slidingSpeed, const core::CVector3d<float>& gravityDirectionAndSpeed) = 0;
    virtual core::CLine3d<float> getRayFromScreenCoordinates(core::CPosition2d<int> pos,
        ICameraSceneNode* camera) = 0;
    virtual core::CPosition2d<int> getScreenCoordinatesFrom3DPosition(core::CVector3d<float> pos,
        ICameraSceneNode* camera = 0) = 0;
    virtual ISceneNode* getSceneNodeFromScreenCoordinatesBB(core::CPosition2d<int> pos, int idBitMask = 0) = 0;
    virtual ISceneNode* getSceneNodeFromRayBB(core::CLine3d<float> ray, int idBitMask) = 0;
    virtual ISceneNode* getSceneNodeFromCameraBB(ICameraSceneNode* camera, int idBitMask) = 0;
};

} // end namespace scene
} // end namespace ox

#endif
