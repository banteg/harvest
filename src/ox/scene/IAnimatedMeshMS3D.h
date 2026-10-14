// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IAnimatedMeshMS3D.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_ANIMATED_MESH_MS3D_H_INCLUDED__
#define __I_ANIMATED_MESH_MS3D_H_INCLUDED__

#include "IAnimatedMesh.h"
#include "ox/core/CMatrix4.h"

namespace ox
{
namespace scene
{
	//! Interface for using some special functions of MS3D meshes
	class IAnimatedMeshMS3D : public IAnimatedMesh
	{
	public:

		//! Returns a pointer to a transformation matrix of a part of the 
		//! mesh based on a frame time. This is used for being able to attach
		//! objects to parts of animated meshes. For example a weapon to an animated 
		//! hand. 
		//! \param jointNumber: Zero based index of joint. The last joint has the number
		//! getJointCount()-1;
		//! \param frame: Frame of the animation.
		//! \return Returns a pointer to the matrix of the mesh part or 
		//! null if an error occured.
		virtual ox::core::CMatrix4* getMatrixOfJoint(int jointNumber, int frame) = 0;

		//! Gets joint count.
		//! \return Returns amount of joints in the skeletal animated mesh.
		virtual int getJointCount() const = 0;

		//! Gets the name of a joint.
		//! \param number: Zero based index of joint. The last joint has the number
		//! getJointCount()-1;
		//! \return Returns name of joint and null if an error happened.
		virtual const char* getJointName(int number) const = 0;

		//! Gets a joint number from its name
		//! \param name: Name of the joint.
		//! \return Returns the number of the joint or -1 if not found.
		virtual int getJointNumber(const char* name) const = 0;

	};

} // end namespace scene
} // end namespace ox

#endif

