// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only the factory that CZipReader uses is declared.

#ifndef DAISY_IO_CLIMITREADFILE_H
#define DAISY_IO_CLIMITREADFILE_H

#include "ox/io/IReadFile.h"

namespace daisy {
namespace io {

//! Creates a read file over areaSize bytes of an open file, starting at its current position.
ox::io::IReadFile* createLimitReadFile(const char* fileName, ox::io::IReadFile* alreadyOpenedFile, int areaSize);

} // end namespace io
} // end namespace daisy

#endif
