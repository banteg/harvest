// Copyright (C) 2002-2004 Nikolaus Gebhardt
// Adapted from Irrlicht 0.7 source/Irrlicht/os.h for Harvest; not the original Daisy source.
// See third_party/irrlicht-0.7/readme.txt for the zlib/libpng license.
// Partial: only what recovered units use is declared.

#ifndef DAISY_OS_H
#define DAISY_OS_H

namespace daisy {
namespace os {

class Timer
{
public:
    //! Milliseconds since the timer was initialized.
    static unsigned int getTime();
};

} // end namespace os
} // end namespace daisy

#endif
