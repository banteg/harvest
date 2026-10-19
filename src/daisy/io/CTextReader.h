// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial and provisional: only what CFileSystem::createXMLReader needs is declared, sized as the
// Linux build allocates it. The game never uses XML.

#ifndef DAISY_IO_CTEXTREADER_H
#define DAISY_IO_CTEXTREADER_H

#include "ox/io/IReadFile.h"

namespace daisy {
namespace io {

//! Reads a whole text file and converts it to wide characters for the XML reader.
class CTextReader : public ox::IUnknown
    {
    public:

        CTextReader(ox::io::IReadFile* file);

        //! Destructor
        virtual ~CTextReader();

        //! Returns pointer to read text buffer
        wchar_t* getText();

        //! returns size of text buffer
        int getTextSize() const;

    private:

        //! reads the complete file into the buffer
        void readFile(ox::io::IReadFile* file);

        //! converts the read text data to utf16 text.
        void convertToUTF16();

        //! converts ASCII to an UTF-16, output pointer should be deleted by caller
        void convertASCIIToUTF16(const char* characters, unsigned int len, wchar_t** out);

        //! converts an big endian to little endian
        inline unsigned short convertBEtoLE (unsigned short be)
        {
            return (be >> 8) | (be << 8);
        }

        char* CData;                    // read data from file
        wchar_t* TextData;          // pointer to textdata
        unsigned int TextDataSize;  // size of wchat_t buffer without header
        unsigned int CDataSize;     // site of char buffer
    };

} // end namespace io
} // end namespace daisy

#endif
