// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. The virtual order follows
// the Mac 1.18 vtables of the daisy::scene nodes, and the fields are Irrlicht 0.7's in the order the
// Mac build's ISceneNode constructor (0x112f5e) stores them. The inline bodies are Irrlicht 0.7's,
// with daisy's two changes: OnPreRender updates the absolute transformation of a visible node
// before its children register (Mac 0x1136dc), where Irrlicht updates it only in OnPostRender, and
// addChild does not check its argument (Linux 0x4c72c0).

#ifndef OX_SCENE_ISCENENODE_H
#define OX_SCENE_ISCENENODE_H

#include "../IUnknown.h"
#include "../core/CMatrix4.h"
#include "../core/CString.h"
#include "../core/CVector3d.h"
#include "../video/SMaterial.h"
#include "../TList.h"
#include "ox/scene/ISceneNodeAnimator.h"
#include "ox/core/CAabbox3d.h"
#include "ox/scene/ITriangleSelector.h"

namespace ox {
namespace core {
template <class T> class CAabbox3d;
} // end namespace core
namespace scene {

class ISceneNodeAnimator;
class ITriangleSelector;
class ISceneManager;

//! A node of the scene graph.
class ISceneNode : public IUnknown
{
public:
    ISceneNode(ISceneNode* parent, ISceneManager* mgr, int id = -1,
        const core::CVector3d<float>& position = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& rotation = core::CVector3d<float>(0, 0, 0),
        const core::CVector3d<float>& scale = core::CVector3d<float>(1.0f, 1.0f, 1.0f))
        : RelativeTranslation(position), RelativeRotation(rotation), RelativeScale(scale), Parent(0), ID(id),
          SceneManager(mgr), TriangleSelector(0), AutomaticCullingEnabled(true), DebugDataVisible(false),
          IsVisible(true)
    {
        if (parent)
            parent->addChild(this);
        Parent = parent;

        updateAbsolutePosition();
    }
    virtual ~ISceneNode()
    {
        // delete all children
        ox::TList<ISceneNode*>::iterator it = Children.begin();
        for (; it != Children.end(); ++it)
            (*it)->drop();

        // delete all animators
        ox::TList<ISceneNodeAnimator*>::iterator ait = Animators.begin();
            for (; ait != Animators.end(); ++ait)
                (*ait)->drop();

        if (TriangleSelector)
            TriangleSelector->drop();
    }

    //! The bounding box transformed by the absolute transformation, re-boxed.
    core::CAabbox3d<float> getTransformedBoundingBox()
    {
        core::CAabbox3d<float> box = getBoundingBox();
        AbsoluteTransformation.transformBox(box);
        return box;
    }

    const TList<ISceneNode*>& getChildren() const
    {
        return Children;
    }

    void setAutomaticCulling(bool enabled)
    {
        AutomaticCullingEnabled = enabled;
    }
    virtual void OnPreRender()
    {
        if (!IsVisible)
            return;

        updateAbsolutePosition();

        for (TList<ISceneNode*>::iterator it = Children.begin(); it != Children.end(); ++it)
            (*it)->OnPreRender();
    }
    virtual void OnPostRender(unsigned int timeMs)
    {
        if (IsVisible)
        {
            // animate this node with all animators

            ox::TList<ISceneNodeAnimator*>::iterator ait = Animators.begin();
            for (; ait != Animators.end(); ++ait)
                (*ait)->animateNode(this, timeMs);

            // update absolute position
            updateAbsolutePosition();

            // perform the post render process on all children

            ox::TList<ISceneNode*>::iterator it = Children.begin();
            for (; it != Children.end(); ++it)
                (*it)->OnPostRender(timeMs);
        }
    }
    virtual void render() = 0;
    virtual const wchar_t* getName() const
    {
        return Name.c_str();
    }
    virtual void setName(const wchar_t* name)
    {
        Name = name;
    }
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
    virtual core::CMatrix4 getRelativeTransformation() const
    {
        ox::core::CMatrix4 mat;
        mat.setRotationDegrees(RelativeRotation);
        mat.setTranslation(RelativeTranslation);

        if (RelativeScale != ox::core::CVector3d<float>(1,1,1))
        {
            ox::core::CMatrix4 smat;
            smat.setScale(RelativeScale);
            mat *= smat;
        }

        return mat;
    }
    virtual bool isVisible()
    {
        return IsVisible;
    }
    virtual void setVisible(bool visible)
    {
        IsVisible = visible;
    }
    virtual int getID()
    {
        return ID;
    }
    virtual void setID(int id)
    {
        ID = id;
    }
    virtual void addChild(ISceneNode* child)
    {
        child->grab();
        child->remove(); // remove from old parent
        Children.push_back(child);
        child->Parent = this;
    }
    virtual bool removeChild(ISceneNode* child)
    {
        ox::TList<ISceneNode*>::iterator it = Children.begin();
        for (; it != Children.end(); ++it)
            if ((*it) == child)
            {
                (*it)->drop();
                Children.erase(it);
                return true;
            }

        return false;
    }
    virtual void removeAll()
    {
        ox::TList<ISceneNode*>::iterator it = Children.begin();
        for (; it != Children.end(); ++it)
            (*it)->drop();

        Children.clear();
    }
    virtual void remove()
    {
        if (Parent)
            Parent->removeChild(this);
    }
    virtual void addAnimator(ISceneNodeAnimator* animator)
    {
        if (animator)
        {
            Animators.push_back(animator);
            animator->grab();
        }
    }
    virtual void removeAnimator(ISceneNodeAnimator* animator)
    {
        ox::TList<ISceneNodeAnimator*>::iterator it = Animators.begin();
        for (; it != Animators.end(); ++it)
            if ((*it) == animator)
            {
                (*it)->drop();
                Animators.erase(it);
                return;
            }
    }
    virtual void removeAnimators()
    {
        ox::TList<ISceneNodeAnimator*>::iterator it = Animators.begin();
        for (; it != Animators.end(); ++it)
            (*it)->drop();

        Animators.clear();    
    }
    virtual video::SMaterial& getMaterial(int i)
    {
        return *((video::SMaterial*)0);
    }
    virtual int getMaterialCount()
    {
        return 0;
    }
    virtual core::CVector3d<float> getScale() const
    {
        return RelativeScale;
    }
    virtual void setScale(const core::CVector3d<float>& scale)
    {
        RelativeScale = scale;
    }
    virtual const core::CVector3d<float> getRotation() const
    {
        return RelativeRotation;
    }
    virtual void setRotation(const core::CVector3d<float>& rotation)
    {
        RelativeRotation = rotation;
    }
    virtual const core::CVector3d<float> getPosition() const
    {
        return RelativeTranslation;
    }
    virtual void setPosition(const core::CVector3d<float>& position)
    {
        RelativeTranslation = position;
    }
    virtual core::CVector3d<float> getAbsolutePosition() const
    {
        ox::core::CVector3d<float> pos(0.0f, 0.0f, 0.0f);
        AbsoluteTransformation.transformVect(pos);
        return pos;
    }
    virtual void setParent(ISceneNode* parent)
    {
        grab();
        remove();

        Parent = parent;

        if (Parent)
            Parent->addChild(this);

        drop();
    }
    virtual ITriangleSelector* getTriangleSelector() const
    {
        return TriangleSelector;
    }
    virtual void setTriangleSelector(ITriangleSelector* selector)

    {
        if (TriangleSelector)
            TriangleSelector->drop();

        TriangleSelector = selector;
        TriangleSelector->grab();
    }
    virtual void updateAbsolutePosition()
    {
        if (Parent)
            AbsoluteTransformation = 
                Parent->getAbsoluteTransformation() * getRelativeTransformation();
        else
            AbsoluteTransformation = getRelativeTransformation();
    }

    //! Sets a texture of all materials of this node.
    void setMaterialTexture(int textureLayer, video::ITexture* texture)
    {
        if (textureLayer < 0 || textureLayer >= video::MATERIAL_MAX_TEXTURES)
            return;

        for (int i = 0; i < getMaterialCount(); ++i)
            getMaterial(i).Textures[textureLayer] = texture;
    }

    //! Sets the material type of all materials of this node.
    void setMaterialType(video::E_MATERIAL_TYPE newType)
    {
        for (int i = 0; i < getMaterialCount(); ++i)
            getMaterial(i).MaterialType = newType;
    }

    bool getAutomaticCulling() const
    {
        return AutomaticCullingEnabled;
    }

    core::CMatrix4& getAbsoluteTransformation()
    {
        return AbsoluteTransformation;
    }

protected:
    core::CString<wchar_t> Name;
    core::CMatrix4 AbsoluteTransformation;
    core::CVector3d<float> RelativeTranslation;
    //! In degrees.
    core::CVector3d<float> RelativeRotation;
    core::CVector3d<float> RelativeScale;
    ISceneNode* Parent;
    TList<ISceneNode*> Children;
    TList<ISceneNodeAnimator*> Animators;
    int ID;
    ISceneManager* SceneManager;
    ITriangleSelector* TriangleSelector;
    bool AutomaticCullingEnabled;
    bool DebugDataVisible;
    bool IsVisible;
};

} // end namespace scene
} // end namespace ox

#endif
