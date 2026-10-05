// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: only what recovered units use is declared.

#ifndef DAISY_IO_CMEMORYREADFILE_H
#define DAISY_IO_CMEMORYREADFILE_H

#include "ox/io/IReadFile.h"

namespace daisy {
namespace io {

//! Creates a read file over a memory block, which is deleted with the file when wanted.
ox::io::IReadFile* createMemoryReadFile(void* memory, int size, const char* fileName, bool deleteMemoryWhenDropped);

} // end namespace io
} // end namespace daisy

#endif
