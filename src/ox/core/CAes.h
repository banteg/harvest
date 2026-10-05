// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// Partial: the tail keeps the Linux object size; the key schedule is not recovered yet.

#ifndef OX_CORE_CAES_H
#define OX_CORE_CAES_H

#include "ICipher.h"

namespace ox {
namespace core {

//! AES with 128, 192 or 256 bit keys.
class CAes : public ICipher
{
public:
    CAes();
    virtual ~CAes();

    virtual int getEncryptedDataSize(int size) const;
    virtual int getValidKeyLength() const;
    virtual bool encrypt(void* target, const void* source, int size);
    virtual bool decrypt(void* target, const void* source, int size);
    virtual const char* getShortName() const;
    virtual const char* getLongName() const;
    virtual bool isKeyValid(const CCipherKey* key) const;

private:
    // Not recovered yet; keeps the Linux object size of 328 bytes.
    char Unrecovered[328 - sizeof(ICipher)];
};

} // end namespace core
} // end namespace ox

#endif
