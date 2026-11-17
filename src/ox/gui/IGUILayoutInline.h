// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The inline IGUILayout sorters. The Linux build emits their copies in daisy/gui/CGUIEnvironment.cpp.
// They are kept out of IGUILayout.h because instantiating their templates there changes the
// register choices of the game units that include it.

#ifndef OX_GUI_IGUILAYOUTINLINE_H
#define OX_GUI_IGUILAYOUTINLINE_H

#include <vector>
#include "IGUILayout.h"
#include "IGUIWindow.h"
#include "IGUIElementInline.h"
#include "../TArray.h"

namespace ox {
namespace gui {

inline IGUILayout::IGUILayout(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
    : IGUIElement(environment, parent, id, rectangle)
{
    Type = EGUIET_LAYOUT;
}

inline IGUIWindow::IGUIWindow(IGUIEnvironment* environment, IGUIElement* parent, int id, core::CRect<int> rectangle)
    : IGUILayout(environment, parent, id, rectangle)
{
    Type = EGUIET_WINDOW;
}

inline void IGUILayout::sortFlow(int spacingX, int spacingY, bool rightToLeft, bool resize)
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
            x -= rect.getWidth();
            if (spacingX > x && !first)
            {
                y += rowHeight + spacingY;
                rowHeight = 0;
                x = width - rect.getWidth();
            }

            core::CRect<int> placed(x, y, x + rect.getWidth(), rect.getHeight() + y);
            x -= spacingX;
            (*it)->setRelativePosition(placed);
            first = false;
            if (rowHeight < placed.getHeight())
                rowHeight = placed.getHeight();
            maxWidth = width - x < maxWidth ? maxWidth : width - x;
        }
    }

    int growY = y + rowHeight + (spacingY + area.UpperLeftCorner.Y - area.LowerRightCorner.Y);
    if (growY > 0)
        RelativeRect.LowerRightCorner.Y += growY;

    if (resize)
    {
        int growX = maxWidth + area.UpperLeftCorner.X - area.LowerRightCorner.X;
        if (growX > 0)
            RelativeRect.LowerRightCorner.X += growX;
    }

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

        core::CRect<int> rect = (*it)->getRelativePosition();
        int y = (height - rect.getHeight()) / 2;
        core::CRect<int> placed(x, y, rect.getWidth() + x, y + rect.getHeight());
        (*it)->setRelativePosition(placed);
        x += placed.getWidth() + spacing;
    }

    int grow = x + area.UpperLeftCorner.X - area.LowerRightCorner.X;
    if (grow > 0)
        RelativeRect.LowerRightCorner.X += grow;

    setRelativePosition(RelativeRect);
    updateChildrenForContentArea();
}

inline void IGUILayout::sortRiver(bool resize, int spacingX, int spacingY, bool sortHidden)
{
    if (Children.empty())
        return;

    int horizontalAlign = 0;
    int verticalAlign = 0;
    std::vector<TArray<IGUIElement*> > rows;
    std::vector<int> horizontal;
    std::vector<int> vertical;
    rows.push_back(TArray<IGUIElement*>());
    // the first row's vertical alignment starts from horizontalAlign, as in both builds (both are 0)
    horizontal.push_back(horizontalAlign);
    vertical.push_back(horizontalAlign);

    int tabs = 0;
    int row = 0;
    int maxTabs = 0;
    for (std::list<IGUIElement*>::iterator it = Children.begin(); it != Children.end(); ++it)
    {
        if ((*it)->isFixed())
            continue;

        const char* flags = (*it)->LayoutFlags;
        if (!flags)
        {
            if (!sortHidden)
            {
                rows[row].push_back(*it);
                continue;
            }
            flags = "tab";
        }

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
        {
            verticalAlign = 0;
            vertical[row] = 0;
        }
        else if (hints.findNext("middle", 0) >= 0)
        {
            verticalAlign = 1;
            vertical[row] = 1;
        }
        else if (hints.findNext("bottom", 0) >= 0)
        {
            verticalAlign = 2;
            vertical[row] = 2;
        }

        if (hints.findNext("left", 0) >= 0)
        {
            horizontalAlign = 0;
            horizontal[row] = 0;
        }
        else if (hints.findNext("center", 0) >= 0)
        {
            horizontalAlign = 1;
            horizontal[row] = 1;
        }
        else if (hints.findNext("right", 0) >= 0)
        {
            horizontalAlign = 2;
            horizontal[row] = 2;
        }

        if (hints.findNext("tab", 0) >= 0)
        {
            ++tabs;
            if (maxTabs < tabs)
                maxTabs = tabs;
        }
    }

    // place the rows at the preferred sizes
    int y = spacingY;
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        int x = spacingX;
        int rowHeight = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            core::CDimension2d<int> size = rows[i][j]->getPreferredSize();
            if (rowHeight < size.Height)
                rowHeight = size.Height;
            rows[i][j]->setRelativePosition(core::CRect<int>(core::CPosition2d<int>(x, y), size));
            x += size.Width + spacingX;
        }
        y += rowHeight + spacingY;
    }

    // line up the n-th tab of every row with the rightmost one
    for (int tab = 0; tab < maxTabs; ++tab)
    {
        int tabX = 0;
        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            int index = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                const char* flags = rows[i][j]->LayoutFlags;
                if (!flags)
                {
                    if (!sortHidden)
                        continue;
                    flags = "tab";
                }

                core::CString<char> hints = flags;
                if (hints.findNext("tab", 0) >= 0)
                {
                    if (index == tab)
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

        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            int index = 0;
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                const char* flags = rows[i][j]->LayoutFlags;
                if (!flags)
                {
                    if (!sortHidden)
                        continue;
                    flags = "tab";
                }

                core::CString<char> hints = flags;
                if (hints.findNext("tab", 0) >= 0)
                {
                    if (index == tab)
                    {
                        int offset = tabX - rows[i][j]->getRelativePosition().UpperLeftCorner.X;
                        if (offset > 0)
                        {
                            for (; rows[i].size() > j; ++j)
                            {
                                core::CRect<int> rect = rows[i][j]->getRelativePosition();
                                rect.UpperLeftCorner.X += offset;
                                rect.LowerRightCorner.X += offset;
                                rows[i][j]->setRelativePosition(rect);
                            }
                        }
                        break;
                    }
                    ++index;
                }
            }
        }
    }

    core::CRect<int> area;
    if (resize)
    {
        int bottom = spacingY;
        int right = spacingX;
        for (unsigned int i = 0; i < rows.size(); ++i)
        {
            for (unsigned int j = 0; j < rows[i].size(); ++j)
            {
                core::CRect<int> rect = rows[i][j]->getRelativePosition();
                if (right < rect.LowerRightCorner.X)
                    right = rect.LowerRightCorner.X;
                if (bottom < rect.LowerRightCorner.Y)
                    bottom = rect.LowerRightCorner.Y;
            }
        }

        area = getContentArea();
        int height = area.getHeight();
        RelativeRect.LowerRightCorner.X += right + spacingX - area.getWidth();
        RelativeRect.LowerRightCorner.Y += bottom + (spacingY - height);
        setRelativePosition(RelativeRect);
    }

    area = getContentArea();
    int width = area.getWidth();

    // align the rows horizontally; the divisor is 2 for centered rows
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        if (horizontal[i] == 0)
            continue;

        int right = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
            if (right < rows[i][j]->getRelativePosition().LowerRightCorner.X)
                right = rows[i][j]->getRelativePosition().LowerRightCorner.X;

        int offset = width - right - spacingX;
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
    for (unsigned int i = 0; i < rows.size(); ++i)
    {
        int align = vertical[i];
        if (align == 0)
            continue;

        int rowHeight = 0;
        for (unsigned int j = 0; j < rows[i].size(); ++j)
            if (rowHeight < rows[i][j]->getRelativePosition().getHeight())
                rowHeight = rows[i][j]->getRelativePosition().getHeight();

        for (unsigned int j = 0; j < rows[i].size(); ++j)
        {
            core::CRect<int> rect = rows[i][j]->getRelativePosition();
            int offset = rowHeight - rect.getHeight();
            if (align == 1)
                offset /= 2;
            rect.UpperLeftCorner.Y += offset;
            rect.LowerRightCorner.Y += offset;
            rows[i][j]->setRelativePosition(rect);
        }
    }

    updateChildrenForContentArea();
}

} // end namespace gui
} // end namespace ox

#endif
