#include "zobrist.h"
#include <cstdint>

static uint64_t splitmix64(uint64_t& x)
{
    x += 0x9e3779b97f4a7c15ULL;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void Zobrist::init()
{
    uint64_t seed = 20250601;               // фиксованный → детерминизм
    for (int s = 0; s < 64; ++s)
        for (int c = 0; c < 2; ++c)
            for (int p = 0; p < 6; ++p)
                R[c][p][s] = splitmix64(seed);

    for (int i = 0; i < 16; ++i) CASTLE[i] = splitmix64(seed);
    for (int f = 0; f < 8; ++f)  EP[f] = splitmix64(seed);
    SIDE = splitmix64(seed);
}

uint64_t Zobrist::hash(const Position& pos)
{
    uint64_t h = 0;
    for (int s = 0; s < 64; ++s) {
        Bitboard sq = one(Square(s));
        for (int p = 0; p < 6; ++p) {
            if (pos.bb[WHITE][p] & sq) h ^= R[WHITE][p][s];
            if (pos.bb[BLACK][p] & sq) h ^= R[BLACK][p][s];
        }
    }
    h ^= CASTLE[pos.cr & 0xF];
    if (pos.ep != SQ_NONE) h ^= EP[pos.ep & 7];
    if (pos.stm == BLACK)  h ^= SIDE;
    return h;
}