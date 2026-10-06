// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 include/irrString.h (license: third_party/irrlicht-0.7/include/irrlicht.h).
// Recovered for Harvest's ox::core namespace; not the original source. Oxeye made subString const
// and added subStringToEnd and a string-argument findNext.

#ifndef OX_CORE_CSTRING_H
#define OX_CORE_CSTRING_H

namespace ox {
namespace core {

//! Very simple string class with some useful features.
/** CString<char> and CString<wchar_t> work with both unicode and ascii: assigning one to the
    other converts per character, without any encoding. */
template <class T>
class CString
{
public:
    //! Default constructor
    CString()
        : array(0), allocated(1), used(1)
    {
        array = new T[1];
        array[0] = 0x0;
    }

    //! Copy constructor
    CString(const CString<T>& other)
        : array(0), allocated(0), used(0)
    {
        *this = other;
    }

    //! Constructs a string from an int
    CString(int number)
        : array(0), allocated(0), used(0)
    {
        // store if negative and make positive
        bool negative = false;
        if (number < 0)
        {
            number *= -1;
            negative = true;
        }

        // temporary buffer for 16 numbers
        char tmpbuf[16];
        tmpbuf[15] = 0;
        int idx = 15;

        // special case '0'
        if (!number)
        {
            tmpbuf[14] = '0';
            *this = &tmpbuf[14];
            return;
        }

        // add numbers
        while (number && idx)
        {
            idx--;
            tmpbuf[idx] = (char)('0' + (number % 10));
            number = number / 10;
        }

        // add sign
        if (negative)
        {
            idx--;
            tmpbuf[idx] = '-';
        }

        *this = &tmpbuf[idx];
    }

    //! Constructor for copying a string from a pointer with a given length
    template <class B>
    CString(const B* c, int length)
        : array(0), allocated(0), used(0)
    {
        if (!c)
            return;

        allocated = used = length + 1;
        array = new T[used];

        for (int l = 0; l < length; ++l)
            array[l] = (T)c[l];

        array[length] = 0;
    }

    //! Constructor for unicode and ascii strings
    template <class B>
    CString(const B* c)
        : array(0), allocated(0), used(0)
    {
        *this = c;
    }

    //! Destructor
    ~CString()
    {
        delete [] array;
    }

    //! Assignment operator
    CString<T>& operator=(const CString<T>& other)
    {
        if (this == &other)
            return *this;

        delete [] array;
        allocated = used = other.used;
        array = new T[used];

        const T* p = other.c_str();
        for (int i = 0; i < used; ++i, ++p)
            array[i] = *p;

        return *this;
    }

    //! Assignment operator for strings, ascii and unicode
    template <class B>
    CString<T>& operator=(const B* c)
    {
        if (!c)
        {
            if (!array)
            {
                array = new T[1];
                allocated = 1;
                used = 1;
            }
            array[0] = 0x0;
            return *this;
        }

        if ((void*)c == (void*)array)
            return *this;

        int len = 0;
        const B* p = c;
        while (*p)
        {
            ++len;
            ++p;
        }

        // we'll take the old string for a while, because the new string could be
        // a part of the current string
        T* oldArray = array;

        allocated = used = len + 1;
        array = new T[used];

        for (int l = 0; l < len + 1; ++l)
            array[l] = (T)c[l];

        delete [] oldArray;
        return *this;
    }

    //! Direct access operator
    T& operator[](const int index) const
    {
        return array[index];
    }

    //! Comparison operator
    bool operator==(const CString<T>& other) const
    {
        for (int i1 = 0, i2 = 0; array[i1] && other.array[i2]; ++i1, ++i2)
            if (array[i1] != other.array[i2])
                return false;

        return used == other.used;
    }

    //! Is smaller operator
    bool operator<(const CString<T>& other) const
    {
        for (int i1 = 0, i2 = 0; array[i1] && other.array[i2]; ++i1, ++i2)
            if (array[i1] != other.array[i2])
                return (array[i1] < other.array[i2]);

        return used < other.used;
    }

    //! Equals not operator
    bool operator!=(const CString<T>& other) const
    {
        return !(*this == other);
    }

    //! Returns length of string, without the terminating 0
    int size() const
    {
        return used - 1;
    }

    //! Returns character string
    const T* c_str() const
    {
        return array;
    }

    //! Makes the string lower case.
    void make_lower()
    {
        const T A = (T)'A';
        const T Z = (T)'Z';
        const T diff = (T)'a' - A;

        for (int i = 0; i < used; ++i)
        {
            if (array[i] >= A && array[i] <= Z)
                array[i] += diff;
        }
    }

    //! Makes the string upper case.
    void make_upper()
    {
        const T a = (T)'a';
        const T z = (T)'z';
        const T diff = (T)'A' - a;

        for (int i = 0; i < used; ++i)
        {
            if (array[i] >= a && array[i] <= z)
                array[i] += diff;
        }
    }

    //! Compares the string ignoring case.
    bool equals_ignore_case(const CString<T>& other) const
    {
        for (int i1 = 0, i2 = 0; array[i1] && other[i2]; ++i1, ++i2)
            if (toLower(array[i1]) != toLower(other[i2]))
                return false;

        return used == other.used;
    }

    //! Appends a character to this string
    void append(T character)
    {
        if (used + 1 > allocated)
            reallocate((int)used + 1);

        used += 1;

        array[used - 2] = character;
        array[used - 1] = 0;
    }

    //! Appends a string to this string
    void append(const CString<T>& other)
    {
        --used;

        int len = other.size();

        if (used + len + 1 > allocated)
            reallocate((int)used + (int)len + 1);

        for (int l = 0; l < len + 1; ++l)
            array[l + used] = other[l];

        used = used + len + 1;
    }

    //! Appends a number in decimal. An Oxeye addition.
    void append(int number)
    {
        if (!number)
        {
            if (used + 1 > allocated)
                reallocate((int)used + 1);

            array[used - 1] = (T)'0';
            array[used] = 0;
            ++used;
            return;
        }

        // store if negative and make positive
        bool negative = false;
        if (number < 0)
        {
            number *= -1;
            negative = true;
        }

        // temporary buffer for 16 numbers
        T tmpbuf[16];
        tmpbuf[15] = 0;
        int idx = 15;

        // add numbers
        while (number && idx)
        {
            idx--;
            tmpbuf[idx] = (T)('0' + (number % 10));
            number = number / 10;
        }

        // add sign
        if (negative)
        {
            idx--;
            tmpbuf[idx] = '-';
        }

        int len = 15 - idx;

        if (used + len + 1 > allocated)
            reallocate((int)used + (int)len + 1);

        for (unsigned int i = idx; i < 16; ++i)
            array[used + i - idx - 1] = tmpbuf[i];

        used += len;
    }

    //! Appends a string of the length l to this string.
    void append(const CString<T>& other, int length)
    {
        int len = other.size();

        if (len < length)
        {
            append(other);
            return;
        }

        len = length;
        --used;

        if (used + len > allocated)
            reallocate((int)used + (int)len);

        for (int l = 0; l < len; ++l)
            array[l + used] = other[l];

        used = used + len;
    }

    //! Reserves some memory.
    void reserve(int count)
    {
        if (count < allocated)
            return;

        reallocate(count);
    }

    //! Finds first occurrence of character in string.
    int findFirst(T c) const
    {
        for (int i = 0; i < used; ++i)
            if (array[i] == c)
                return i;

        return -1;
    }

    //! Finds next occurrence of character in string.
    int findNext(T c, int startPos) const
    {
        for (int i = startPos; i < used; ++i)
            if (array[i] == c)
                return i;

        return -1;
    }

    //! Finds the next occurrence of a zero-terminated string, or -1.
    int findNext(const T* str, int startPos) const
    {
        for (int i = startPos; i < used; ++i)
        {
            if (array[i] == str[0])
            {
                int j = 0;
                while (array[i + j] == str[j])
                {
                    ++j;
                    if (!str[j])
                        return i;
                }
            }
        }

        return -1;
    }

    //! Finds last occurrence of character in string.
    int findLast(T c) const
    {
        for (int i = used - 1; i >= 0; --i)
            if (array[i] == c)
                return i;

        return -1;
    }

    //! Returns true if the string starts with other. The name is provisional: every copy is inlined.
    bool startsWith(const CString<T>& other) const
    {
        for (int i = 0; i < other.size(); ++i)
            if (array[i] != other.array[i])
                return false;
        return true;
    }

    //! Replaces all characters of a special type with another one
    void replace(T toReplace, T replaceWith)
    {
        for (int i = 0; i < used; ++i)
            if (array[i] == toReplace)
                array[i] = replaceWith;
    }

    //! Returns true if the string ends with other, terminators compared. The name is provisional:
    //! every copy is inlined.
    bool endsWith(const CString<T>& other) const
    {
        int offset = used - other.used;
        if (offset < 0)
            return false;
        for (int i = 0; i < other.used; ++i)
            if (array[offset + i] != other.array[i])
                return false;
        return true;
    }

    //! Returns a substring
    CString<T> subString(int begin, int length) const
    {
        if (length <= 0)
            return CString<T>("");

        CString<T> o;
        o.reserve(length + 1);

        for (int i = 0; i < length; ++i)
            o.array[i] = array[i + begin];

        o.array[length] = 0;
        o.used = o.allocated;

        return o;
    }

    //! Returns the substring from begin to the end of the string
    CString<T> subStringToEnd(int begin) const
    {
        if (begin == 0)
            return *this;

        if (begin >= used)
            return CString<T>();

        CString<T> o;
        o.reserve(used - begin);

        for (int i = begin; i < used; ++i)
            o.array[i - begin] = array[i];

        o.used = o.allocated;

        return o;
    }

    void operator+=(T c)
    {
        append(c);
    }

    void operator+=(const CString<T>& other)
    {
        append(other);
    }

    void operator+=(int i)
    {
        append(CString<T>(i));
    }

    //! Appends a C string in place and returns this string. An Oxeye addition (Mac
    //! CString<char>::operator+<char>(const char*)); unlike std::string it modifies the left side.
    template <class B>
    CString<T>& operator+(const B* c)
    {
        --used;

        int len = 0;
        const B* p = c;
        while (*p)
        {
            ++len;
            ++p;
        }
        ++len;

        if (used + len > allocated)
            reallocate(used + len);

        for (int l = 0; l < len; ++l)
            array[used + l] = (T)c[l];

        used += len;
        return *this;
    }

private:
    //! Returns a character converted to lower case
    T toLower(const T& t) const
    {
        if (t >= (T)'A' && t <= (T)'Z')
            return t + ((T)'a' - (T)'A');
        else
            return t;
    }

    //! Reallocate the array, make it bigger or smaller
    void reallocate(int new_size)
    {
        T* old_array = array;

        array = new T[new_size];
        allocated = new_size;

        int amount = used < new_size ? used : new_size;
        for (int i = 0; i < amount; ++i)
            array[i] = old_array[i];

        if (allocated < used)
            used = allocated;

        delete [] old_array;
    }

    //--- member variables

    T* array;
    int allocated;
    int used;
};

} // end namespace core
} // end namespace ox

#endif
