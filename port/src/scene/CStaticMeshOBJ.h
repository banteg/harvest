// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CStaticMeshOBJ.h (license: third_party/irrlicht-0.7/include/irrlicht.h).

#ifndef PORT_SCENE_CSTATICMESHOBJ_H
#define PORT_SCENE_CSTATICMESHOBJ_H

#include "ox/scene/IAnimatedMesh.h"
#include "scene/SMesh.h"

namespace ox {
namespace io { class IReadFile; }
} // end namespace ox

namespace daisy {
namespace scene {

//! A Wavefront OBJ file as a one-frame animated mesh with one mesh buffer.
class CStaticMeshOBJ : public ox::scene::IAnimatedMesh
{
public:
    //! Reads the file; false if it is empty or has a face with 40 or more corners.
    bool loadFile(ox::io::IReadFile* file);

    virtual int getFrameCount();
    virtual ox::scene::IMesh* getMesh(int frame, int detailLevel, int startFrameLoop, int endFrameLoop);
    virtual const aabbox3df& getBoundingBox() const;

private:
    SMesh Mesh;
};

} // end namespace scene
} // end namespace daisy

#endif
