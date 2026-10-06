// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SMeshBuffer.h and SMesh.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// A mesh buffer of standard vertices with 16-bit indices, and a mesh made of such buffers: all the
// OBJ loader produces.

#ifndef PORT_SCENE_SMESH_H
#define PORT_SCENE_SMESH_H

#include <vector>
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "scene/SceneMath.h"

namespace daisy {
namespace scene {

class SMeshBuffer : public ox::scene::IMeshBuffer
{
public:
    virtual ox::video::SMaterial& getMaterial() { return Material; }
    virtual const ox::video::SMaterial& getMaterial() const { return Material; }
    virtual ox::video::E_VERTEX_TYPE getVertexType() const { return ox::video::EVT_STANDARD; }
    virtual const void* getVertices() const { return Vertices.empty() ? 0 : &Vertices[0]; }
    virtual void* getVertices() { return Vertices.empty() ? 0 : &Vertices[0]; }
    virtual int getVertexCount() const { return (int)Vertices.size(); }
    virtual const unsigned short* getIndices() const { return Indices.empty() ? 0 : &Indices[0]; }
    virtual unsigned short* getIndices() { return Indices.empty() ? 0 : &Indices[0]; }
    virtual int getIndexCount() const { return (int)Indices.size(); }
    virtual const aabbox3df& getBoundingBox() const { return BoundingBox; }
    virtual aabbox3df& getBoundingBox() { return BoundingBox; }

    //! The box around all vertices; an empty buffer gets the box (0, 0, 0)-(0, 0, 0).
    void recalculateBoundingBox()
    {
        if (Vertices.empty())
        {
            resetBox(BoundingBox, vector3df(0, 0, 0));
            return;
        }

        resetBox(BoundingBox, Vertices[0].Pos);
        for (size_t i = 1; i < Vertices.size(); ++i)
            addInternalPoint(BoundingBox, Vertices[i].Pos);
    }

    ox::video::SMaterial Material;
    std::vector<ox::video::S3DVertex> Vertices;
    std::vector<unsigned short> Indices;
    aabbox3df BoundingBox;
};

class SMesh : public ox::scene::IMesh
{
public:
    virtual ~SMesh()
    {
        for (size_t i = 0; i < MeshBuffers.size(); ++i)
            MeshBuffers[i]->drop();
    }

    virtual int getMeshBufferCount() { return (int)MeshBuffers.size(); }
    virtual ox::scene::IMeshBuffer* getMeshBuffer(int nr) { return MeshBuffers[nr]; }
    virtual const aabbox3df& getBoundingBox() const { return BoundingBox; }
    virtual aabbox3df& getBoundingBox() { return BoundingBox; }

    virtual void setMaterialFlag(ox::video::E_MATERIAL_FLAG flag, bool newvalue)
    {
        for (size_t i = 0; i < MeshBuffers.size(); ++i)
            MeshBuffers[i]->getMaterial().Flags[flag] = newvalue;
    }

    void addMeshBuffer(ox::scene::IMeshBuffer* buffer)
    {
        if (!buffer)
            return;
        buffer->grab();
        MeshBuffers.push_back(buffer);
    }

    //! The box around all buffers' boxes.
    void recalculateBoundingBox()
    {
        if (MeshBuffers.empty())
        {
            resetBox(BoundingBox, vector3df(0, 0, 0));
            return;
        }

        BoundingBox = MeshBuffers[0]->getBoundingBox();
        for (size_t i = 1; i < MeshBuffers.size(); ++i)
        {
            addInternalPoint(BoundingBox, MeshBuffers[i]->getBoundingBox().MaxEdge);
            addInternalPoint(BoundingBox, MeshBuffers[i]->getBoundingBox().MinEdge);
        }
    }

    std::vector<ox::scene::IMeshBuffer*> MeshBuffers;
    aabbox3df BoundingBox;
};

} // end namespace scene
} // end namespace daisy

#endif
