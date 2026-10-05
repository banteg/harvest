// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMeshViewer.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#ifndef DAISY_GUI_CGUIMESHVIEWER_H
#define DAISY_GUI_CGUIMESHVIEWER_H

#include "ox/gui/IGUIMeshViewer.h"
#include "ox/video/SMaterial.h"

namespace daisy {
namespace gui {

class CGUIMeshViewer : public ox::gui::IGUIMeshViewer
{
public:
    //! constructor
    CGUIMeshViewer(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
        ox::core::CRect<int> rectangle);

    //! destructor
    ~CGUIMeshViewer();

    //! sets the mesh to be shown
    virtual void setMesh(ox::scene::IAnimatedMesh* mesh);

    //! sets the material
    virtual void setMaterial(const ox::video::SMaterial& material);

    //! gets the material
    virtual const ox::video::SMaterial& getMaterial();

    //! called if an event happened.
    virtual bool OnEvent(const ox::event::SEvent& event);

    //! draws the element and its children
    virtual void draw();

private:
    ox::video::SMaterial Material;
    ox::scene::IAnimatedMesh* Mesh;
};

} // end namespace gui
} // end namespace daisy

#endif
