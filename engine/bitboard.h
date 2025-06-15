#pragma once
#include "types.h"
#include <array>

/* ��� ��������/�������� �����, !!!�� ������� ���� ��������!!! ��� */                           /*���� ����� ������ � avx - �������� ����*/
inline constexpr Bitboard FILE_A = 0x0101010101010101ULL;
inline constexpr Bitboard FILE_H = 0x8080808080808080ULL;
inline constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
inline constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

/* --- ��������������� �������� ���� --- */
extern std::array<Bitboard, 64> KnightAtt; // extern ��������, ��� ��������� ��� ������� � ������ �����
extern std::array<Bitboard, 64> KingAtt;
extern std::array<Bitboard, 64> PawnAttW;   // ����� �����: �������
extern std::array<Bitboard, 64> PawnAttB;   // ������ �����: �������

void init_attack_tables();   // ������� ���� ��� ��� ������

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
