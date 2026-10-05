// Recovered for Harvest; not the original source. The file name is inferred from ox/TArray.h: the
// Mac debug map records no header for ox::TList. Mac builds its nodes with std::list's
// _M_create_node, so it adds no state of its own.

#ifndef OX_TLIST_H
#define OX_TLIST_H

#include <list>

namespace ox {

template <class T>
class TList : public std::list<T>
{
};

} // end namespace ox

#endif
