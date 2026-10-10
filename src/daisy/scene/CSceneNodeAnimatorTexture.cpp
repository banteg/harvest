// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CSceneNodeAnimatorTexture.cpp (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's daisy namespace; not the original source.

#include <iostream>
#include "CSceneNodeAnimatorTexture.h"
#include "ox/video/ITexture.h"
#include "ox/scene/ISceneNode.h"
#include "ox/TArray.h"

namespace ox { namespace scene {} }
namespace daisy
{
namespace scene
{
using namespace ox::scene;


//! constructor
CSceneNodeAnimatorTexture::CSceneNodeAnimatorTexture(const ox::TArray<ox::video::ITexture*>& textures, 
													 int timePerFrame, bool loop, unsigned int now)
: Loop(loop), StartTime(now), TimePerFrame(timePerFrame)
{

	for (unsigned int i=0; i<textures.size(); ++i)
	{
		if (textures[i])
			textures[i]->grab();

		Textures.push_back(textures[i]);
	}

	EndTime = now + (timePerFrame * Textures.size());
}



//! destructor
CSceneNodeAnimatorTexture::~CSceneNodeAnimatorTexture()
{
	for (unsigned int i=0; i<Textures.size(); ++i)
		if (Textures[i])
			Textures[i]->drop();
}



//! animates a scene node
void CSceneNodeAnimatorTexture::animateNode(ISceneNode* node, unsigned int timeMs)
{
	unsigned int t = (timeMs-StartTime);

	int idx = 0;

	if (!Loop && timeMs >= EndTime)
		idx = Textures.size() - 1;
	else
		idx = (t/TimePerFrame) % Textures.size();

	if (idx < (int)Textures.size())
		node->setMaterialTexture(0, Textures[idx]);
}



} // end namespace scene
} // end namespace daisy
