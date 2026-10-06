// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CLightSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Checked against the Mac 1.18 build (constructor 0xfe1ea, OnPreRender 0xfe3cc, render 0xfe406).
// daisy's constructor also copies the colour into the specular colour, and initialises the rest of
// the light as Irrlicht's SLight does: black ambient (alpha 1), shadows on, not directional.

#include "scene/CLightSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/IVideoDriver.h"

namespace daisy {
namespace scene {

CLightSceneNode::CLightSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id,
    const vector3df& position, ox::video::SColorf color, float radius)
    : ox::scene::ILightSceneNode(parent, mgr, id, position)
{
    LightData.AmbientColor = ox::video::SColorf(0.0f, 0.0f, 0.0f);
    LightData.DiffuseColor = color;
    LightData.SpecularColor = color;
    LightData.Position = position;
    LightData.Radius = radius;
    LightData.CastShadows = true;
    LightData.Directional = false;
}

void CLightSceneNode::OnPreRender()
{
    if (!IsVisible)
        return;

    SceneManager->registerNodeForRendering(this, ox::scene::SNRT_LIGHT_AND_CAMERA);
    ISceneNode::OnPreRender();
}

void CLightSceneNode::render()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    if (!driver)
        return;

    LightData.Position = getAbsolutePosition();
    driver->addDynamicLight(LightData);
}

const aabbox3df& CLightSceneNode::getBoundingBox() const
{
    return BBox;
}

ox::video::SLight& CLightSceneNode::getLightData()
{
    return LightData;
}

} // end namespace scene
} // end namespace daisy
