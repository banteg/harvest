// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/IGUIMeshViewer.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::gui namespace; not the original source. The virtual order follows the
// Mac 1.18 vtable of daisy::gui::CGUIMeshViewer.

#ifndef OX_GUI_IGUIMESHVIEWER_H
#define OX_GUI_IGUIMESHVIEWER_H

#include "IGUIElement.h"

namespace ox {
namespace scene { class IAnimatedMesh; }
namespace video { struct SMaterial; }
namespace gui {

//! Shows an animated mesh in a frame.
class IGUIMeshViewer : public IGUIElement
{
public:
    IGUIMeshViewer(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
    }

    //! sets the mesh to be shown
    virtual void setMesh(scene::IAnimatedMesh* mesh) = 0;
    //! sets the material
    virtual void setMaterial(const video::SMaterial& material) = 0;
    //! gets the material
    virtual const video::SMaterial& getMaterial() = 0;
};

} // end namespace gui
} // end namespace ox

#endif
