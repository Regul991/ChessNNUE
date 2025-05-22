


// engine/movegen.cpp 
#include "movegen.h"
#include "bitboard.h"
#include <cstdlib>   // abs
#include <array>
#include <vector>
#include "magic.h"

/* --------------------------------------------------------
 *  Утилиты сдвигов и маски столбцов/рядов
 * --------------------------------------------------------*/
static inline Bitboard north(Bitboard b) { return b << 8; }
static inline Bitboard south(Bitboard b) { return b >> 8; }
static inline Bitboard northtwo(Bitboard b) { return b << 16; }
static inline Bitboard southtwo(Bitboard b) { return b >> 16; }


/* Направления слайдеров */
static const int DIR_ORTH[4] = { 8, -8, 1, -1 };          // вертикаль/горизонталь
static const int DIR_DIAG[4] = { 9, 7, -9, -7 };          // диагонали

/* --------------------------------------------------------
 *  Вспомогательная функция «толкнуть» слайдер (слон/ладья/ферзь)
 * --------------------------------------------------------*/
static void push_slider(const Position& pos, Side us, Square from,
    const int* dirs, std::vector<Move>& list)
{
    Bitboard own = pos.occ[us];

    for (int k = 0; k < 4; ++k)
    {
        int d = dirs[k];
        int s = from;

        while (true)
        {
            int f = s & 7;          // столбец 0..7
            int r = s >> 3;         // ряд 0..7

            /* Проверяем выход за доску */
            if ((d == 1 && f == 7) || (d == -1 && f == 0) ||
                (d == 9 && (f == 7 || r == 7)) ||
                (d == 7 && (f == 0 || r == 7)) ||
                (d == -7 && (f == 7 || r == 0)) ||
                (d == -9 && (f == 0 || r == 0)) ||
                (d == 8 && r == 7) || (d == -8 && r == 0))
                break;

            s += d;
            Bitboard sq = one(Square(s));

            if (sq & own) break;            // своя фигура – луч обрывается

            list.push_back(make_move(from, Square(s)));

            if (sq & pos.occ[us ^ 1]) break; // взяли чужую – дальше не идём
        }
    }
}

/* --------------------------------------------------------
 *  Хелпер: добавляет либо один обычный ход, либо 4 хода‑промоции
 * --------------------------------------------------------*/
static inline void push_pawn_move(std::vector<Move>& list,
    Square from, Square to, bool is_promotion)
{
    if (!is_promotion)
    {
        list.push_back(make_move(from, to));
        return;
    }

    // 4 стандартные фигуры‑промоции
    list.push_back(make_move(from, to, QUEEN));
    list.push_back(make_move(from, to, ROOK));
    list.push_back(make_move(from, to, BISHOP));
    list.push_back(make_move(from, to, KNIGHT));
}

/* --------------------------------------------------------
 *  Генерация ПСЕВДОЛЕГАЛЬНЫХ ходов (по правилам ходов, без учёта шаха)
 * --------------------------------------------------------*/
static void generate_pseudo(const Position& pos, std::vector<Move>& list)
{
    list.clear();
    const Side us = pos.stm;
    const Side them = Side(us ^ 1);

    /* ---------------- Пешки ---------------- */
    Bitboard pawns = pos.bb[us][PAWN];
    const Bitboard empty = ~pos.occ_all;

    if (us == WHITE)
    {
        /* ---- простой шаг вперёд ---- */
        Bitboard oneStep = north(pawns) & empty;
        Bitboard bb = oneStep;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to - 8);
            bool promo = (to >> 3) == 7;        // достигли 8‑й горизонтали
            push_pawn_move(list, from, to, promo);
        }

        /* ---- двойной шаг с 2‑й линии ---- */
        Bitboard single = oneStep;               // уже вычислено
        Bitboard rank4 = 0x00000000FF000000ULL; // 4‑я горизонталь (a4‑h4)
        Bitboard doubleStep = north(single) & empty & rank4;

        bb = doubleStep;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to - 16);
            list.push_back(make_move(from, to)); // двойной ход никогда не промоция
        }

        /* ---- захваты влево / вправо ---- */
        Bitboard capL = (pawns << 7) & pos.occ[them] & ~FILE_H;
        Bitboard capR = (pawns << 9) & pos.occ[them] & ~FILE_A;

        bb = capL;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to - 7);
            bool promo = (to >> 3) == 7;
            push_pawn_move(list, from, to, promo);
        }
        bb = capR;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to - 9);
            bool promo = (to >> 3) == 7;
            push_pawn_move(list, from, to, promo);
        }
    }
    else /* ---------------- ЧЕРНЫЕ Пешки ---------------- */
    {
        Bitboard oneStep = south(pawns) & empty;
        Bitboard bb = oneStep;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to + 8);
            bool promo = (to >> 3) == 0;        // достигли 1‑й горизонтали
            push_pawn_move(list, from, to, promo);
        }

        /* ---- двойной шаг с 7-й линии ---- */
        Bitboard singleB = oneStep;
        Bitboard rank5 = 0x000000FF00000000ULL; // 5‑я горизонталь (a5‑h5)
        Bitboard doubleStepB = south(singleB) & empty & rank5;

        bb = doubleStepB;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to + 16);
            list.push_back(make_move(from, to));
        }

        /* ---- захваты ---- */
        Bitboard capL = (pawns >> 9) & pos.occ[them] & ~FILE_H;
        Bitboard capR = (pawns >> 7) & pos.occ[them] & ~FILE_A;

        bb = capL;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to + 9);
            bool promo = (to >> 3) == 0;
            push_pawn_move(list, from, to, promo);
        }
        bb = capR;
        while (bb)
        {
            Square to = pop_lsb(bb);
            Square from = Square(to + 7);
            bool promo = (to >> 3) == 0;
            push_pawn_move(list, from, to, promo);
        }
    }

    /* ---------------- Кони ---------------- */
    Bitboard knights = pos.bb[us][KNIGHT];
    while (knights)
    {
        Square from = pop_lsb(knights);
        Bitboard targets = KnightAtt[from] & ~pos.occ[us];
        Bitboard bbK = targets;
        while (bbK)
        {
            Square to = pop_lsb(bbK);
            list.push_back(make_move(from, to));
        }
    }

    /* ---------------- Слоны ---------------- */
    Bitboard bishops = pos.bb[us][BISHOP];
    while (bishops) {
        Square from = pop_lsb(bishops);
        Bitboard attacks = bishop_attacks(from, pos.occ_all) & ~pos.occ[us];
        while (attacks) {
            Square to = pop_lsb(attacks);
            list.push_back(make_move(from, to));
        }
    }

    /* ---------------- Ладьи ---------------- */
    Bitboard rooks = pos.bb[us][ROOK];
    while (rooks) {
        Square from = pop_lsb(rooks);
        Bitboard attacks = rook_attacks(from, pos.occ_all) & ~pos.occ[us];
        while (attacks) {
            Square to = pop_lsb(attacks);
            list.push_back(make_move(from, to));
        }
    }

    /* ---------------- Ферзи ---------------- */
    Bitboard queens = pos.bb[us][QUEEN];
    while (queens) {
        Square from = pop_lsb(queens);
        Bitboard attacks =
            (bishop_attacks(from, pos.occ_all) |
                rook_attacks(from, pos.occ_all))
            & ~pos.occ[us];
        while (attacks) {
            Square to = pop_lsb(attacks);
            list.push_back(make_move(from, to));
        }
    }

    /* ---------------- Король ---------------- */
    Square ksq = Square(lsb_index(pos.bb[us][KING]));
    Bitboard kTargets = KingAtt[ksq] & ~pos.occ[us];
    Bitboard bbK = kTargets;
    while (bbK)
    {
        Square to = pop_lsb(bbK);
        list.push_back(make_move(ksq, to));
    }

    /* --------- Рокировка --------- */
    if (us == WHITE)
    {
        if ((pos.cr & WOO) &&
            !(pos.occ_all & (one(F1) | one(G1))) &&
            !pos.attacked(E1, them) && !pos.attacked(F1, them) && !pos.attacked(G1, them))
            list.push_back(make_move(E1, G1));

        if ((pos.cr & WOOO) &&
            !(pos.occ_all & (one(B1) | one(C1) | one(D1))) &&
            !pos.attacked(E1, them) && !pos.attacked(D1, them) && !pos.attacked(C1, them))
            list.push_back(make_move(E1, C1));
    }
    else
    {
        if ((pos.cr & BOO) &&
            !(pos.occ_all & (one(F8) | one(G8))) &&
            !pos.attacked(E8, them) && !pos.attacked(F8, them) && !pos.attacked(G8, them))
            list.push_back(make_move(E8, G8));

        if ((pos.cr & BOOO) &&
            !(pos.occ_all & (one(B8) | one(C8) | one(D8))) &&
            !pos.attacked(E8, them) && !pos.attacked(D8, them) && !pos.attacked(C8, them))
            list.push_back(make_move(E8, C8));
    }

    /* --------- En‑passant --------- */
    if (pos.ep != SQ_NONE)
    {
        Bitboard epAttackers = (us == WHITE) ? PawnAttB[pos.ep] : PawnAttW[pos.ep];
        Bitboard pawnsCan = epAttackers & pos.bb[us][PAWN];
        while (pawnsCan)
        {
            Square from = pop_lsb(pawnsCan);
            list.push_back(make_move(from, pos.ep));
        }
    }
}

/* --------------------------------------------------------
 *  Генерация ЛЕГАЛЬНЫХ ходов (отсеиваем шах своему королю)
 * --------------------------------------------------------*/
void generate_moves(const Position& pos, std::vector<Move>& legal)
{
    std::vector<Move> pseudo;
    generate_pseudo(pos, pseudo);

    legal.clear();
    Position nxt;
    for (Move m : pseudo)
    {
        pos.make_move(m, nxt);
        Side us = pos.stm;
        Square ksq = Square(lsb_index(nxt.bb[us][KING]));
        if (!nxt.attacked(ksq, nxt.stm))
            legal.push_back(m);
    }
}


