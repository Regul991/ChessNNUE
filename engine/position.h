#pragma once
#include "types.h"
#include <array>
#include "move.h"  
#include <string>

struct Position {
    // �������� ��� ������ ������� � ������� ���� ������: bb[WHITE/BLACK][PAWN�KING]
    std::array<std::array<Bitboard, 6>, 2> bb{}; // bb[side][piece]
    Bitboard occ[2]{};            // ������������� �������� ������� ���� ����� ���������� �������
    Bitboard occ_all{};           // � ����� ������� ������� ������
    Side stm = WHITE;             // �������, ��� �����: WHITE ��� BLACK

    int cr = WOO | WOOO | BOO | BOOO;  // castling rights
    Square ep = SQ_NONE;               // en-passant square

    /* ------------- ������ ------------- */
    void set_startpos();
    bool attacked(Square sq, Side by) const; // ���������, ��������� �� ������� sq ������� ������� by
    void make_move(Move m, Position& nxt) const; // �������� ������� + ��������� ���
    // �������� ������� ������� � nxt, ��������� ��� m:
    // ��������� bb, occ, occ_all,
    // ������ stm,
    // ���������� ��� ������������� ep,
    // ��������� ����� ��������� � cr.
};

bool position_from_fen(Position& p, const std::string& fen);