// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBillboardSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Unchanged in daisy (Mac 0xf65ba-0xf6d2e): the quad is built from the camera's target and up
// vector, drawn with an identity world matrix, and automatic culling is off.

#include "scene/CBillboardSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace scene {

CBillboardSceneNode::CBillboardSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id,
    const vector3df& position, const ox::core::CDimension2d<float>& size)
    : ox::scene::IBillboardSceneNode(parent, mgr, id, position)
{
    setSize(size);

    AutomaticCullingEnabled = false;

    Indices[0] = 0;
    Indices[1] = 2;
    Indices[2] = 1;
    Indices[3] = 0;
    Indices[4] = 3;
    Indices[5] = 2;

    Vertices[0].TCoords.set(0.0f, 0.0f);
    Vertices[0].Color = ox::video::SColor(0xffffffff);
    Vertices[1].TCoords.set(0.0f, 1.0f);
    Vertices[1].Color = ox::video::SColor(0xffffffff);
    Vertices[2].TCoords.set(1.0f, 1.0f);
    Vertices[2].Color = ox::video::SColor(0xffffffff);
    Vertices[3].TCoords.set(1.0f, 0.0f);
    Vertices[3].Color = ox::video::SColor(0xffffffff);
}

void CBillboardSceneNode::OnPreRender()
{
    if (!IsVisible)
        return;

    SceneManager->registerNodeForRendering(this, ox::scene::SNRT_DEFAULT);
    ISceneNode::OnPreRender();
}

void CBillboardSceneNode::render()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    ox::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
    if (!camera || !driver)
        return;

    vector3df pos = getAbsolutePosition();

    vector3df campos = camera->getAbsolutePosition();
    vector3df target = camera->getTarget();
    vector3df up = camera->getUpVector();
    vector3df view = target - campos;
    view.normalize();

    vector3df horizontal = up.crossProduct(view);
    horizontal.normalize();

    vector3df vertical = horizontal.crossProduct(view);
    vertical.normalize();

    horizontal *= 0.5f * Size.Width;
    vertical *= 0.5f * Size.Height;

    Vertices[0].Pos = pos + horizontal + vertical;
    Vertices[1].Pos = pos + horizontal - vertical;
    Vertices[2].Pos = pos - horizontal - vertical;
    Vertices[3].Pos = pos - horizontal + vertical;

    view *= -1.0f;
    for (int i = 0; i < 4; ++i)
        Vertices[i].Normal = view;

    matrix4 mat;
    driver->setTransform(ox::video::ETS_WORLD, mat);
    driver->setMaterial(Material);
    driver->drawIndexedTriangleList(Vertices, 4, Indices, 2);
}

const aabbox3df& CBillboardSceneNode::getBoundingBox() const
{
    return BBox;
}

void CBillboardSceneNode::setSize(const ox::core::CDimension2d<float>& size)
{
    Size = size;
}

ox::video::SMaterial& CBillboardSceneNode::getMaterial(int i)
{
    return Material;
}

int CBillboardSceneNode::getMaterialCount()
{
    return 1;
}

const ox::core::CDimension2d<float>& CBillboardSceneNode::getSize()
{
    return Size;
}

} // end namespace scene
} // end namespace daisy
