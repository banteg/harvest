// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.
// The virtual order follows the Mac 1.18 vtable of ox::core::CAes; member names are ours.

#ifndef OX_CORE_ICIPHER_H
#define OX_CORE_ICIPHER_H

#include "CCipherKey.h"

namespace ox {
namespace core {

//! A block cipher with a key.
class ICipher
{
public:
    ICipher()
        : Key(0), EncryptKeyReady(false), DecryptKeyReady(false)
    {
    }

    // Inline: the Linux build emits it only with the ICipher vtable.
    virtual ~ICipher()
    {
        if (DecryptKeyReady && Key)
            delete Key;
    }

    //! The size of size bytes once encrypted, padded to whole blocks.
    virtual int getEncryptedDataSize(int size) const = 0;
    virtual int getValidKeyLength() const = 0;
    virtual bool encrypt(void* target, const void* source, int size) = 0;
    virtual bool decrypt(void* target, const void* source, int size) = 0;
    virtual const wchar_t* getShortName() const;
    virtual const wchar_t* getLongName() const;
    virtual bool isKeyValid(const CCipherKey* key) const = 0;

    //! Uses the key if it suits the cipher; the key schedules are built on first use.
    void setKey(const CCipherKey* key)
    {
        if (isKeyValid(key))
        {
            EncryptKeyReady = false;
            Key = key;
            DecryptKeyReady = false;
        }
    }

protected:
    const CCipherKey* Key;
    //! Set once the key schedule is built; CAes builds one schedule for both directions.
    bool EncryptKeyReady;
    //! Set when the cipher owns Key: the destructor then deletes it.
    bool DecryptKeyReady;
};

} // end namespace core
} // end namespace ox

#endif
