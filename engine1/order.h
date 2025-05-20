#pragma once
#include "move.h"
#include "position.h"

inline int move_score(const Position& pos, Move m)
{
    Square to = to_sq(m);

    /* захват? */
    if (pos.occ[pos.stm ^ 1] & one(to))
    {
        /* определить нападающую и бьющую фигуры */
        PieceType attacker = NO_PIECE, victim = NO_PIECE;
        for (int t = 0; t < 6; ++t) {
            if (pos.bb[pos.stm][t] & one(from_sq(m))) attacker = (PieceType)t;
            if (pos.bb[pos.stm ^ 1][t] & one(to))     victim = (PieceType)t;
        }
        return 10'000 + mvv_lva_score(victim, attacker); // захваты всегда старше
    }
    return 0;  // обычный ход
}