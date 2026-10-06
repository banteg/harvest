// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneCollisionManager.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Unchanged in daisy (Mac 0x10d9d6 getSceneNodeFromScreenCoordinatesBB, 0x10dd5a getPickedNodeBB,
// 0x10fc64 getRayFromScreenCoordinates, 0x10feb0 getScreenCoordinatesFrom3DPosition).

#include "scene/CSceneCollisionManager.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"
#include "scene/SViewFrustrum.h"

namespace daisy {
namespace scene {

CSceneCollisionManager::CSceneCollisionManager(ox::scene::ISceneManager* smanager, ox::video::IVideoDriver* driver)
    : SceneManager(smanager), Driver(driver)
{
    if (Driver)
        Driver->grab();
}

CSceneCollisionManager::~CSceneCollisionManager()
{
    if (Driver)
        Driver->drop();
}

//! The ray from the camera through the screen point to the far plane, through the frustum's far
//! corners: x / width runs from the left edge to the right, y / height from the top down.
static ox::core::CLine3d<float> rayThroughScreen(const ox::scene::SViewFrustrum* f, ox::core::CPosition2d<int> pos,
    const ox::core::CDimension2d<int>& screenSize)
{
    vector3df farLeftUp = f->getFarLeftUp();
    vector3df lefttoright = f->getFarRightUp() - farLeftUp;
    vector3df uptodown = f->getFarLeftDown() - farLeftUp;

    float dx = pos.X / (float)screenSize.Width;
    float dy = pos.Y / (float)screenSize.Height;

    return ox::core::CLine3d<float>(f->cameraPosition, farLeftUp + (lefttoright * dx) + (uptodown * dy));
}

ox::scene::ISceneNode* CSceneCollisionManager::getSceneNodeFromScreenCoordinatesBB(ox::core::CPosition2d<int> pos,
    int idBitMask)
{
    if (!SceneManager || !Driver)
        return 0;

    ox::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
    if (!camera)
        return 0;

    return getSceneNodeFromRayBB(rayThroughScreen(camera->getViewFrustrum(), pos, Driver->getScreenSize()), idBitMask);
}

//! Of the visible nodes whose world box the ray crosses, the one whose absolute position is
//! nearest the ray's start (not the nearest hit).
ox::scene::ISceneNode* CSceneCollisionManager::getSceneNodeFromRayBB(ox::core::CLine3d<float> ray, int idBitMask)
{
    ox::scene::ISceneNode* best = 0;
    float dist = 9999999999.0f;

    vector3df middle = (ray.start + ray.end) * 0.5f;
    vector3df vect = ray.end - ray.start;
    vect.normalize();
    float halflength = (float)(ray.start.getDistanceFrom(ray.end) * 0.5);

    getPickedNodeBB(SceneManager->getRootSceneNode(), middle, vect, ray.start, halflength, idBitMask, dist, best);

    return best;
}

//! Walks the whole tree, including the children of invisible nodes.
void CSceneCollisionManager::getPickedNodeBB(ox::scene::ISceneNode* root, const vector3df& linemiddle,
    const vector3df& linevect, const vector3df& pos, float halflength, int bits, float& outbestdistance,
    ox::scene::ISceneNode*& outbestnode)
{
    const ox::TList<ox::scene::ISceneNode*>& children = root->getChildren();
    for (ox::TList<ox::scene::ISceneNode*>::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        ox::scene::ISceneNode* current = *it;

        if (current->isVisible() && (bits == 0 || (current->getID() & bits)))
        {
            if (intersectsWithLine(current->getTransformedBoundingBox(), linemiddle, linevect, halflength))
            {
                float dist = (float)current->getAbsolutePosition().getDistanceFrom(pos);
                if (dist < outbestdistance)
                {
                    outbestnode = current;
                    outbestdistance = dist;
                }
            }
        }

        getPickedNodeBB(current, linemiddle, linevect, pos, halflength, bits, outbestdistance, outbestnode);
    }
}

ox::scene::ISceneNode* CSceneCollisionManager::getSceneNodeFromCameraBB(ox::scene::ICameraSceneNode* camera,
    int idBitMask)
{
    if (!camera)
        return 0;

    vector3df start = camera->getAbsolutePosition();
    vector3df end = camera->getTarget();
    end = start + ((end - start).normalize() * camera->getFarValue());

    return getSceneNodeFromRayBB(ox::core::CLine3d<float>(start, end), idBitMask);
}

ox::core::CLine3d<float> CSceneCollisionManager::getRayFromScreenCoordinates(ox::core::CPosition2d<int> pos,
    ox::scene::ICameraSceneNode* camera)
{
    ox::core::CLine3d<float> ln;

    if (!SceneManager)
        return ln;

    if (!camera)
        camera = SceneManager->getActiveCamera();

    if (!camera)
        return ln;

    return rayThroughScreen(camera->getViewFrustrum(), pos, Driver->getScreenSize());
}

//! Projects with projection x view: (W/2 * x/w + W/2, H/2 - H/2 * y/w) with integer halves of the
//! screen size, truncated; (-10000, -10000) behind the camera, (-1000, -1000) without one.
ox::core::CPosition2d<int> CSceneCollisionManager::getScreenCoordinatesFrom3DPosition(vector3df pos3d,
    ox::scene::ICameraSceneNode* camera)
{
    ox::core::CPosition2d<int> pos2d(-1000, -1000);

    if (!SceneManager || !Driver)
        return pos2d;

    if (!camera)
        camera = SceneManager->getActiveCamera();

    if (!camera)
        return pos2d;

    ox::core::CDimension2d<int> dim = Driver->getScreenSize();
    dim.Width /= 2;
    dim.Height /= 2;

    matrix4 trans = camera->getProjectionMatrix();
    trans *= camera->getViewMatrix();

    float transformedPos[4] = { pos3d.X, pos3d.Y, pos3d.Z, 1.0f };
    multiplyWith1x4Matrix(trans, transformedPos);

    if (transformedPos[3] < 0)
        return ox::core::CPosition2d<int>(-10000, -10000);

    float zDiv = transformedPos[3] == 0.0f ? 1.0f : (1.0f / transformedPos[3]);

    pos2d.X = (int)(dim.Width * transformedPos[0] * zDiv) + dim.Width;
    pos2d.Y = ((int)(dim.Height - (dim.Height * (transformedPos[1] * zDiv))));
    return pos2d;
}

bool CSceneCollisionManager::getCollisionPoint(const ox::core::CLine3d<float>& ray,
    ox::scene::ITriangleSelector* selector, vector3df& outCollisionPoint, ox::core::CTriangle3d<float>& outTriangle)
{
    return false;
}

vector3df CSceneCollisionManager::getCollisionResultPosition(ox::scene::ITriangleSelector* selector,
    const vector3df& ellipsoidPosition, const vector3df& ellipsoidRadius, const vector3df& ellipsoidDirectionAndSpeed,
    ox::core::CTriangle3d<float>& triout, bool& outFalling, float slidingSpeed,
    const vector3df& gravityDirectionAndSpeed)
{
    return ellipsoidPosition;
}

} // end namespace scene
} // end namespace daisy
