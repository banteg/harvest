// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_CORE_CAES_H
#define OX_CORE_CAES_H

#include "ICipher.h"

namespace ox {
namespace core {

//! AES with 256 bit keys. Data that is not a whole number of blocks is padded with zeros.
class CAes : public ICipher
{
public:
    CAes();
    virtual ~CAes();

    virtual int getEncryptedDataSize(int size) const;
    virtual int getValidKeyLength() const;
    virtual bool encrypt(void* target, const void* source, int size);
    virtual bool decrypt(void* target, const void* source, int size);
    virtual const wchar_t* getShortName() const;
    virtual const wchar_t* getLongName() const;
    virtual bool isKeyValid(const CCipherKey* key) const;

private:
    void expandKey();
    //! Builds the GF(2^8) multiplication tables of the column mixing steps.
    void initMulTable();
    void encryptBlock(unsigned char* target, const unsigned char* source);
    void decryptBlock(unsigned char* target, const unsigned char* source);
    void addRoundKey(unsigned char* state, int round);
    void applySBox(unsigned int& word);
    void subBytes(unsigned char* state);
    void shiftRows(unsigned char* state);
    void mixColumns(unsigned char* state);
    void invSubBytes(unsigned char* state);
    void invShiftRows(unsigned char* state);
    void invMixColumns(unsigned char* state);
    //! Does nothing in this build.
    void printVec(const unsigned char* data) const;

    //! The expanded key: 15 round keys, then the unused rest of the last expansion step.
    unsigned char RoundKeys[256];
    //! Products with 2, 3, 9, 11, 13 and 14.
    unsigned char* Mul2;
    unsigned char* Mul3;
    unsigned char* Mul9;
    unsigned char* Mul11;
    unsigned char* Mul13;
    unsigned char* Mul14;
};

} // end namespace core
} // end namespace ox

#endif
