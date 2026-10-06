// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSkyBoxSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
//
// Checked against the Mac 1.18 build (constructor 0x11930a, render 0x11a0b4). daisy keeps
// Irrlicht's cube, material and draw, and changes the texture coordinates: U is mirrored on the
// four side faces, the top face is Irrlicht's reflected across the anti-diagonal and the bottom
// face is Irrlicht's transposed (docs/port/menu-scene.md, "Skybox").

#include "scene/CSkyBoxSceneNode.h"
#include "ox/scene/ICameraSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "ox/video/ITexture.h"
#include "ox/video/IVideoDriver.h"
#include "ox/video/S3DVertexInline.h"

namespace daisy {
namespace scene {

CSkyBoxSceneNode::CSkyBoxSceneNode(ox::video::ITexture* top, ox::video::ITexture* bottom, ox::video::ITexture* left,
    ox::video::ITexture* right, ox::video::ITexture* front, ox::video::ITexture* back,
    ox::scene::ISceneNode* parent, ox::scene::ISceneManager* mgr, int id)
    : ox::scene::ISceneNode(parent, mgr, id)
{
    Indices[0] = 0;
    Indices[1] = 1;
    Indices[2] = 2;
    Indices[3] = 0;
    Indices[4] = 2;
    Indices[5] = 3;

    // A half-pixel inset of the first texture there is, so that the edges do not sample across.
    ox::video::ITexture* sizeTexture = front;
    if (!sizeTexture) sizeTexture = left;
    if (!sizeTexture) sizeTexture = back;
    if (!sizeTexture) sizeTexture = right;
    if (!sizeTexture) sizeTexture = top;
    if (!sizeTexture) sizeTexture = bottom;

    float onepixel = 0.0f;
    if (sizeTexture)
        onepixel = 1.0f / ((float)sizeTexture->getSize().Width * 1.5f);

    const float o = 0.0f + onepixel;
    const float t = 1.0f - onepixel;
    const float l = 10.0f;
    const ox::video::SColor white(0xffffffff);

    ox::video::ITexture* textures[FACE_COUNT] = { front, left, back, right, top, bottom };
    for (int i = 0; i < FACE_COUNT; ++i)
    {
        Material[i].Lighting = false;
        Material[i].ZBuffer = false;
        Material[i].ZWriteEnable = false;
        Material[i].Texture1 = textures[i];
    }

    // front, z = -10
    Vertices[0] = ox::video::S3DVertex(-l, -l, -l, 0, 0, 1, white, t, t);
    Vertices[1] = ox::video::S3DVertex(l, -l, -l, 0, 0, 1, white, o, t);
    Vertices[2] = ox::video::S3DVertex(l, l, -l, 0, 0, 1, white, o, o);
    Vertices[3] = ox::video::S3DVertex(-l, l, -l, 0, 0, 1, white, t, o);

    // left, x = +10
    Vertices[4] = ox::video::S3DVertex(l, -l, -l, -1, 0, 0, white, t, t);
    Vertices[5] = ox::video::S3DVertex(l, -l, l, -1, 0, 0, white, o, t);
    Vertices[6] = ox::video::S3DVertex(l, l, l, -1, 0, 0, white, o, o);
    Vertices[7] = ox::video::S3DVertex(l, l, -l, -1, 0, 0, white, t, o);

    // back, z = +10
    Vertices[8] = ox::video::S3DVertex(l, -l, l, 0, 0, -1, white, t, t);
    Vertices[9] = ox::video::S3DVertex(-l, -l, l, 0, 0, -1, white, o, t);
    Vertices[10] = ox::video::S3DVertex(-l, l, l, 0, 0, -1, white, o, o);
    Vertices[11] = ox::video::S3DVertex(l, l, l, 0, 0, -1, white, t, o);

    // right, x = -10
    Vertices[12] = ox::video::S3DVertex(-l, -l, l, 1, 0, 0, white, t, t);
    Vertices[13] = ox::video::S3DVertex(-l, -l, -l, 1, 0, 0, white, o, t);
    Vertices[14] = ox::video::S3DVertex(-l, l, -l, 1, 0, 0, white, o, o);
    Vertices[15] = ox::video::S3DVertex(-l, l, l, 1, 0, 0, white, t, o);

    // top, y = +10
    Vertices[16] = ox::video::S3DVertex(l, l, l, 0, -1, 0, white, t, t);
    Vertices[17] = ox::video::S3DVertex(-l, l, l, 0, -1, 0, white, o, t);
    Vertices[18] = ox::video::S3DVertex(-l, l, -l, 0, -1, 0, white, o, o);
    Vertices[19] = ox::video::S3DVertex(l, l, -l, 0, -1, 0, white, t, o);

    // bottom, y = -10
    Vertices[20] = ox::video::S3DVertex(-l, -l, l, 0, 1, 0, white, o, o);
    Vertices[21] = ox::video::S3DVertex(l, -l, l, 0, 1, 0, white, t, o);
    Vertices[22] = ox::video::S3DVertex(l, -l, -l, 0, 1, 0, white, t, t);
    Vertices[23] = ox::video::S3DVertex(-l, -l, -l, 0, 1, 0, white, o, t);
}

void CSkyBoxSceneNode::OnPreRender()
{
    if (IsVisible)
        SceneManager->registerNodeForRendering(this, ox::scene::SNRT_SKY_BOX);

    ISceneNode::OnPreRender();
}

//! Each face with its own material, translated to the camera's absolute position.
void CSkyBoxSceneNode::render()
{
    ox::video::IVideoDriver* driver = SceneManager->getVideoDriver();
    ox::scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();
    if (!camera || !driver)
        return;

    matrix4 mat;
    mat.setTranslation(camera->getAbsolutePosition());
    driver->setTransform(ox::video::ETS_WORLD, mat);

    for (int i = 0; i < FACE_COUNT; ++i)
    {
        driver->setMaterial(Material[i]);
        driver->drawIndexedTriangleList(&Vertices[i * 4], 4, Indices, 2);
    }
}

const aabbox3df& CSkyBoxSceneNode::getBoundingBox() const
{
    return Box;
}

ox::video::SMaterial& CSkyBoxSceneNode::getMaterial(int i)
{
    return Material[i];
}

int CSkyBoxSceneNode::getMaterialCount()
{
    return FACE_COUNT;
}

} // end namespace scene
} // end namespace daisy
