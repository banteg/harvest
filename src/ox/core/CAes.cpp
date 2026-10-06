// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#include "CAes.h"
#include <cstring>
// The native unit has an iostream static initializer, as do the other ox units.
#include <iostream> // IWYU pragma: keep

namespace ox {
namespace core {

//! Rounds of AES-256.
static const int ROUNDS = 14;
static const int BLOCK_SIZE = 16;

const unsigned char RIJNDAEL_S_BOX[256] =
{
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

const unsigned char INVERSE_RIJNDAEL_S_BOX[256] =
{
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
};

CAes::CAes()
{
    Mul2 = 0;
    Mul3 = 0;
    Mul9 = 0;
    Mul11 = 0;
    Mul13 = 0;
    Mul14 = 0;
}

CAes::~CAes()
{
    delete [] Mul2;
    delete [] Mul3;
    delete [] Mul9;
    delete [] Mul11;
    delete [] Mul13;
    delete [] Mul14;
    Mul2 = 0;
    Mul3 = 0;
    Mul9 = 0;
    Mul11 = 0;
    Mul13 = 0;
    Mul14 = 0;
}

int CAes::getEncryptedDataSize(int size) const
{
    return ((size >> 4) + ((size & 15) != 0)) << 4;
}

int CAes::getValidKeyLength() const
{
    return 32;
}

bool CAes::encrypt(void* target, const void* source, int size)
{
    if (!Key)
        return false;

    if (!EncryptKeyReady)
    {
        expandKey();
        initMulTable();
        EncryptKeyReady = true;
    }

    int blocks = size / BLOCK_SIZE;
    int i;
    for (i = 0; i < blocks; ++i)
        encryptBlock((unsigned char*)target + i * BLOCK_SIZE, (const unsigned char*)source + i * BLOCK_SIZE);

    if (size % BLOCK_SIZE != 0)
    {
        unsigned char last[BLOCK_SIZE];
        memset(last, 0, BLOCK_SIZE);
        memcpy(last, (const unsigned char*)source + i * BLOCK_SIZE, size % BLOCK_SIZE);
        encryptBlock((unsigned char*)target + i * BLOCK_SIZE, last);
    }
    return true;
}

void CAes::expandKey()
{
#ifdef HARVEST_PORT
    // RoundKeys follows the cipher's two flags, so its words are misaligned (offset 18 in the
    // 64-bit builds) and UBSan traps on the loads below. The port expands the key in an aligned
    // copy.
    unsigned int words[sizeof(RoundKeys) / sizeof(unsigned int)];
    memcpy(words, Key->getKey(), 32);

    unsigned int* w = words;
#else
    memcpy(RoundKeys, Key->getKey(), 32);

    unsigned int* w = (unsigned int*)RoundKeys;
#endif
    unsigned int temp;
    for (int i = 1; i < 8; ++i)
    {
        int k = i * 8;
        temp = w[k - 1];
        temp = (temp >> 8) | ((unsigned char)temp << 24);
        applySBox(temp);
        temp ^= (1 << (i - 1)) & 0xff;
        w[k] = w[k - 8] ^ temp;
        for (int j = k + 1; j < k + 4; ++j)
        {
            temp = w[j - 1];
            temp ^= w[j - 8];
            w[j] = temp;
        }

        temp = w[k + 3];
        applySBox(temp);
        temp ^= w[k - 4];
        w[k + 4] = temp;
        for (int j = k + 5; j < k + 8; ++j)
        {
            temp = w[j - 1];
            temp ^= w[j - 8];
            w[j] = temp;
        }
    }
#ifdef HARVEST_PORT
    memcpy(RoundKeys, words, sizeof(RoundKeys));
#endif
}

//! Multiplication by 2 in GF(2^8).
static inline unsigned char xtime(unsigned char x)
{
    unsigned char result = x << 1;
    if (x & 0x80)
        result ^= 0x1b;
    return result;
}

void CAes::initMulTable()
{
    delete [] Mul2;
    delete [] Mul3;
    delete [] Mul9;
    delete [] Mul11;
    delete [] Mul13;
    delete [] Mul14;
    Mul2 = 0;
    Mul3 = 0;
    Mul9 = 0;
    Mul11 = 0;
    Mul13 = 0;
    Mul14 = 0;

    Mul2 = new unsigned char[256];
    Mul3 = new unsigned char[256];
    Mul9 = new unsigned char[256];
    Mul11 = new unsigned char[256];
    Mul13 = new unsigned char[256];
    Mul14 = new unsigned char[256];

    for (int i = 0; i < 256; ++i)
    {
        unsigned char x2 = xtime(i);
        unsigned char x4 = xtime(x2);
        unsigned char x8 = xtime(x4);
        Mul2[i] = x2;
        Mul3[i] = x2 ^ i;
        Mul9[i] = x8 ^ i;
        Mul11[i] = x8 ^ x2 ^ i;
        Mul13[i] = x8 ^ x4 ^ i;
        Mul14[i] = x8 ^ x4 ^ x2;
    }
}

void CAes::encryptBlock(unsigned char* target, const unsigned char* source)
{
    memcpy(target, source, BLOCK_SIZE);
    addRoundKey(target, 0);
    for (int round = 1; round <= ROUNDS; ++round)
    {
        subBytes(target);
        shiftRows(target);
        if (round < ROUNDS)
            mixColumns(target);
        addRoundKey(target, round);
    }
}

bool CAes::decrypt(void* target, const void* source, int size)
{
    if (!Key || size % BLOCK_SIZE != 0)
        return false;

    if (!EncryptKeyReady)
    {
        expandKey();
        initMulTable();
        EncryptKeyReady = true;
    }

    unsigned char* out = (unsigned char*)target;
    const unsigned char* in = (const unsigned char*)source;
    int blocks = size / BLOCK_SIZE;
    for (int i = 0; i < blocks; ++i)
        decryptBlock(out + i * BLOCK_SIZE, in + i * BLOCK_SIZE);
    return true;
}

void CAes::decryptBlock(unsigned char* target, const unsigned char* source)
{
    memcpy(target, source, BLOCK_SIZE);
    addRoundKey(target, ROUNDS);
    for (int round = 1; round <= ROUNDS; ++round)
    {
        invShiftRows(target);
        invSubBytes(target);
        addRoundKey(target, ROUNDS - round);
        if (round < ROUNDS)
            invMixColumns(target);
    }
}

const wchar_t* CAes::getShortName() const
{
    return L"AES";
}

const wchar_t* CAes::getLongName() const
{
    return L"Advanced Encryption Standard";
}

bool CAes::isKeyValid(const CCipherKey* key) const
{
    return key->getKeyLength() == 32;
}

void CAes::addRoundKey(unsigned char* state, int round)
{
    const unsigned char* key = RoundKeys + round * BLOCK_SIZE;
    state[0] ^= key[0];
    state[1] ^= key[1];
    state[2] ^= key[2];
    state[3] ^= key[3];
    state[4] ^= key[4];
    state[5] ^= key[5];
    state[6] ^= key[6];
    state[7] ^= key[7];
    state[8] ^= key[8];
    state[9] ^= key[9];
    state[10] ^= key[10];
    state[11] ^= key[11];
    state[12] ^= key[12];
    state[13] ^= key[13];
    state[14] ^= key[14];
    state[15] ^= key[15];
}

void CAes::applySBox(unsigned int& word)
{
    unsigned char* bytes = (unsigned char*)&word;
    for (int i = 0; i < 4; ++i)
        bytes[i] = RIJNDAEL_S_BOX[bytes[i]];
}

void CAes::subBytes(unsigned char* state)
{
    for (int i = 0; i < BLOCK_SIZE; ++i)
        state[i] = RIJNDAEL_S_BOX[state[i]];
}

void CAes::shiftRows(unsigned char* state)
{
    unsigned char temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;

    temp = state[2];
    unsigned char temp2 = state[6];
    state[2] = state[10];
    state[6] = state[14];
    state[10] = temp;
    state[14] = temp2;

    temp = state[3];
    temp2 = state[7];
    unsigned char temp3 = state[11];
    state[3] = state[15];
    state[7] = temp;
    state[11] = temp2;
    state[15] = temp3;
}

void CAes::mixColumns(unsigned char* state)
{
    for (int c = 0; c < 4; ++c)
    {
        unsigned char a0 = state[c * 4];
        unsigned char a1 = state[c * 4 + 1];
        unsigned char a2 = state[c * 4 + 2];
        unsigned char a3 = state[c * 4 + 3];
        unsigned char b0 = Mul2[a0] ^ Mul3[a1] ^ a2 ^ a3;
        unsigned char b1 = a0 ^ Mul2[a1] ^ Mul3[a2] ^ a3;
        unsigned char b2 = a0 ^ a1 ^ Mul2[a2] ^ Mul3[a3];
        unsigned char b3 = Mul3[a0] ^ a1 ^ a2 ^ Mul2[a3];
        state[c * 4] = b0;
        state[c * 4 + 1] = b1;
        state[c * 4 + 2] = b2;
        state[c * 4 + 3] = b3;
    }
}

void CAes::invSubBytes(unsigned char* state)
{
    for (int i = 0; i < BLOCK_SIZE; ++i)
        state[i] = INVERSE_RIJNDAEL_S_BOX[state[i]];
}

void CAes::invShiftRows(unsigned char* state)
{
    unsigned char temp = state[13];
    state[13] = state[9];
    state[9] = state[5];
    state[5] = state[1];
    state[1] = temp;

    temp = state[14];
    unsigned char temp2 = state[10];
    state[14] = state[6];
    state[10] = state[2];
    state[6] = temp;
    state[2] = temp2;

    temp = state[7];
    temp2 = state[11];
    unsigned char temp3 = state[15];
    state[15] = state[3];
    state[11] = temp3;
    state[7] = temp2;
    state[3] = temp;
}

void CAes::invMixColumns(unsigned char* state)
{
    for (int c = 0; c < 4; ++c)
    {
        unsigned char a0 = state[c * 4];
        unsigned char a1 = state[c * 4 + 1];
        unsigned char a2 = state[c * 4 + 2];
        unsigned char a3 = state[c * 4 + 3];
        unsigned char b0 = Mul14[a0] ^ Mul11[a1] ^ Mul13[a2] ^ Mul9[a3];
        unsigned char b1 = Mul9[a0] ^ Mul14[a1] ^ Mul11[a2] ^ Mul13[a3];
        unsigned char b2 = Mul13[a0] ^ Mul9[a1] ^ Mul14[a2] ^ Mul11[a3];
        unsigned char b3 = Mul11[a0] ^ Mul13[a1] ^ Mul9[a2] ^ Mul14[a3];
        state[c * 4] = b0;
        state[c * 4 + 1] = b1;
        state[c * 4 + 2] = b2;
        state[c * 4 + 3] = b3;
    }
}

void CAes::printVec(const unsigned char* data) const
{
}

const wchar_t* ICipher::getShortName() const
{
    return L"";
}

const wchar_t* ICipher::getLongName() const
{
    return L"";
}

} // end namespace core
} // end namespace ox
