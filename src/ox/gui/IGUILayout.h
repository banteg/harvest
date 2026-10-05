// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow up to
// updateChildrenForContentArea. The methods are inline; their copies are emitted in
// daisy/gui/CGUIEnvironment.cpp.

#ifndef OX_GUI_IGUILAYOUT_H
#define OX_GUI_IGUILAYOUT_H

#include <vector>
#include "IGUIElement.h"
#include "../TArray.h"

namespace ox {
namespace video { class ISpritePackage; }
namespace gui {

//! An element that arranges its children by their layout hints.
class IGUILayout : public IGUIElement
{
public:
    IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
        : IGUIElement(environment, parent, id, rectangle)
    {
        Type = EGUIET_LAYOUT;
    }

    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip)
    {
        if (IsInvisible && Parent)
            return Parent->getParentAbsoluteClippingRect(clip);

        if (clip)
            return AbsoluteClippingRect;

        // the content area in absolute coordinates, clipped against the parent
        core::CRect<int> rect = getContentArea();
        rect.UpperLeftCorner.X += AbsoluteRect.UpperLeftCorner.X;
        rect.UpperLeftCorner.Y += AbsoluteRect.UpperLeftCorner.Y;
        rect.LowerRightCorner.X += AbsoluteRect.UpperLeftCorner.X;
        rect.LowerRightCorner.Y += AbsoluteRect.UpperLeftCorner.Y;

        if (Parent)
        {
            core::CRect<int> parentClip = Parent->getParentAbsoluteClippingRect(IsFixed);
            if (parentClip.LowerRightCorner.X < rect.LowerRightCorner.X)
                rect.LowerRightCorner.X = parentClip.LowerRightCorner.X;
            if (parentClip.LowerRightCorner.Y < rect.LowerRightCorner.Y)
                rect.LowerRightCorner.Y = parentClip.LowerRightCorner.Y;
            if (parentClip.UpperLeftCorner.X > rect.UpperLeftCorner.X)
                rect.UpperLeftCorner.X = parentClip.UpperLeftCorner.X;
            if (parentClip.UpperLeftCorner.Y > rect.UpperLeftCorner.Y)
                rect.UpperLeftCorner.Y = parentClip.UpperLeftCorner.Y;
        }

        return rect;
    }

    //! Places the children in rows from the left, or from the right, wrapping at the content width,
    //! and grows the element to fit them (in width too when resize is set).
    virtual void sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize)
    {
        core::CRect<int> area = getContentArea();
        int width = area.getWidth() - spacingX;
        int y = spacingY;
        int rowHeight = 0;
        int maxWidth = 0;

        if (!rightToLeft)
        {
            int x = spacingX;
            bool first = true;
            for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            {
                if ((*it)->isFixed())
                    continue;

                core::CRect<int> rect = (*it)->getRelativePosition();
                int right = x + rect.getWidth();
                if (right >= width && !first)
                {
                    y += rowHeight + spacingY;
                    x = spacingX;
                    rowHeight = 0;
                    right = spacingX + rect.getWidth();
                }

                core::CRect<int> placed(x, y, right, rect.getHeight() + y);
                (*it)->setRelativePosition(placed);
                x += spacingX + placed.getWidth();
                first = false;
                if (rowHeight < placed.getHeight())
                    rowHeight = placed.getHeight();
                if (x > maxWidth)
                    maxWidth = x;
            }
        }
        else
        {
            int x = width;
            bool first = true;
            for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            {
                if ((*it)->isFixed())
                    continue;

                core::CRect<int> rect = (*it)->getRelativePosition();
                int left = x - rect.getWidth();
                if (spacingX > left && !first)
                {
                    y += rowHeight + spacingY;
                    rowHeight = 0;
                    left = width - rect.getWidth();
                }

                core::CRect<int> placed(left, y, left + rect.getWidth(), rect.getHeight() + y);
                (*it)->setRelativePosition(placed);
                x = left - spacingX;
                first = false;
                if (rowHeight < placed.getHeight())
                    rowHeight = placed.getHeight();
                if (width - x > maxWidth)
                    maxWidth = width - x;
            }
        }

        int growY = y + rowHeight + spacingY - area.getHeight();
        if (growY > 0)
            RelativeRect.LowerRightCorner.Y += growY;

        if (resize)
        {
            int growX = maxWidth - area.getWidth();
            if (growX > 0)
                RelativeRect.LowerRightCorner.X += growX;
        }

        setRelativePosition(RelativeRect);
        updateChildrenForContentArea();
    }

    //! Stacks the children from the top, centered horizontally when center is set, and grows the
    //! element to fit them.
    virtual void sortVertically(int spacing, bool center)
    {
        core::CRect<int> area = getContentArea();
        int width = area.getWidth();
        int x = 0;
        int y = spacing;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            core::CRect<int> rect = (*it)->getRelativePosition();
            if (center)
                x = (width - rect.getWidth()) / 2;

            core::CRect<int> placed(x, y, rect.getWidth() + x, rect.getHeight() + y);
            (*it)->setRelativePosition(placed);
            y += placed.getHeight() + spacing;
        }

        int grow = y - area.getHeight();
        if (grow > 0)
            RelativeRect.LowerRightCorner.Y += grow;

        setRelativePosition(RelativeRect);
        updateChildrenForContentArea();
    }

    //! Lines the children up from the left, centered vertically, and grows the element to fit them.
    virtual void sortHorizontally(int spacing)
    {
        core::CRect<int> area = getContentArea();
        int height = area.getHeight();
        int x = spacing;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            core::CRect<int> rect = (*it)->getRelativePosition();
            int y = (height - rect.getHeight()) / 2;
            core::CRect<int> placed(x, y, rect.getWidth() + x, y + rect.getHeight());
            (*it)->setRelativePosition(placed);
            x += placed.getWidth() + spacing;
        }

        int grow = x - area.getWidth();
        if (grow > 0)
            RelativeRect.LowerRightCorner.X += grow;

        setRelativePosition(RelativeRect);
        updateChildrenForContentArea();
    }

    //! Places the children at their preferred sizes in rows broken at "br" hints, lines up the
    //! elements hinted "tab" across the rows, aligns each row by its last "left", "center",
    //! "right", "top", "middle" or "bottom" hint and grows the element to fit when resize is set.
    //! Children without hints are only aligned when sortHidden is set.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden)
    {
        if (Children.empty())
            return;

        std::vector<TArray<IGUIElement*> > rows;
        std::vector<int> horizontal;
        std::vector<int> vertical;
        int horizontalAlign = 0;
        int verticalAlign = 0;
        rows.push_back(TArray<IGUIElement*>());
        horizontal.push_back(horizontalAlign);
        vertical.push_back(verticalAlign);

        unsigned int row = 0;
        int tabs = 0;
        int maxTabs = 0;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            const char* flags = (*it)->LayoutFlags;
            if (!flags && !sortHidden)
            {
                rows[row].push_back(*it);
                continue;
            }
            if (!flags)
                flags = "";

            core::CString<char> hints = flags;
            if (hints.findNext("br", 0) >= 0 && !rows[row].empty())
            {
                rows.push_back(TArray<IGUIElement*>());
                horizontal.push_back(horizontalAlign);
                vertical.push_back(verticalAlign);
                ++row;
                tabs = 0;
            }

            rows[row].push_back(*it);

            if (hints.findNext("top", 0) >= 0)
                vertical[row] = verticalAlign = 0;
            else if (hints.findNext("middle", 0) >= 0)
                vertical[row] = verticalAlign = 1;
            else if (hints.findNext("bottom", 0) >= 0)
                vertical[row] = verticalAlign = 2;

            if (hints.findNext("left", 0) >= 0)
                horizontal[row] = horizontalAlign = 0;
            else if (hints.findNext("center", 0) >= 0)
                horizontal[row] = horizontalAlign = 1;
            else if (hints.findNext("right", 0) >= 0)
                horizontal[row] = horizontalAlign = 2;

            if (hints.findNext("tab", 0) >= 0)
            {
                ++tabs;
                if (tabs > maxTabs)
                    maxTabs = tabs;
            }
        }

        unsigned int rowCount = rows.size();

        // place the rows at the preferred sizes
        int y = spacingY;
        for (unsigned int i = 0; i < rowCount; ++i)
        {
            int x = spacingX;
            int rowHeight = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                core::CDimension2d<int> size = rows[i][j]->getPreferredSize();
                rows[i][j]->setRelativePosition(core::CRect<int>(x, y, size.Width + x, size.Height + y));
                if (rowHeight < size.Height)
                    rowHeight = size.Height;
                x += spacingX + size.Width;
            }
            y += spacingY + rowHeight;
        }

        // line up the n-th tab of every row with the rightmost one
        for (int tab = 0; tab < maxTabs; ++tab)
        {
            int tabX = 0;
            for (unsigned int i = 0; i < rowCount; ++i)
            {
                int index = 0;
                for (unsigned int j = 0; j < rows[i].size(); ++j)
                {
                    const char* flags = rows[i][j]->LayoutFlags;
                    if (!flags && !sortHidden)
                        continue;
                    if (!flags)
                        flags = "";

                    core::CString<char> hints = flags;
                    if (hints.findNext("tab", 0) >= 0)
                    {
                        if (tab == index)
                        {
                            int left = rows[i][j]->getRelativePosition().UpperLeftCorner.X;
                            if (left > tabX)
                                tabX = left;
                            break;
                        }
                        ++index;
                    }
                }
            }

            for (unsigned int i = 0; i < rowCount; ++i)
            {
                int index = 0;
                for (unsigned int j = 0; j < rows[i].size(); ++j)
                {
                    const char* flags = rows[i][j]->LayoutFlags;
                    if (!flags && !sortHidden)
                        continue;
                    if (!flags)
                        flags = "";

                    core::CString<char> hints = flags;
                    if (hints.findNext("tab", 0) >= 0)
                    {
                        if (tab == index)
                        {
                            int offset = tabX - rows[i][j]->getRelativePosition().UpperLeftCorner.X;
                            if (offset > 0)
                            {
                                for (unsigned int k = j; k < rows[i].size(); ++k)
                                {
                                    core::CRect<int> rect = rows[i][k]->getRelativePosition();
                                    rect.UpperLeftCorner.X += offset;
                                    rect.LowerRightCorner.X += offset;
                                    rows[i][k]->setRelativePosition(rect);
                                }
                            }
                            break;
                        }
                        ++index;
                    }
                }
            }
        }

        if (resize)
        {
            int right = spacingX;
            int bottom = spacingY;
            for (unsigned int i = 0; i < rowCount; ++i)
            {
                for (unsigned int j = 0; j < rows[i].size(); ++j)
                {
                    core::CRect<int> rect = rows[i][j]->getRelativePosition();
                    if (bottom < rect.LowerRightCorner.Y)
                        bottom = rect.LowerRightCorner.Y;
                    if (right < rect.LowerRightCorner.X)
                        right = rect.LowerRightCorner.X;
                }
            }

            core::CRect<int> area = getContentArea();
            RelativeRect.LowerRightCorner.X += right + spacingX - area.getWidth();
            RelativeRect.LowerRightCorner.Y += bottom + spacingY - area.getHeight();
            setRelativePosition(RelativeRect);
        }

        core::CRect<int> area = getContentArea();
        int width = area.getWidth() - spacingX;

        // align the rows horizontally
        for (unsigned int i = 0; i < rowCount; ++i)
        {
            if (horizontal[i] == 0)
                continue;

            int right = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
                if (right < rows[i][j]->getRelativePosition().LowerRightCorner.X)
                    right = rows[i][j]->getRelativePosition().LowerRightCorner.X;

            int offset = width - right;
            if (horizontal[i] == 1)
                offset /= 2;

            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                core::CRect<int> rect = rows[i][j]->getRelativePosition();
                rect.UpperLeftCorner.X += offset;
                rect.LowerRightCorner.X += offset;
                rows[i][j]->setRelativePosition(rect);
            }
        }

        // align the elements of each row vertically
        for (unsigned int i = 0; i < rowCount; ++i)
        {
            if (vertical[i] == 0)
                continue;

            int rowHeight = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
                if (rowHeight < rows[i][j]->getRelativePosition().getHeight())
                    rowHeight = rows[i][j]->getRelativePosition().getHeight();

            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                core::CRect<int> rect = rows[i][j]->getRelativePosition();
                int offset = rowHeight - rect.getHeight();
                if (vertical[i] == 1)
                    offset /= 2;
                rect.UpperLeftCorner.Y += offset;
                rect.LowerRightCorner.Y += offset;
                rows[i][j]->setRelativePosition(rect);
            }
        }

        updateChildrenForContentArea();
    }

    //! The area children are placed in, relative to the element.
    virtual core::CRect<int> getContentArea()
    {
        return core::CRect<int>(0, 0, RelativeRect.getWidth(), RelativeRect.getHeight());
    }

    //! Moves the movable children by the content area's offset.
    virtual void updateChildrenForContentArea()
    {
        core::CRect<int> area = getContentArea();
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
            if (!(*it)->isFixed())
                (*it)->move(area.UpperLeftCorner);
    }
};

} // end namespace gui
} // end namespace ox

#endif
