// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SViewFrustrum.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// The port's definition of the view frustum that ox::scene::ICameraSceneNode::getViewFrustrum
// returns. The Mac 1.18 build uses the same layout: the camera position, six planes (far, near,
// left, right, bottom, top) and the bounding box at offset 0x6c.

#ifndef PORT_SCENE_SVIEWFRUSTRUM_H
#define PORT_SCENE_SVIEWFRUSTRUM_H

#include "scene/SceneMath.h"

namespace ox {
namespace scene {

struct SViewFrustrum
{
    enum VFPLANES
    {
        VF_FAR_PLANE = 0,
        VF_NEAR_PLANE,
        VF_LEFT_PLANE,
        VF_RIGHT_PLANE,
        VF_BOTTOM_PLANE,
        VF_TOP_PLANE,
        VF_PLANE_COUNT
    };

    SViewFrustrum() {}

    //! The planes of the clip volume of a projection (times view) matrix, pointing outwards.
    SViewFrustrum(const core::CMatrix4& mat)
    {
#define sw(a, b) (mat((b), (a)))
        planes[VF_LEFT_PLANE].Normal.X = -(sw(0, 3) + sw(0, 0));
        planes[VF_LEFT_PLANE].Normal.Y = -(sw(1, 3) + sw(1, 0));
        planes[VF_LEFT_PLANE].Normal.Z = -(sw(2, 3) + sw(2, 0));
        planes[VF_LEFT_PLANE].D = -(sw(3, 3) + sw(3, 0));

        planes[VF_RIGHT_PLANE].Normal.X = -(sw(0, 3) - sw(0, 0));
        planes[VF_RIGHT_PLANE].Normal.Y = -(sw(1, 3) - sw(1, 0));
        planes[VF_RIGHT_PLANE].Normal.Z = -(sw(2, 3) - sw(2, 0));
        planes[VF_RIGHT_PLANE].D = -(sw(3, 3) - sw(3, 0));

        planes[VF_TOP_PLANE].Normal.X = -(sw(0, 3) - sw(0, 1));
        planes[VF_TOP_PLANE].Normal.Y = -(sw(1, 3) - sw(1, 1));
        planes[VF_TOP_PLANE].Normal.Z = -(sw(2, 3) - sw(2, 1));
        planes[VF_TOP_PLANE].D = -(sw(3, 3) - sw(3, 1));

        planes[VF_BOTTOM_PLANE].Normal.X = -(sw(0, 3) + sw(0, 1));
        planes[VF_BOTTOM_PLANE].Normal.Y = -(sw(1, 3) + sw(1, 1));
        planes[VF_BOTTOM_PLANE].Normal.Z = -(sw(2, 3) + sw(2, 1));
        planes[VF_BOTTOM_PLANE].D = -(sw(3, 3) + sw(3, 1));

        planes[VF_NEAR_PLANE].Normal.X = -sw(0, 2);
        planes[VF_NEAR_PLANE].Normal.Y = -sw(1, 2);
        planes[VF_NEAR_PLANE].Normal.Z = -sw(2, 2);
        planes[VF_NEAR_PLANE].D = -sw(3, 2);

        planes[VF_FAR_PLANE].Normal.X = -(sw(0, 3) - sw(0, 2));
        planes[VF_FAR_PLANE].Normal.Y = -(sw(1, 3) - sw(1, 2));
        planes[VF_FAR_PLANE].Normal.Z = -(sw(2, 3) - sw(2, 2));
        planes[VF_FAR_PLANE].D = -(sw(3, 3) - sw(3, 2));
#undef sw

        for (int i = 0; i < VF_PLANE_COUNT; ++i)
        {
            float len = (float)(1.0f / planes[i].Normal.getLength());
            planes[i].Normal *= len;
            planes[i].D *= len;
        }

        recalculateBoundingBox();
    }

    core::CVector3d<float> getFarLeftUp() const
    {
        return corner(VF_TOP_PLANE, VF_LEFT_PLANE);
    }

    core::CVector3d<float> getFarLeftDown() const
    {
        return corner(VF_BOTTOM_PLANE, VF_LEFT_PLANE);
    }

    core::CVector3d<float> getFarRightUp() const
    {
        return corner(VF_TOP_PLANE, VF_RIGHT_PLANE);
    }

    core::CVector3d<float> getFarRightDown() const
    {
        return corner(VF_BOTTOM_PLANE, VF_RIGHT_PLANE);
    }

    //! The box around the camera position and the four far corners.
    void recalculateBoundingBox()
    {
        core::CAabbox3d<float> box;
        daisy::scene::resetBox(box, cameraPosition);
        daisy::scene::addInternalPoint(box, getFarLeftUp());
        daisy::scene::addInternalPoint(box, getFarRightUp());
        daisy::scene::addInternalPoint(box, getFarLeftDown());
        daisy::scene::addInternalPoint(box, getFarRightDown());
        boundingBox = box;
    }

    core::CVector3d<float> cameraPosition;
    daisy::scene::SPlane3d planes[VF_PLANE_COUNT];
    //! Default constructed this is Irrlicht's (-1, -1, -1)-(1, 1, 1), which is what culling sees
    //! before the camera's first OnPreRender.
    core::CAabbox3d<float> boundingBox;

private:
    //! The intersection of the far plane with two side planes; (0, 0, 0) if there is none.
    core::CVector3d<float> corner(VFPLANES vertical, VFPLANES horizontal) const
    {
        core::CVector3d<float> p;
        planes[VF_FAR_PLANE].getIntersectionWithPlanes(planes[vertical], planes[horizontal], p);
        return p;
    }
};

} // end namespace scene
} // end namespace ox

#endif
