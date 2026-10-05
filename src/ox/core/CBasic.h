// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Declarations are added as the functions are recovered.

#ifndef OX_CORE_CBASIC_H
#define OX_CORE_CBASIC_H

#include "CString.h"

namespace ox {
namespace core {

//! Returns the smaller of two values.
template <class T>
inline const T min_(const T a, const T b)
{
    return a < b ? a : b;
}

//! Returns the larger of two values.
template <class T>
inline T max_(const T a, const T b)
{
    return a > b ? a : b;
}

//! Returns the absolute value.
//! value limited to [low, high]; the name and argument order are provisional.
template <class T>
inline T clamp(const T value, const T low, const T high)
{
    return value > high ? high : max_(low, value);
}

template <class T>
inline T abs_(const T a)
{
    return a < 0 ? -a : a;
}

//! Assorted helpers.
class CBasic
{
public:
    //! The current local time, formatted with strftime.
    static CString<char> getTimeString(char* format);
    //! A time_t value as local time, formatted with strftime.
    static CString<char> getTimeString(int time, char* format);
};

} // end namespace core
} // end namespace ox

#endif
