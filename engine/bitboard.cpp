#include "bitboard.h"

/* ========== ��������������� ������� ========== */


std::array<Bitboard, 64> KnightAtt, KingAtt, PawnAttW, PawnAttB; // ��������� 4 ���������� �������

static inline bool on_board(int sq) { return sq >= 0 && sq < 64; } // ��������� ��� ����� �������� � [0;63] (static - ��������� ������ � ���� �����

/* ---- �������� ����� ---- */
static constexpr int KNIGHT_D[8] = { 17,15,10,6,-17,-15,-10,-6 }; // �������� �������� ��� ��������� ����� (17 - +2 �� ���������, +1 �� �����������) �� 8x8 �����, ���� ������� �������� ������
static constexpr int KING_D[8] = { 8,1,-8,-1,9,7,-9,-7 };

void init_attack_tables()
{
    for (int s = 0; s < 64; ++s) // ���������� ������ ������� 
    {
        Bitboard bb = 0; // ������� ���������� �������
        /* ���� */ 
        bb = 0;
        for (int d : KNIGHT_D) { // ��� ������� �������� D 
            int to = s + d; // ��������� ������� ������� 
            int f_diff = abs((s % 8) - (to % 8)); // ������� �������, ����� ���� �� ��������� �� ���� �����
            if (on_board(to) && f_diff <= 2) bb |= one(Square(to)); // ��������� ��� ���� �� ����� � �� ������� ������ ������ || ������������� ��� � �������� bb ��� �������� to
        }
        KnightAtt[s] = bb; // ��������� ��������� � ���������� ������ ���� ����

        /* ������ */
        bb = 0;
        for (int d : KING_D) {
            int to = s + d;
            int f_diff = abs((s % 8) - (to % 8));
            if (on_board(to) && f_diff <= 1) bb |= one(Square(to));
        }
        KingAtt[s] = bb;

        /* ����� */
        PawnAttW[s] = 0;
        PawnAttB[s] = 0;
        int r = s / 8, f = s % 8;
        if (r < 7) {
            if (f > 0) PawnAttW[s] |= one(Square(s + 7));
            if (f < 7) PawnAttW[s] |= one(Square(s + 9));
        }
        if (r > 0) {
            if (f > 0) PawnAttB[s] |= one(Square(s - 9));
            if (f < 7) PawnAttB[s] |= one(Square(s - 7));
        }
    }
}
// ���������� � �����, ����� ����� ��� ������ +1 +2 ���� �� �������