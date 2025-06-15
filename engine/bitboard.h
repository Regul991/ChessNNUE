#pragma once
#include "types.h"
#include <array>

/* ——— Файловые/ранговые маски, !!!НЕ ТРОГАТЬ ПОКА РАБОТАЕТ!!! ——— */                           /*если будет ошибка в avx - смотреть сюда*/
inline constexpr Bitboard FILE_A = 0x0101010101010101ULL;
inline constexpr Bitboard FILE_H = 0x8080808080808080ULL;
inline constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
inline constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

/* --- Предвычисленные битборды атак --- */
extern std::array<Bitboard, 64> KnightAtt; // extern означает, что определям эти массивы в другом файле
extern std::array<Bitboard, 64> KingAtt;
extern std::array<Bitboard, 64> PawnAttW;   // белая пешка: захваты
extern std::array<Bitboard, 64> PawnAttB;   // чёрная пешка: захваты

void init_attack_tables();   // вызвать один раз при старте

// ---------------------------------
#ifdef _MSC_VER
#include <intrin.h>
inline int lsb_index(Bitboard b) { unsigned long i; _BitScanForward64(&i, b); return int(i); }
#else
inline int lsb_index(Bitboard b) { return __builtin_ctzll(b); }
#endif
inline Square pop_lsb(Bitboard& b)
{
    int i = lsb_index(b);
    b &= b - 1;
    return Square(i);
}
