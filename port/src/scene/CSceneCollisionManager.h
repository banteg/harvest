// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneCollisionManager.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CSCENECOLLISIONMANAGER_H
#define PORT_SCENE_CSCENECOLLISIONMANAGER_H

#include "ox/core/CLine3d.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "scene/SceneMath.h"

namespace ox {
namespace video { class IVideoDriver; }
namespace scene { class ISceneManager; }
} // end namespace ox

namespace daisy {
namespace scene {

//! Picking by bounding box and projection to the screen. The triangle-selector collision
//! queries are not part of the port (the game never makes a selector).
class CSceneCollisionManager : public ox::scene::ISceneCollisionManager
{
public:
    CSceneCollisionManager(ox::scene::ISceneManager* smanager, ox::video::IVideoDriver* driver);
    virtual ~CSceneCollisionManager();

    virtual bool getCollisionPoint(const ox::core::CLine3d<float>& ray, ox::scene::ITriangleSelector* selector,
        vector3df& outCollisionPoint, ox::core::CTriangle3d<float>& outTriangle);
    virtual vector3df getCollisionResultPosition(ox::scene::ITriangleSelector* selector,
        const vector3df& ellipsoidPosition, const vector3df& ellipsoidRadius,
        const vector3df& ellipsoidDirectionAndSpeed, ox::core::CTriangle3d<float>& triout, bool& outFalling,
        float slidingSpeed, const vector3df& gravityDirectionAndSpeed);
    virtual ox::core::CLine3d<float> getRayFromScreenCoordinates(ox::core::CPosition2d<int> pos,
        ox::scene::ICameraSceneNode* camera);
    virtual ox::core::CPosition2d<int> getScreenCoordinatesFrom3DPosition(vector3df pos,
        ox::scene::ICameraSceneNode* camera);
    virtual ox::scene::ISceneNode* getSceneNodeFromScreenCoordinatesBB(ox::core::CPosition2d<int> pos, int idBitMask);
    virtual ox::scene::ISceneNode* getSceneNodeFromRayBB(ox::core::CLine3d<float> ray, int idBitMask);
    virtual ox::scene::ISceneNode* getSceneNodeFromCameraBB(ox::scene::ICameraSceneNode* camera, int idBitMask);

private:
    void getPickedNodeBB(ox::scene::ISceneNode* root, const vector3df& linemiddle, const vector3df& linevect,
        const vector3df& pos, float halflength, int bits, float& outbestdistance, ox::scene::ISceneNode*& outbestnode);

    ox::scene::ISceneManager* SceneManager;
    ox::video::IVideoDriver* Driver;
};

} // end namespace scene
} // end namespace daisy

#endif
