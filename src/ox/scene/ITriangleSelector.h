// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ITriangleSelector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef __I_TRIANGLE_SELECTOR_H_INCLUDED__
#define __I_TRIANGLE_SELECTOR_H_INCLUDED__

#include "ox/IUnknown.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CTriangle3d.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CMatrix4.h"

namespace ox
{
namespace scene
{

//! Interface to return triangles with specific properties.
/** Every ISceneNode may have a triangle selector, available with
 ISceneNode::getTriangleScelector() or ISceneManager::createTriangleSelector.
 This is used for doing collision detection: For example if you know, that a collision may happened in the
 area between (1,1,1) and (10,10,10), you can get all triangles of the
 scene node in this area with the ITriangleSelector easily and check every triangle
 if it collided. */
class ITriangleSelector : public ox::IUnknown
{
public:

	//! destructor
	virtual ~ITriangleSelector() {};

	//! Returns amount of all available triangles in this selector
	virtual int getTriangleCount() const = 0;

	//! Gets all triangles.
	//! \param triangles: Array where the resulting triangles will be written to.
	//! \param arraySize: Size of the target array.
	//! \param outTriangleCount: Amount of triangles which have been written into the
	//! array.
	//! \param transform: Pointer to matrix for transforming the triangles before.
	//! they are returned. Useful for example to scale all triangles down into an
	//! ellipsoid space. If this pointer is null, no transformation will be done.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CMatrix4* transform=0) = 0;

	//! Gets all triangles which lie within a specific bounding box.
	//! Please note that unoptimized triangle selectors also may return triangles,
	//! which are not in the specific box at all.
	//! \param triangles: Array where the resulting triangles will be written to.
	//! \param arraySize: Size of the target array.
	//! \param outTriangleCount: Amount of triangles which have been written into the
	//! array.
	//! \param box: Only triangles which are in this axis aligned bounding box
	//! will be written into the array.
	//! \param transform: Pointer to matrix for transforming the triangles before.
	//! they are returned. Useful for example to scale all triangles down into an
	//! ellipsoid space. If this pointer is null, no transformation will be done.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CAabbox3d<float>& box,
		const ox::core::CMatrix4* transform=0) = 0;

	//! Gets all triangles which have or may have contact with a 3d line.
	//! Please note that unoptimized triangle selectors also may return triangles,
	//! which are not in contact at all with the 3d line.
	//! \param triangles: Array where the resulting triangles will be written to.
	//! \param arraySize: Size of the target array.
	//! \param outTriangleCount: Amount of triangles which have been written into the
	//! array.
	//! \param line: Only triangles which may be in contact with this 3d line will
	//! be written into the array.
	//! \param transform: Pointer to matrix for transforming the triangles before.
	//! they are returned. Useful for example to scale all triangles down into an
	//! ellipsoid space. If this pointer is null, no transformation will be done.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CLine3d<float>& line,
		const ox::core::CMatrix4* transform=0) = 0;
};

} // end namespace scene
} // end namespace ox


#endif
