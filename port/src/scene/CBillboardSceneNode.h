// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBillboardSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CBILLBOARDSCENENODE_H
#define PORT_SCENE_CBILLBOARDSCENENODE_H

#include "ox/scene/IBillboardSceneNode.h"
#include "ox/video/S3DVertex.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

//! A quad of the node's size facing the active camera (the menu's sun).
class CBillboardSceneNode : public ox::scene::IBillboardSceneNode
{
public:
    CBillboardSceneNode(ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id,
        const vector3df& position, const ox::core::CDimension2d<float>& size);

    virtual void OnPreRender();
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;
    virtual ox::video::SMaterial& getMaterial(int i);
    virtual int getMaterialCount();
    virtual void setSize(const ox::core::CDimension2d<float>& size);
    virtual const ox::core::CDimension2d<float>& getSize();

private:
    ox::core::CDimension2d<float> Size;
    aabbox3df BBox;
    ox::video::SMaterial Material;
    ox::video::S3DVertex Vertices[4];
    unsigned short Indices[6];
};

} // end namespace scene
} // end namespace daisy

#endif
