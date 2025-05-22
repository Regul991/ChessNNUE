#pragma once

#include "types.h"    //  ��� ���������� Square, Side � ��
#include "bitboard.h" //  ��� typedef Bitboard = uint64_t

// public API
void init_magic();
Bitboard rook_attacks(Square sq, Bitboard occ);
Bitboard bishop_attacks(Square sq, Bitboard occ);

// ���������� ������� (�� �������)
extern Bitboard RookMask[64], BishopMask[64];
extern Bitboard RookAttack[64][4096];
extern Bitboard BishopAttack[64][512];