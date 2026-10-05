// Recovered for Harvest from the Mac and Linux 1.18 builds; not the original source.

#ifndef OX_ALGO_CRAND_H
#define OX_ALGO_CRAND_H

namespace ox {
namespace algo {

//! A multiplicative congruential generator (a = 40692, m = 2147483399) with a shared static
//! sequence and a per-object one.
class CRand
{
public:
    CRand() : Current(0x0f0f0f0f) {}
    //! A sequence of its own starting at seed.
    CRand(int seed) : Current(seed) {}
    virtual ~CRand() {}

    //! The next value of the sequence after seed.
    static int performRand(int seed);

    //! Advances the shared sequence.
    static int rand();
    static void reset();
    static void srand(int seed);

    //! Advances this object's sequence.
    int nextInt();
    //! Advances this object's sequence and returns it modulo max.
    int nextInt(int max);
    void setCurrent(int current);

private:
    int Current;

    static int ms_seed;
};

} // end namespace algo
} // end namespace ox

#endif
