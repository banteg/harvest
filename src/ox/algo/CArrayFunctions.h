// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: other helpers in this header are not recovered yet.

#ifndef OX_ALGO_CARRAYFUNCTIONS_H
#define OX_ALGO_CARRAYFUNCTIONS_H

#include <algorithm>
#include <functional>
#include <iterator>

namespace ox {
namespace algo {

//! Returns the iterator moved by count positions, which may be negative.
template <class T>
T advanceIterator(T it, int count)
{
    std::advance(it, count);
    return it;
}

//! Searches a sorted array. searcher(value, element) compares like strcmp: negative when the value
//! sorts before the element. Returns the index of a matching element, or -1.
template <class TArrayType, class TSearcher, class TValue>
int binarySearchIf(const TArrayType& array, TSearcher searcher, const TValue& value)
{
    int low = 0;
    int high = array.size();
    while (low < high)
    {
        int middle = low + ((high - low) >> 1);
        int result = searcher(value, array[middle]);
        if (result == 0)
            return middle;
        if (result < 0)
            high = middle;
        else
            low = middle + 1;
    }
    return -1;
}

//! Returns the index of the first element in [first, last) that matches value by predicate, or -1.
template <class TIterator, class TPredicate, class TValue>
int linearSearchIf(TIterator first, TIterator last, TPredicate predicate, const TValue& value)
{
    TIterator found = std::find_if(first, last, std::bind2nd(predicate, value));
    if (found != last && predicate(*found, value))
        return found - first;
    return -1;
}

} // end namespace algo
} // end namespace ox

#endif
