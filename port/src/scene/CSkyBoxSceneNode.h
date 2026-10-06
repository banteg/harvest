// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSkyBoxSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CSKYBOXSCENENODE_H
#define PORT_SCENE_CSKYBOXSCENENODE_H

#include "ox/scene/ISceneNode.h"
#include "ox/video/S3DVertex.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

//! A textured cube of half-size 10 drawn around the camera before everything else.
class CSkyBoxSceneNode : public ox::scene::ISceneNode
{
public:
    //! Faces in material order: front (-z), left (+x), back (+z), right (-x), top, bottom.
    enum EFace
    {
        FRONT = 0,
        LEFT,
        BACK,
        RIGHT,
        TOP,
        BOTTOM,
        FACE_COUNT
    };

    CSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom, ox::video::ITexture* left,
        ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back,
        ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id);

    virtual void OnPreRender();
    virtual void render();
    virtual const aabbox3df& getBoundingBox() const;
    virtual ox::video::SMaterial& getMaterial(int i);
    virtual int getMaterialCount();

private:
    aabbox3df Box;
    unsigned short Indices[6];
    ox::video::S3DVertex Vertices[FACE_COUNT * 4];
    ox::video::SMaterial Material[FACE_COUNT];
};

} // end namespace scene
} // end namespace daisy

#endif
