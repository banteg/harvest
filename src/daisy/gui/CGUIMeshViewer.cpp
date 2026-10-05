// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/CGUIMeshViewer.cpp for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.

#include "CGUIMeshViewer.h"
#include "ox/gui/IGUIElementInline.h"
#include "daisy/os.h"
#include "ox/core/CMatrix4.h"
#include "ox/gui/IGUIEnvironment.h"
#include "ox/gui/IGUISkin.h"
#include "ox/scene/IAnimatedMesh.h"
#include "ox/scene/IMesh.h"
#include "ox/scene/IMeshBuffer.h"
#include "ox/video/IVideoDriver.h"
// The object has an iostream initializer; the original including header is unidentified.
#include <iostream>

namespace daisy {
namespace gui {

//! Irrlicht's world transformation state; IVideoDriver.h does not name the states.
static const ox::video::E_TRANSFORMATION_STATE ETS_WORLD = (ox::video::E_TRANSFORMATION_STATE)1;

//! constructor
CGUIMeshViewer::CGUIMeshViewer(ox::gui::IGUIEnvironment* environment, ox::gui::IGUIElement* parent, int id,
    ox::core::CRect<int> rectangle)
    : IGUIMeshViewer(environment, parent, id, rectangle), Mesh(0)
{
}

//! destructor
CGUIMeshViewer::~CGUIMeshViewer()
{
    if (Mesh)
        Mesh->drop();
}

//! sets the mesh to be shown
void CGUIMeshViewer::setMesh(ox::scene::IAnimatedMesh* mesh)
{
    if (Mesh)
        Mesh->drop();

    Mesh = mesh;
    if (!Mesh)
        return;

    // Irrlicht computed the center of the first frame's box here and never used it.
    if (mesh->getFrameCount())
        mesh->getMesh(0)->getBoundingBox();

    if (Mesh)
        Mesh->grab();
}

//! sets the material
void CGUIMeshViewer::setMaterial(const ox::video::SMaterial& material)
{
    Material = material;
}

//! gets the material
const ox::video::SMaterial& CGUIMeshViewer::getMaterial()
{
    return Material;
}

//! called if an event happened.
bool CGUIMeshViewer::OnEvent(const ox::event::SEvent& event)
{
    return Parent ? Parent->OnEvent(event) : false;
}

//! Clips the rectangle against another one, as Irrlicht's rect::clipAgainst does.
static inline void clipAgainst(ox::core::CRect<int>& rect, const ox::core::CRect<int>& other)
{
    if (other.LowerRightCorner.X < rect.LowerRightCorner.X)
        rect.LowerRightCorner.X = other.LowerRightCorner.X;
    if (other.LowerRightCorner.Y < rect.LowerRightCorner.Y)
        rect.LowerRightCorner.Y = other.LowerRightCorner.Y;
    if (other.UpperLeftCorner.X > rect.UpperLeftCorner.X)
        rect.UpperLeftCorner.X = other.UpperLeftCorner.X;
    if (other.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
        rect.UpperLeftCorner.Y = other.UpperLeftCorner.Y;
}

//! draws the element and its children
void CGUIMeshViewer::draw()
{
    if (!IsVisible)
        return;

    ox::gui::IGUISkin* skin = Environment->getSkin();
    ox::video::IVideoDriver* driver = Environment->getVideoDriver();
    ox::core::CRect<int> viewPort = AbsoluteRect;
    viewPort.LowerRightCorner.X -= 1;
    viewPort.LowerRightCorner.Y -= 1;
    viewPort.UpperLeftCorner.X += 1;
    viewPort.UpperLeftCorner.Y += 1;

    clipAgainst(viewPort, AbsoluteClippingRect);

    // draw the frame

    ox::core::CRect<int> frameRect(AbsoluteRect);
    frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + 1;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

    frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + 1;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

    frameRect = AbsoluteRect;
    frameRect.UpperLeftCorner.X = frameRect.LowerRightCorner.X - 1;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

    frameRect = AbsoluteRect;
    frameRect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
    frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
    driver->draw2DRectangle(skin->getColor(ox::gui::EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

    // draw the mesh

    if (Mesh)
    {
        ox::core::CRect<int> oldViewPort = driver->getViewPort();

        driver->setViewPort(viewPort);

        ox::core::CMatrix4 mat;

        mat.makeIdentity();
        mat.setTranslation(ox::core::CVector3d<float>(0, 0, 0));
        driver->setTransform(ETS_WORLD, mat);

        driver->setMaterial(Material);

        ox::scene::IMesh* m = Mesh->getMesh(os::Timer::getTime() / 20);
        for (int i = 0; i < m->getMeshBufferCount(); ++i)
        {
            ox::scene::IMeshBuffer* mb = m->getMeshBuffer(i);

            switch (mb->getVertexType())
            {
            case ox::video::EVT_STANDARD:
                driver->drawIndexedTriangleList((ox::video::S3DVertex*)mb->getVertices(), mb->getVertexCount(),
                    mb->getIndices(), mb->getIndexCount() / 3);
                break;
            case ox::video::EVT_2TCOORDS:
                driver->drawIndexedTriangleList((ox::video::S3DVertex2TCoords*)mb->getVertices(),
                    mb->getVertexCount(), mb->getIndices(), mb->getIndexCount() / 3);
                break;
            default:
                break;
            }
        }

        driver->setViewPort(oldViewPort);
    }

    IGUIElement::draw();
}

} // end namespace gui
} // end namespace daisy
