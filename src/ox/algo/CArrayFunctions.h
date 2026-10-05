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

//! Returns the index of value in [begin, end), or -1.
template <class I, class T>
int linearSearchPos(I begin, I end, const T& value)
{
    I it = std::find(begin, end, value);
    if (it != end && *it == value)
        return it - begin;
    return -1;
}

//! Returns the index of an element of a sorted array that the searcher matches with value, or -1.
//! The searcher returns a negative number when value sorts before the element, 0 on a match and a
//! positive number when it sorts after.
template <class A, class S, class V>
int binarySearchIf(const A& array, S searcher, const V& value)
{
    int low = 0;
    int high = array.size();
    while (low < high)
    {
        int middle = ((high - low) >> 1) + low;
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
