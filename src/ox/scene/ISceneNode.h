// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/ISceneNode.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::scene namespace; not the original source. Partial: the virtual order
// follows the Mac 1.18 vtables of the daisy::scene nodes, and only the members up to the culling
// flag's Linux amd64 offset (0xd0) are laid out; the inline virtual bodies are not recovered.

#ifndef OX_SCENE_ISCENENODE_H
#define OX_SCENE_ISCENENODE_H

#include "../IUnknown.h"
#include "../core/CMatrix4.h"
#include "../core/CString.h"
#include "../core/CVector3d.h"
#include "../video/SMaterial.h"

namespace ox {
namespace core {
template <class T> class CAabbox3d;
} // end namespace core
namespace scene {

class ISceneNodeAnimator;
class ITriangleSelector;

//! A node of the scene graph.
class ISceneNode : public IUnknown
{
public:
    virtual void OnPreRender();
    virtual void OnPostRender(unsigned int timeMs);
    virtual void render() = 0;
    virtual const wchar_t* getName() const;
    virtual void setName(const wchar_t* name);
    virtual const core::CAabbox3d<float>& getBoundingBox() const = 0;
    virtual core::CMatrix4 getRelativeTransformation() const;
    virtual bool isVisible();
    virtual void setVisible(bool visible);
    virtual int getID();
    virtual void setID(int id);
    virtual void addChild(ISceneNode* child);
    virtual bool removeChild(ISceneNode* child);
    virtual void removeAll();
    virtual void remove();
    virtual void addAnimator(ISceneNodeAnimator* animator);
    virtual void removeAnimator(ISceneNodeAnimator* animator);
    virtual void removeAnimators();
    virtual video::SMaterial& getMaterial(int i);
    virtual int getMaterialCount();
    virtual core::CVector3d<float> getScale() const;
    virtual void setScale(const core::CVector3d<float>& scale);
    virtual const core::CVector3d<float> getRotation() const;
    virtual void setRotation(const core::CVector3d<float>& rotation);
    virtual const core::CVector3d<float> getPosition() const;
    virtual void setPosition(const core::CVector3d<float>& position);
    virtual core::CVector3d<float> getAbsolutePosition() const;
    virtual void setParent(ISceneNode* parent);
    virtual ITriangleSelector* getTriangleSelector() const;
    virtual void setTriangleSelector(ITriangleSelector* selector);
    virtual void updateAbsolutePosition();

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
    // Not recovered yet: the relative transformation, parent, children, animators, id, scene manager
    // and triangle selector.
    char Unrecovered[0xd0 - sizeof(IUnknown) - sizeof(core::CString<wchar_t>) - sizeof(core::CMatrix4)];
    bool AutomaticCullingEnabled;
    bool DebugDataVisible;
    bool IsVisible;
};

} // end namespace scene
} // end namespace ox

#endif
