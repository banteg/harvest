// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMetaTriangleSelector.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_META_TRIANGLE_SELECTOR_H_INCLUDED__
#define __C_META_TRIANGLE_SELECTOR_H_INCLUDED__

#include "ox/scene/IMetaTriangleSelector.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/ITriangleSelector.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! Interface for making multiple triangle selectors work as one big selector.
class CMetaTriangleSelector : public IMetaTriangleSelector
{
public:

	//! constructor
	CMetaTriangleSelector();

	//! destructor
	virtual ~CMetaTriangleSelector();

	//! Returns amount of all available triangles in this selector
	virtual int getTriangleCount() const;

	//! Gets all triangles.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CMatrix4* transform=0);

	//! Gets all triangles which lie within a specific bounding box.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CAabbox3d<float>& box,
		const ox::core::CMatrix4* transform=0);

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
		int& outTriangleCount, const ox::core::CLine3d<float>& line,
		const ox::core::CMatrix4* transform=0);

	//! Adds a triangle selector to the collection of triangle selectors
	//! in this metaTriangleSelector.
	virtual void addTriangleSelector(ITriangleSelector* toAdd);

	//! Removes a specific triangle selector which was added before	from the collection.
	virtual bool removeTriangleSelector(ITriangleSelector* toRemove);

	//! Removes all triangle selectors from the collection.
	virtual void removeAllTriangleSelectors();

private:

	ox::TArray<ITriangleSelector*> TriangleSelectors;
};

} // end namespace scene
} // end namespace daisy


#endif
