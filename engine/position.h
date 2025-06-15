#pragma once
#include "types.h"
#include <array>
#include "move.h"  
#include <string>

struct Position {
    // битборды для каждой стороны и каждого типа фигуры: bb[WHITE/BLACK][PAWN…KING]
    std::array<std::array<Bitboard, 6>, 2> bb{}; // bb[side][piece]
    Bitboard occ[2]{};            // дополнительно хранится битборд всех фигур конкретной стороны
    Bitboard occ_all{};           // и общий битборд занятых клеток
    Side stm = WHITE;             // сторона, что ходит: WHITE или BLACK

    int cr = WOO | WOOO | BOO | BOOO;  // castling rights
    Square ep = SQ_NONE;               // en-passant square

    /* ------------- методы ------------- */
    void set_startpos();
    bool attacked(Square sq, Side by) const; // Проверяет, атакуется ли квадрат sq фигурой стороны by
    void make_move(Move m, Position& nxt) const; // копирует позицию + применяет ход
    // Копирует текущую позицию в nxt, применяет ход m:
    // обновляет bb, occ, occ_all,
    // меняет stm,
    // сбрасывает или устанавливает ep,
    // обновляет права рокировки в cr.
};

bool position_from_fen(Position& p, const std::string& fen);