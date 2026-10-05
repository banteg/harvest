// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneNodeAnimator.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source.

#ifndef OX_SCENE_ISCENENODEANIMATOR_H
#define OX_SCENE_ISCENENODEANIMATOR_H

#include "../IUnknown.h"

namespace ox {
namespace scene {

class ISceneNode;

//! Animates a scene node, for example by rotating it.
class ISceneNodeAnimator : public IUnknown
{
public:
    virtual void animateNode(ISceneNode* node, unsigned int timeMs) = 0;
};

} // end namespace scene
} // end namespace ox

#endif
