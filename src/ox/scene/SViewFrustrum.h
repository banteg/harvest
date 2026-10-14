// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/SViewFrustrum.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __S_VIEW_FRUSTRUM_H_INCLUDED__
#define __S_VIEW_FRUSTRUM_H_INCLUDED__

#include "ox/core/CPlane3d.h"
#include "ox/core/CVector3d.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"

namespace ox
{
namespace scene
{

	//! Defines the view frustrum. Thats the space viewed by the camera.
	/** The view frustrum is enclosed by 6 planes. These six planes share
	four points. ox::core::A bounding box around these four points is also stored in
	this structure.
	*/
	struct SViewFrustrum
	{
		enum VFPLANES
		{
			//! Far plane of the frustrum. Thats the plane farest away from the eye.
			VF_FAR_PLANE = 0,
			//! Near plane of the frustrum. Thats the plane nearest to the eye.
			VF_NEAR_PLANE,
			//! Left plane of the frustrum.
			VF_LEFT_PLANE,
			//! Right plane of the frustrum.
			VF_RIGHT_PLANE,
			//! Bottom plane of the frustrum.
			VF_BOTTOM_PLANE,
			//! Top plane of the frustrum.
			VF_TOP_PLANE,

			//! Amount of planes enclosing the view frustum. Should be 6.
			VF_PLANE_COUNT
		};

		//! Default Constructor
		SViewFrustrum() {};

		//! This constructor creates a view frustrum based on a projection and/or
		//! view matrix.
		SViewFrustrum(const ox::core::CMatrix4& mat);

		//! the position of the camera
		ox::core::CVector3d<float> cameraPosition;

		//! all planes enclosing the view frustrum.
		ox::core::CPlane3d<float> planes[VF_PLANE_COUNT];

		//! bouding box around the view frustrum
		ox::core::CAabbox3d<float> boundingBox;

		//! transforms the furstum by the matrix
		//! \param Matrix by which the view frustrum is transformed.
		void transform(const ox::core::CMatrix4 &mat);

		//! returns the point which is on the far left upper corner inside the
		//! the view frustrum.
		ox::core::CVector3d<float> getFarLeftUp() const;

		//! returns the point which is on the far left bottom corner inside the
		//! the view frustrum.
		ox::core::CVector3d<float> getFarLeftDown() const;

		//! returns the point which is on the far right top corner inside the
		//! the view frustrum.
		ox::core::CVector3d<float> getFarRightUp() const;

		//! returns the point which is on the far right bottom corner inside the
		//! the view frustrum.
		ox::core::CVector3d<float> getFarRightDown() const;

		//! returns a bounding box enclosing the whole view frustrum
		ox::core::CAabbox3d<float> getBoundingBox() const;

		//! recalculates the bounding box member based on the planes
		void recalculateBoundingBox();
	};


	//! transforms the furstum by the matrix
	//! \param Matrix by which the view frustrum is transformed.
	inline void SViewFrustrum::transform(const ox::core::CMatrix4 &mat)
	{
		for (int i=0; i<VF_PLANE_COUNT; ++i)
			mat.transformPlane(planes[i]);

		mat.transformVect(cameraPosition);

		recalculateBoundingBox();
    }


	//! returns the point which is on the far left upper corner inside the
	//! the view frustrum.
	inline ox::core::CVector3d<float> SViewFrustrum::getFarLeftUp() const
	{
		ox::core::CVector3d<float> p;
		planes[ox::scene::SViewFrustrum::VF_FAR_PLANE].getIntersectionWithPlanes(
			planes[ox::scene::SViewFrustrum::VF_TOP_PLANE],
			planes[ox::scene::SViewFrustrum::VF_LEFT_PLANE], p);

		return p;
	}

	//! returns the point which is on the far left bottom corner inside the
	//! the view frustrum.
	inline ox::core::CVector3d<float> SViewFrustrum::getFarLeftDown() const
	{
		ox::core::CVector3d<float> p;
		planes[ox::scene::SViewFrustrum::VF_FAR_PLANE].getIntersectionWithPlanes(
			planes[ox::scene::SViewFrustrum::VF_BOTTOM_PLANE],
			planes[ox::scene::SViewFrustrum::VF_LEFT_PLANE], p);

		return p;
	}

	//! returns the point which is on the far right top corner inside the
	//! the view frustrum.
	inline ox::core::CVector3d<float> SViewFrustrum::getFarRightUp() const
	{
		ox::core::CVector3d<float> p;
		planes[ox::scene::SViewFrustrum::VF_FAR_PLANE].getIntersectionWithPlanes(
			planes[ox::scene::SViewFrustrum::VF_TOP_PLANE],
			planes[ox::scene::SViewFrustrum::VF_RIGHT_PLANE], p);

		return p;
	}

	//! returns the point which is on the far right bottom corner inside the
	//! the view frustrum.
	inline ox::core::CVector3d<float> SViewFrustrum::getFarRightDown() const
	{
		ox::core::CVector3d<float> p;
		planes[ox::scene::SViewFrustrum::VF_FAR_PLANE].getIntersectionWithPlanes(
			planes[ox::scene::SViewFrustrum::VF_BOTTOM_PLANE],
			planes[ox::scene::SViewFrustrum::VF_RIGHT_PLANE], p);

		return p;
	}


	//! returns a bounding box encosing the whole view frustrum
	inline ox::core::CAabbox3d<float> SViewFrustrum::getBoundingBox() const
	{
		return boundingBox;
	}


	//! recalculates the bounding box member based on the planes
	inline void SViewFrustrum::recalculateBoundingBox()
	{
		ox::core::CAabbox3d<float> box(cameraPosition);

		box.addInternalPoint(getFarLeftUp());
		box.addInternalPoint(getFarRightUp());
		box.addInternalPoint(getFarLeftDown());
		box.addInternalPoint(getFarRightDown());

		boundingBox = box;
	}


	//! This constructor creates a view frustrum based on a projection and/or
	//! view matrix.
	inline SViewFrustrum::SViewFrustrum(const ox::core::CMatrix4& mat)
	{
		#define sw(a,b)		(mat((b),(a)))

		// left clipping plane
		planes[SViewFrustrum::VF_LEFT_PLANE].Normal.X = -(sw(0,3) + sw(0,0));
		planes[SViewFrustrum::VF_LEFT_PLANE].Normal.Y = -(sw(1,3) + sw(1,0));
		planes[SViewFrustrum::VF_LEFT_PLANE].Normal.Z = -(sw(2,3) + sw(2,0));
		planes[SViewFrustrum::VF_LEFT_PLANE].D =		  -(sw(3,3) + sw(3,0));
	                                                 
		// right clipping plane                  
		planes[SViewFrustrum::VF_RIGHT_PLANE].Normal.X = -(sw(0,3) - sw(0,0));
		planes[SViewFrustrum::VF_RIGHT_PLANE].Normal.Y = -(sw(1,3) - sw(1,0));
		planes[SViewFrustrum::VF_RIGHT_PLANE].Normal.Z = -(sw(2,3) - sw(2,0));
		planes[SViewFrustrum::VF_RIGHT_PLANE].D =        -(sw(3,3) - sw(3,0));
	    
		// top clipping plane                  
		planes[SViewFrustrum::VF_TOP_PLANE].Normal.X = -(sw(0,3) - sw(0,1));
		planes[SViewFrustrum::VF_TOP_PLANE].Normal.Y = -(sw(1,3) - sw(1,1));
		planes[SViewFrustrum::VF_TOP_PLANE].Normal.Z = -(sw(2,3) - sw(2,1));
		planes[SViewFrustrum::VF_TOP_PLANE].D =        -(sw(3,3) - sw(3,1));

		// bottom clipping plane                  
		planes[SViewFrustrum::VF_BOTTOM_PLANE].Normal.X = -(sw(0,3) + sw(0,1));
		planes[SViewFrustrum::VF_BOTTOM_PLANE].Normal.Y = -(sw(1,3) + sw(1,1));
		planes[SViewFrustrum::VF_BOTTOM_PLANE].Normal.Z = -(sw(2,3) + sw(2,1));
		planes[SViewFrustrum::VF_BOTTOM_PLANE].D =        -(sw(3,3) + sw(3,1));
	 
		// near clipping plane                   
		planes[SViewFrustrum::VF_NEAR_PLANE].Normal.X = -sw(0,2);
		planes[SViewFrustrum::VF_NEAR_PLANE].Normal.Y = -sw(1,2);
		planes[SViewFrustrum::VF_NEAR_PLANE].Normal.Z = -sw(2,2);
		planes[SViewFrustrum::VF_NEAR_PLANE].D =        -sw(3,2);
	                                                 
		// far clipping plane                    
		planes[SViewFrustrum::VF_FAR_PLANE].Normal.X = -(sw(0,3) - sw(0,2));
		planes[SViewFrustrum::VF_FAR_PLANE].Normal.Y = -(sw(1,3) - sw(1,2));
		planes[SViewFrustrum::VF_FAR_PLANE].Normal.Z = -(sw(2,3) - sw(2,2));
		planes[SViewFrustrum::VF_FAR_PLANE].D =        -(sw(3,3) - sw(3,2));

		// normalize normals
		
		for (int i=0; i<6; ++i)
		{
			float len = (float)(1.0f / planes[i].Normal.getLength());
			planes[i].Normal *= len;
			planes[i].D *= len;
		}

		// make bounding box

		recalculateBoundingBox();
	}


} // end namespace scene
} // end namespace ox

#endif

