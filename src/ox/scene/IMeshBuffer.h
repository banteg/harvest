// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IMeshBuffer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The slots CGUIMeshViewer::draw
// calls are verified against the Mac 1.18 build.

#ifndef OX_SCENE_IMESHBUFFER_H
#define OX_SCENE_IMESHBUFFER_H

#include "../IUnknown.h"
#include "../video/S3DVertex.h"

namespace ox {
namespace core {
template <class T> class CAabbox3d;
} // end namespace core

namespace video {
struct SMaterial;
} // end namespace video

namespace scene {

//! A part of a mesh with one material.
class IMeshBuffer : public IUnknown
{
public:
    virtual ~IMeshBuffer() {}

    virtual video::SMaterial& getMaterial() = 0;
    virtual const video::SMaterial& getMaterial() const = 0;
    virtual video::E_VERTEX_TYPE getVertexType() const = 0;
    virtual const void* getVertices() const = 0;
    virtual void* getVertices() = 0;
    virtual int getVertexCount() const = 0;
    virtual const unsigned short* getIndices() const = 0;
    virtual unsigned short* getIndices() = 0;
    virtual int getIndexCount() const = 0;
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
    virtual core::CAabbox3d<float>& getBoundingBox() = 0;
};

} // end namespace scene
} // end namespace ox

#endif
