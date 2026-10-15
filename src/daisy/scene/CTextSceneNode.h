// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CTextSceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#ifndef __C_TEXT_SCENE_NODE_H_INCLUDED__
#define __C_TEXT_SCENE_NODE_H_INCLUDED__

#include "ox/scene/ITextSceneNode.h"
#include "ox/core/CAabbox3d.h"
#include "ox/core/CString.h"
#include "ox/core/CVector3d.h"
#include "ox/video/SColor.h"
#include "ox/scene/ISceneManager.h"
#include "ox/scene/ISceneNode.h"
#include "ox/gui/IGUIFont.h"
#include "ox/scene/ISceneCollisionManager.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;

	class CTextSceneNode : public ITextSceneNode
	{
	public:

		//! constructor
		CTextSceneNode(ISceneNode* parent, ISceneManager* mgr, int id,
			ox::gui::IGUIFont* font, ox::scene::ISceneCollisionManager* coll,
			const ox::core::CVector3d<float>& position = ox::core::CVector3d<float>(0,0,0), const wchar_t* text=0,
			ox::video::SColor color=ox::video::SColor(100,0,0,0));

		//! destructor
		virtual ~CTextSceneNode();

		virtual void OnPreRender();

		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const ox::core::CAabbox3d<float>& getBoundingBox() const;

		//! returns amount of materials used by this scene node.
		virtual int getMaterialCount();

		//! sets the text string
		virtual void setText(const wchar_t* text);

		//! sets the color of the text
		virtual void setTextColor(ox::video::SColor color);

	private:

		ox::core::CAabbox3d<float> Box;
		ox::core::CString<wchar_t> Text;
		ox::video::SColor Color;
		ox::gui::IGUIFont* Font;
		ox::scene::ISceneCollisionManager* Coll;
	};

} // end namespace scene
} // end namespace daisy

#endif
