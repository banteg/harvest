// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CBspTreeSceneNode.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/scene/IBspTreeSceneNode.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/ISceneNode.h"
#include "ox/event/ILogger.h"
#include "CBspTreeSceneNode.h"
#include "ox/scene/ISceneManager.h"
#include "CBspTree.h"
#include "daisy/os.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CBspTreeSceneNode::CBspTreeSceneNode(ISceneNode* parent, ISceneManager* mgr, int id)
: IBspTreeSceneNode(parent, mgr, id), Tree(0)
{

	os::Printer::log("Warning: The BspTreeNode is not bug free yet. Use the OctTree instead.", ox::event::ELL_ERROR);
}

//! destructor
CBspTreeSceneNode::~CBspTreeSceneNode()
{
	delete Tree;
}


void CBspTreeSceneNode::OnPreRender()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}



//! renders the node.
void CBspTreeSceneNode::render()
{
	
}



//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CBspTreeSceneNode::getBoundingBox() const
{
	return Box;
}



//! creates the tree
bool CBspTreeSceneNode::createTree(IMesh* mesh)
{
	if (Tree)
		delete Tree;

	Tree = 0;

	int count = mesh->getMeshBufferCount();

	if (!count)
		return false;

	Tree = new CBspTree(mesh->getMeshBuffer(0), count);

	return true;
}


} // end namespace scene
} // end namespace daisy

