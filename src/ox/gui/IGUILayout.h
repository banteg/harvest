// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the virtual order follows the Mac 1.18 vtable of daisy::gui::CGUIWindow up to sortRiver.

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
    }

    //! Without clipping, the children are clipped by the content area instead of the clipping rectangle.
    virtual core::CRect<int> getParentAbsoluteClippingRect(bool clip);
    //! Places the children in rows, from the right when rightToLeft; resize widens the element to fit.
    virtual void sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize);
    //! Stacks the children, horizontally centered when center is set.
    virtual void sortVertically(int spacing, bool center);
    //! Places the children side by side, vertically centered.
    virtual void sortHorizontally(int spacing);
    //! Places the children in rows broken at "br" hints, aligned by their other hints; "tab" hints line up
    //! in columns. Children without hints count as "tab" when tabUnflagged is set.
    virtual void sortRiver(bool resize, int spacingX, int spacingY, bool tabUnflagged);
    //! The area of the children, relative to the element.
    virtual core::CRect<int> getContentArea();
    //! Moves the movable children by the content area's offset.
    virtual void updateChildrenForContentArea();
};

// The Linux build emits these as COMDAT copies, so the original header defines them inline.
inline core::CRect<int> IGUILayout::getParentAbsoluteClippingRect(bool clip)
{
    if (IsInvisible && Parent)
        return Parent->getParentAbsoluteClippingRect(clip);

    if (clip)
        return AbsoluteClippingRect;

    core::CRect<int> rect = getContentArea();
    rect.UpperLeftCorner += AbsoluteRect.UpperLeftCorner;
    rect.LowerRightCorner += AbsoluteRect.UpperLeftCorner;
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

inline void IGUILayout::sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize)
{
    core::CRect<int> area = getContentArea();
    int maxX = area.getWidth() - spacingX;
    int y = spacingY;
    int rowHeight = 0;
    int maxWidth = 0;
    bool first = true;

    if (!rightToLeft)
    {
        int x = spacingX;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            core::CRect<int> r = (*it)->getRelativePosition();
            if (x + r.getWidth() >= maxX && !first)
            {
                y += rowHeight + spacingY;
                x = spacingX;
                rowHeight = 0;
            }

            core::CRect<int> rect(x, y, x + r.getWidth(), r.getHeight() + y);
            (*it)->setRelativePosition(rect);
            x += rect.getWidth() + spacingX;
            first = false;
            if (rowHeight < rect.getHeight())
                rowHeight = rect.getHeight();
            if (x > maxWidth)
                maxWidth = x;
        }
    }
    else
    {
        int x = maxX;
        for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        {
            if ((*it)->isFixed())
                continue;

            core::CRect<int> r = (*it)->getRelativePosition();
            x -= r.getWidth();
            if (x < spacingX && !first)
            {
                y += rowHeight + spacingY;
                x = maxX - r.getWidth();
                rowHeight = 0;
            }

            core::CRect<int> rect(x, y, x + r.getWidth(), r.getHeight() + y);
            (*it)->setRelativePosition(rect);
            x -= spacingX;
            first = false;
            if (rowHeight < rect.getHeight())
                rowHeight = rect.getHeight();
            if (maxX - x > maxWidth)
                maxWidth = maxX - x;
        }
    }

    int extra = y + rowHeight + spacingY + area.UpperLeftCorner.Y - area.LowerRightCorner.Y;
    if (extra > 0)
        RelativeRect.LowerRightCorner.Y += extra;

    if (resize)
    {
        extra = maxWidth + area.UpperLeftCorner.X - area.LowerRightCorner.X;
        if (extra > 0)
            RelativeRect.LowerRightCorner.X += extra;
    }

    setRelativePosition(RelativeRect);
    updateChildrenForContentArea();
}

inline void IGUILayout::sortVertically(int spacing, bool center)
{
    core::CRect<int> area = getContentArea();
    int width = area.getWidth();
    int x = 0;
    int y = spacing;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->isFixed())
            continue;

        core::CRect<int> r = (*it)->getRelativePosition();
        if (center)
            x = (width - r.getWidth()) / 2;

        core::CRect<int> rect(x, y, r.getWidth() + x, r.getHeight() + y);
        (*it)->setRelativePosition(rect);
        y += rect.getHeight() + spacing;
    }

    int extra = y + area.UpperLeftCorner.Y - area.LowerRightCorner.Y;
    if (extra > 0)
        RelativeRect.LowerRightCorner.Y += extra;

    setRelativePosition(RelativeRect);
    updateChildrenForContentArea();
}

inline void IGUILayout::sortHorizontally(int spacing)
{
    core::CRect<int> area = getContentArea();
    int height = area.getHeight();
    int x = spacing;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->isFixed())
            continue;

        core::CRect<int> r = (*it)->getRelativePosition();
        int y = (height - r.getHeight()) / 2;
        core::CRect<int> rect(x, y, r.getWidth() + x, y + r.getHeight());
        (*it)->setRelativePosition(rect);
        x += rect.getWidth() + spacing;
    }

    int extra = x + area.UpperLeftCorner.X - area.LowerRightCorner.X;
    if (extra > 0)
        RelativeRect.LowerRightCorner.X += extra;

    setRelativePosition(RelativeRect);
    updateChildrenForContentArea();
}

inline void IGUILayout::sortRiver(bool resize, int spacingX, int spacingY, bool tabUnflagged)
{
    if (Children.empty())
        return;

    std::vector<TArray<IGUIElement*> > rows;
    // the alignments of each row: 0 left or top, 1 center or middle, 2 right or bottom
    std::vector<int> rowHAlign;
    std::vector<int> rowVAlign;
    int vAlign = 0;
    int hAlign = 0;
    rows.push_back(TArray<IGUIElement*>());
    rowHAlign.push_back(hAlign);
    // the first row's vertical alignment is pushed from the horizontal one, as in the original
    rowVAlign.push_back(hAlign);

    int row = 0;
    int tabs = 0;
    int maxTabs = 0;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->isFixed())
            continue;

        const char* hints = (*it)->LayoutFlags;
        if (!hints)
        {
            if (!tabUnflagged)
            {
                rows[row].push_back(*it);
                continue;
            }
            hints = "tab";
        }

        core::CString<char> flags = hints;
        if (flags.findNext("br", 0) >= 0 && !rows[row].empty())
        {
            rows.push_back(TArray<IGUIElement*>());
            rowHAlign.push_back(hAlign);
            rowVAlign.push_back(vAlign);
            ++row;
            tabs = 0;
        }

        rows[row].push_back(*it);

        if (flags.findNext("top", 0) >= 0)
        {
            vAlign = 0;
            rowVAlign[row] = 0;
        }
        else if (flags.findNext("middle", 0) >= 0)
        {
            vAlign = 1;
            rowVAlign[row] = 1;
        }
        else if (flags.findNext("bottom", 0) >= 0)
        {
            vAlign = 2;
            rowVAlign[row] = 2;
        }

        if (flags.findNext("left", 0) >= 0)
        {
            hAlign = 0;
            rowHAlign[row] = 0;
        }
        else if (flags.findNext("center", 0) >= 0)
        {
            hAlign = 1;
            rowHAlign[row] = 1;
        }
        else if (flags.findNext("right", 0) >= 0)
        {
            hAlign = 2;
            rowHAlign[row] = 2;
        }

        if (flags.findNext("tab", 0) >= 0)
        {
            ++tabs;
            if (maxTabs < tabs)
                maxTabs = tabs;
        }
    }

    // the rows at their preferred sizes
    int y = spacingY;
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        int x = spacingX;
        int rowHeight = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            core::CDimension2d<int> size = rows[i][j]->getPreferredSize();
            rows[i][j]->setRelativePosition(core::CRect<int>(x, y, size.Width + x, size.Height + y));
            if (rowHeight < size.Height)
                rowHeight = size.Height;
            x += size.Width + spacingX;
        }
        y += rowHeight + spacingY;
    }

    // line up the tab columns
    for (int tab = 0; tab < maxTabs; ++tab)
    {
        int column = 0;
        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            int index = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                const char* hints = rows[i][j]->LayoutFlags;
                if (!hints)
                {
                    if (!tabUnflagged)
                        continue;
                    hints = "tab";
                }

                core::CString<char> flags = hints;
                if (flags.findNext("tab", 0) >= 0)
                {
                    if (index == tab)
                    {
                        int x = rows[i][j]->getRelativePosition().UpperLeftCorner.X;
                        if (column < x)
                            column = x;
                        break;
                    }
                    ++index;
                }
            }
        }

        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            int index = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                const char* hints = rows[i][j]->LayoutFlags;
                if (!hints)
                {
                    if (!tabUnflagged)
                        continue;
                    hints = "tab";
                }

                core::CString<char> flags = hints;
                if (flags.findNext("tab", 0) >= 0)
                {
                    if (index == tab)
                    {
                        int shift = column - rows[i][j]->getRelativePosition().UpperLeftCorner.X;
                        if (shift > 0)
                        {
                            for (unsigned int k = j; k < rows[i].size(); ++k)
                            {
                                core::CRect<int> rect = rows[i][k]->getRelativePosition();
                                rect.UpperLeftCorner.X += shift;
                                rect.LowerRightCorner.X += shift;
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
        int width = spacingX;
        int height = spacingY;
        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                core::CRect<int> rect = rows[i][j]->getRelativePosition();
                if (width < rect.LowerRightCorner.X)
                    width = rect.LowerRightCorner.X;
                if (height < rect.LowerRightCorner.Y)
                    height = rect.LowerRightCorner.Y;
            }
        }

        core::CRect<int> area = getContentArea();
        RelativeRect.LowerRightCorner.X += width + spacingX - area.LowerRightCorner.X + area.UpperLeftCorner.X;
        RelativeRect.LowerRightCorner.Y += height + spacingY - area.LowerRightCorner.Y + area.UpperLeftCorner.Y;
        setRelativePosition(RelativeRect);
    }

    // align the rows
    int areaWidth = getContentArea().getWidth();
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        int align = rowHAlign[i];
        if (align == 0)
            continue;

        int right = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            int x = rows[i][j]->getRelativePosition().LowerRightCorner.X;
            if (right < x)
                right = x;
        }

        int shift = (areaWidth - right - spacingX) / (align == 1 ? 2 : 1);
        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            core::CRect<int> rect = rows[i][j]->getRelativePosition();
            rect.UpperLeftCorner.X += shift;
            rect.LowerRightCorner.X += shift;
            rows[i][j]->setRelativePosition(rect);
        }
    }

    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        int align = rowVAlign[i];
        if (align == 0)
            continue;

        int height = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            int h = rows[i][j]->getRelativePosition().getHeight();
            if (height < h)
                height = h;
        }

        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            core::CRect<int> rect = rows[i][j]->getRelativePosition();
            int shift = (height - rect.getHeight()) / (align == 1 ? 2 : 1);
            rect.UpperLeftCorner.Y += shift;
            rect.LowerRightCorner.Y += shift;
            rows[i][j]->setRelativePosition(rect);
        }
    }

    updateChildrenForContentArea();
}

inline core::CRect<int> IGUILayout::getContentArea()
{
    return core::CRect<int>(0, 0, RelativeRect.getWidth(), RelativeRect.getHeight());
}

inline void IGUILayout::updateChildrenForContentArea()
{
    core::CPosition2d<int> offset = getContentArea().UpperLeftCorner;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
        if (!(*it)->isFixed())
            (*it)->move(offset);
}

} // end namespace gui
} // end namespace ox

#endif
