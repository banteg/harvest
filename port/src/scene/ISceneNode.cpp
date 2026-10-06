// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// The port's definitions of ox::scene::ISceneNode, whose bodies are inline in Irrlicht and emitted
// as copies in the Mac 1.18 build (0x112f5e-0x113f2c).
//
// daisy's one change: OnPreRender updates the absolute transformation of a visible node before
// its children register (Mac 0x1136dc), so a position set during the frame takes effect in the
// same drawAll. Irrlicht 0.7 updates it only in OnPostRender.

#include "ox/scene/ISceneNode.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "scene/SceneMath.h"

namespace ox {
namespace scene {

ISceneNode::ISceneNode(ISceneNode* parent, ISceneManager* mgr, int id, const core::CVector3d<float>& position,
    const core::CVector3d<float>& rotation, const core::CVector3d<float>& scale)
    : RelativeTranslation(position), RelativeRotation(rotation), RelativeScale(scale), Parent(0), ID(id),
      SceneManager(mgr), TriangleSelector(0), AutomaticCullingEnabled(true), DebugDataVisible(false),
      IsVisible(true)
{
    if (parent)
        parent->addChild(this);
    Parent = parent;

    updateAbsolutePosition();
}

ISceneNode::~ISceneNode()
{
    for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->drop();

    for (TList<ISceneNodeAnimator*>::iterator it = Animators.begin(); it != Animators.end(); ++it)
        (*it)->drop();
}

core::CAabbox3d<float> ISceneNode::getTransformedBoundingBox()
{
    core::CAabbox3d<float> box = getBoundingBox();
    daisy::scene::transformBox(AbsoluteTransformation, box);
    return box;
}

void ISceneNode::OnPreRender()
{
    if (!IsVisible)
        return;

    updateAbsolutePosition();

    for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->OnPreRender();
}

void ISceneNode::OnPostRender(unsigned int timeMs)
{
    if (!IsVisible)
        return;

    for (TList<ISceneNodeAnimator*>::iterator it = Animators.begin(); it != Animators.end(); ++it)
        (*it)->animateNode(this, timeMs);

    updateAbsolutePosition();

    for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->OnPostRender(timeMs);
}

const wchar_t* ISceneNode::getName() const
{
    return Name.c_str();
}

void ISceneNode::setName(const wchar_t* name)
{
    Name = name;
}

core::CMatrix4 ISceneNode::getRelativeTransformation() const
{
    core::CMatrix4 mat;
    daisy::scene::setRotationDegrees(mat, RelativeRotation);
    mat.setTranslation(RelativeTranslation);

    if (RelativeScale != core::CVector3d<float>(1, 1, 1))
    {
        core::CMatrix4 smat;
        daisy::scene::setScale(smat, RelativeScale);
        mat *= smat;
    }

    return mat;
}

bool ISceneNode::isVisible()
{
    return IsVisible;
}

void ISceneNode::setVisible(bool visible)
{
    IsVisible = visible;
}

int ISceneNode::getID()
{
    return ID;
}

void ISceneNode::setID(int id)
{
    ID = id;
}

void ISceneNode::addChild(ISceneNode* child)
{
    if (!child)
        return;

    child->grab();
    child->remove();
    Children.push_back(child);
    child->Parent = this;
}

bool ISceneNode::removeChild(ISceneNode* child)
{
    for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        if (*it == child)
        {
            (*it)->drop();
            Children.erase(it);
            return true;
        }

    return false;
}

void ISceneNode::removeAll()
{
    for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
        (*it)->drop();

    Children.clear();
}

void ISceneNode::remove()
{
    if (Parent)
        Parent->removeChild(this);
}

void ISceneNode::addAnimator(ISceneNodeAnimator* animator)
{
    if (!animator)
        return;

    Animators.push_back(animator);
    animator->grab();
}

void ISceneNode::removeAnimator(ISceneNodeAnimator* animator)
{
    for (TList<ISceneNodeAnimator*>::iterator it = Animators.begin(); it != Animators.end(); ++it)
        if (*it == animator)
        {
            (*it)->drop();
            Animators.erase(it);
            return;
        }
}

void ISceneNode::removeAnimators()
{
    for (TList<ISceneNodeAnimator*>::iterator it = Animators.begin(); it != Animators.end(); ++it)
        (*it)->drop();

    Animators.clear();
}

//! Nodes without materials report a count of 0; the original returns a null reference here, which
//! every caller avoids by checking the count first.
video::SMaterial& ISceneNode::getMaterial(int i)
{
    static video::SMaterial none;
    return none;
}

int ISceneNode::getMaterialCount()
{
    return 0;
}

core::CVector3d<float> ISceneNode::getScale() const
{
    return RelativeScale;
}

void ISceneNode::setScale(const core::CVector3d<float>& scale)
{
    RelativeScale = scale;
}

const core::CVector3d<float> ISceneNode::getRotation() const
{
    return RelativeRotation;
}

void ISceneNode::setRotation(const core::CVector3d<float>& rotation)
{
    RelativeRotation = rotation;
}

const core::CVector3d<float> ISceneNode::getPosition() const
{
    return RelativeTranslation;
}

void ISceneNode::setPosition(const core::CVector3d<float>& position)
{
    RelativeTranslation = position;
}

core::CVector3d<float> ISceneNode::getAbsolutePosition() const
{
    core::CVector3d<float> pos(0.0f, 0.0f, 0.0f);
    daisy::scene::transformVect(AbsoluteTransformation, pos);
    return pos;
}

void ISceneNode::setParent(ISceneNode* parent)
{
    grab();
    remove();

    Parent = parent;

    if (Parent)
        Parent->addChild(this);

    drop();
}

ITriangleSelector* ISceneNode::getTriangleSelector() const
{
    return TriangleSelector;
}

//! Triangle selectors are not part of the port (the game never creates one), so the selector is
//! only stored.
void ISceneNode::setTriangleSelector(ITriangleSelector* selector)
{
    TriangleSelector = selector;
}

void ISceneNode::updateAbsolutePosition()
{
    if (Parent)
        AbsoluteTransformation = Parent->getAbsoluteTransformation() * getRelativeTransformation();
    else
        AbsoluteTransformation = getRelativeTransformation();
}

} // end namespace scene
} // end namespace ox
