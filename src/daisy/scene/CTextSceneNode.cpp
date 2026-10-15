#include <iostream>
#include "ox/core/CAabbox3d.h"
#include "ox/core/CDimension2d.h"
#include "ox/core/CPosition2d.h"
#include "ox/core/CRect.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/ISceneCollisionManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/scene/ITextSceneNode.h"
#include "ox/gui/IGUIFont.h"
#include "CTextSceneNode.h"
#include "ox/scene/ISceneManager.h"


namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

//! constructor
CTextSceneNode::CTextSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
							   ox::gui::IGUIFont* font, ox::scene::ISceneCollisionManager* coll,
	const ox::core::CVector3d<float>& position, const wchar_t* text,
	ox::video::SColor color)
	: ITextSceneNode(parent, mgr, id, position), Font(font), Coll(coll), Color(color)
{
	Text = text;
	setAutomaticCulling(false);

	if (Font)
		Font->grab();
}

//! destructor
CTextSceneNode::~CTextSceneNode()
{
	if (Font)
		Font->drop();
}

void CTextSceneNode::OnPreRender()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, SNRT_SHADOW);

	ISceneNode::OnPreRender();
}

//! renders the node.
void CTextSceneNode::render()
{
    // Not recovered: Irrlicht 0.7's body does not compile against daisy's interfaces.
    __builtin_trap();
}


//! returns the axis aligned bounding box of this node
const ox::core::CAabbox3d<float>& CTextSceneNode::getBoundingBox() const
{
	return Box;
}

//! returns amount of materials used by this scene node.
int CTextSceneNode::getMaterialCount()
{
	return 0;
}

//! sets the text string
void CTextSceneNode::setText(const wchar_t* text)
{
	Text = text;
}

//! sets the color of the text
void CTextSceneNode::setTextColor(ox::video::SColor color)
{
	Color = color;
}


} // end namespace scene
} // end namespace daisy
