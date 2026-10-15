// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CMetaTriangleSelector.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CLine3d.h"
#include "ox/core/CMatrix4.h"
#include "ox/core/CTriangle3d.h"
#include "ox/scene/ITriangleSelector.h"
#include "CMetaTriangleSelector.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CMetaTriangleSelector::CMetaTriangleSelector()
{
}

//! destructor
CMetaTriangleSelector::~CMetaTriangleSelector()
{
	removeAllTriangleSelectors();
}



//! Returns amount of all available triangles in this selector
int CMetaTriangleSelector::getTriangleCount() const
{
	int count = 0;
	for (int i=0; i<(int)TriangleSelectors.size(); ++i)
		count += TriangleSelectors[i]->getTriangleCount();

	return count;
}



//! Gets all triangles.
void CMetaTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
	int& outTriangleCount, const ox::core::CMatrix4* transform)
{
	int outWritten = 0;

	for (int i=0; i<(int)TriangleSelectors.size(); ++i)
	{
		int t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten, arraySize - outWritten, t, transform);
		outWritten += t;
	}

    outTriangleCount = outWritten;
}



//! Gets all triangles which lie within a specific bounding box.
void CMetaTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
	int& outTriangleCount, const ox::core::CAabbox3d<float>& box,
	const ox::core::CMatrix4* transform)
{
	int outWritten = 0;

	for (int i=0; i<(int)TriangleSelectors.size(); ++i)
	{
		int t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten, arraySize - outWritten, t,
			box, transform);
		outWritten += t;
	}

    outTriangleCount = outWritten;
}



//! Gets all triangles which have or may have contact with a 3d line.
void CMetaTriangleSelector::getTriangles(ox::core::CTriangle3d<float>* triangles, int arraySize,
	int& outTriangleCount, const ox::core::CLine3d<float>& line,
	const ox::core::CMatrix4* transform)
{
	int outWritten = 0;

	for (int i=0; i<(int)TriangleSelectors.size(); ++i)
	{
		int t = 0;
		TriangleSelectors[i]->getTriangles(triangles + outWritten, arraySize - outWritten, t,
			line, transform);
		outWritten += t;
	}

    outTriangleCount = outWritten;
}



//! Adds a triangle selector to the collection of triangle selectors
//! in this metaTriangleSelector.
void CMetaTriangleSelector::addTriangleSelector(ITriangleSelector* toAdd)
{
	if (!toAdd)
		return;

	TriangleSelectors.push_back(toAdd);
	toAdd->grab();
}



//! Removes a specific triangle selector which was added before	from the collection.
bool CMetaTriangleSelector::removeTriangleSelector(ITriangleSelector* toRemove)
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! Removes all triangle selectors from the collection.
void CMetaTriangleSelector::removeAllTriangleSelectors()
{
	for (int i=0; i<(int)TriangleSelectors.size(); ++i)
		TriangleSelectors[i]->drop();

	TriangleSelectors.clear();
}



} // end namespace scene
} // end namespace daisy
