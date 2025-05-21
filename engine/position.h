#pragma once
#include "types.h"
#include <array>
#include "move.h"  
#include <string>

struct Position {

    std::array<std::array<Bitboard, 6>, 2> bb{}; // bb[side][piece]
    Bitboard occ[2]{};
    Bitboard occ_all{};
    Side stm = WHITE;

    int cr = WOO | WOOO | BOO | BOOO;  // castling rights
    Square ep = SQ_NONE;               // en-passant square

    /* ------------- методы ------------- */
    void set_startpos();
    bool attacked(Square sq, Side by) const;
    void make_move(Move m, Position& nxt) const; // копирует позицию + применяет ход
};

bool position_from_fen(Position& p, const std::string& fen);