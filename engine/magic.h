#pragma once

#include "types.h"    //  там определены Square, Side и тд
#include "bitboard.h" //  там typedef Bitboard = uint64_t

// public API
void init_magic();
Bitboard rook_attacks(Square sq, Bitboard occ);
Bitboard bishop_attacks(Square sq, Bitboard occ);

// внутренние таблицы (не трогаем)
extern Bitboard RookMask[64], BishopMask[64];
extern Bitboard RookAttack[64][4096];
extern Bitboard BishopAttack[64][512];