// engine/search.cpp
#include "search.h"

#include "bitboard.h"
#include "eval.h"
#include "movegen.h"
#include "order.h"
#include "tt.h"
#include "zobrist.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <vector>

/* ----------------------------
   Константы и глобальные буферы
   ---------------------------- */
namespace {

    constexpr int  INF = 30'000;
    constexpr int  MATE_SCORE = 29'000;          // < INF − запас
    constexpr int  ASP_WIN = 30;              // пол-пешки
    constexpr int  NULL_REDUCTION_BASE = 2;
    constexpr int  LMR_MIN_DEPTH = 3;
    constexpr int  MAX_PLY = 64;
    constexpr int  KILLER_SLOTS = 2;
    constexpr uint64_t LOG_INTERVAL = 1'000'000ULL;

    static Move killer[MAX_PLY][KILLER_SLOTS]{};
    static int  hist[64][64]{};
    static uint64_t g_nodes = 0;                // общий счётчик

    /* --- утилиты --- */
    inline bool is_capture(const Position& pos, Move m) {
        return pos.occ[pos.stm ^ 1] & one(to_sq(m));
    }
    inline bool is_check(const Position& pos) {
        Square ksq = Square(lsb_index(pos.bb[pos.stm][KING]));
        return pos.attacked(ksq, Side(pos.stm ^ 1));
    }
    inline void store_killer(int ply, Move m) {
        if (killer[ply][0] != m) {
            killer[ply][1] = killer[ply][0];
            killer[ply][0] = m;
        }
    }

} // namespace

/* ------------------------------
   КВИСЕНСИЯ
   ------------------------------*/
static int quiescence(Position& pos, int alpha, int beta)
{
    int stand = evaluate(pos);
    if (stand >= beta) return beta;
    if (stand > alpha) alpha = stand;

    std::vector<Move> moves;
    generate_moves(pos, moves);                 // мы отфильтруем ниже

    moves.erase(std::remove_if(moves.begin(), moves.end(),
        [&](Move m) {
            bool tact = is_capture(pos, m) || promo_of(m);
            return !tact;
        }), moves.end());

    std::stable_sort(moves.begin(), moves.end(),
        [&](Move a, Move b) { return move_score(pos, a) > move_score(pos, b); });

    for (Move m : moves)
    {
        Position nxt;
        pos.make_move(m, nxt);

        ++g_nodes;
        if ((g_nodes % LOG_INTERVAL) == 0)
            std::cerr << "Progress: nodes=" << g_nodes << "\r";

        int score = -quiescence(nxt, -beta, -alpha);
        if (score >= beta)  return beta;
        if (score > alpha)  alpha = score;
    }
    return alpha;
}

/* -----------------------------
   PVS / alphabeta с TT, Null-Move, LMR
   -----------------------------*/
static int alphabeta(Position& pos, int depth, int alpha, int beta, int ply)
{
    /* 0. mate distance pruning */
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta = std::min(beta, MATE_SCORE - ply - 1);
    if (alpha >= beta) return alpha;

    /* 1. TT */
    uint64_t key = Zobrist::hash(pos);
    TT::Entry& tt = TT::probe(key);
    if (tt.key == key && tt.depth >= depth) {
        if (tt.flag == TT::EXACT)                              return tt.score;
        if (tt.flag == TT::LOWER && tt.score >= beta)          return tt.score;
        if (tt.flag == TT::UPPER && tt.score <= alpha)         return tt.score;
    }

    /* 2. Лист квиссенсии */
    if (depth <= 0)
        return quiescence(pos, alpha, beta);

    /* 3. Null-move pruning */
    if (!is_check(pos) && depth >= 3) {
        Position nullPos = pos;
        nullPos.stm = Side(1 - pos.stm);
        nullPos.ep = SQ_NONE;

        int R = NULL_REDUCTION_BASE + (depth > 6);
        int score = -alphabeta(nullPos, depth - 1 - R, -beta, -beta + 1, ply + 1);
        if (score >= beta)
            return beta;
    }

    /* 4. Генерация и сортировка */
    std::vector<Move> moves;
    generate_moves(pos, moves);
    if (moves.empty()) {                                   // мат или пат
        return is_check(pos) ? -MATE_SCORE + ply : 0;
    }

    Move ttMove = (tt.key == key) ? tt.best : 0;
    std::stable_sort(moves.begin(), moves.end(),
        [&](Move a, Move b)
        {
            if (a == ttMove) return true;
            if (b == ttMove) return false;

            int sa = move_score(pos, a);
            int sb = move_score(pos, b);

            if (a == killer[ply][0]) sa += 1'000'000;
            else if (a == killer[ply][1]) sa += 500'000;

            if (b == killer[ply][0]) sb += 1'000'000;
            else if (b == killer[ply][1]) sb += 500'000;

            sa += hist[from_sq(a)][to_sq(a)];
            sb += hist[from_sq(b)][to_sq(b)];

            return sa > sb;
        });

    /* 5. Перебор */
    Move  bestMove = 0;
    int   bestEval = -INF;
    int   moveNo = 0;

    for (Move m : moves)
    {
        ++moveNo;

        Position nxt;
        pos.make_move(m, nxt);

        ++g_nodes;
        if ((g_nodes % LOG_INTERVAL) == 0)
            std::cerr << "Progress: nodes=" << g_nodes << "\r";

        /* LMR для нетактических и непривилегированных ходов */
        int newDepth = depth - 1;
        bool tactical = is_capture(pos, m) || promo_of(m) || is_check(nxt);
        if (!tactical && depth >= LMR_MIN_DEPTH && moveNo > 3)
            newDepth -= 1;

        int score;
        if (bestMove == 0) {                                // полный окно
            score = -alphabeta(nxt, newDepth, -beta, -alpha, ply + 1);
        }
        else {
            // пробный узкий поиск (PVS)
            score = -alphabeta(nxt, newDepth, -alpha - 1, -alpha, ply + 1);
            if (score > alpha && score < beta)              // не угадали – ресёрч
                score = -alphabeta(nxt, newDepth, -beta, -alpha, ply + 1);
        }

        if (score >= beta) {
            /* бета-отсечение */
            if (!tactical) {
                store_killer(ply, m);
                hist[from_sq(m)][to_sq(m)] += depth * depth;
            }
            tt = { key, int8_t(depth), int16_t(beta), TT::LOWER, m };
            return beta;
        }

        if (score > bestEval) {
            bestEval = score;
            bestMove = m;
            if (score > alpha) alpha = score;
        }
    }

    /* 6. запись в TT */
    tt = { key, int8_t(depth), int16_t(bestEval),
           (bestMove ? TT::EXACT : TT::UPPER), bestMove };
    return bestEval;
}

/* -----------------------------------
   Итеративное углубление + aspiration
   ----------------------------------- */
SearchResult search(Position& root, int maxDepth)
{
    SearchResult res{ 0, 0, 0 };

    std::fill(&killer[0][0], &killer[0][0] + MAX_PLY * KILLER_SLOTS, 0);
    std::fill(&hist[0][0], &hist[0][0] + 64 * 64, 0);

    int alpha = -INF;
    int beta = INF;

    for (int depth = 1; depth <= maxDepth; ++depth)
    {
        g_nodes = 0;

        /* aspiration-окно вокруг прошлого статического */
        if (depth >= 3) {
            alpha = res.score - ASP_WIN;
            beta = res.score + ASP_WIN;
        }
        else {
            alpha = -INF;
            beta = INF;
        }

        while (true)
        {
            int val = alphabeta(root, depth, alpha, beta, 0);
            if (val <= alpha) {           // fail-low
                alpha -= ASP_WIN;
                continue;
            }
            if (val >= beta) {            // fail-high
                beta += ASP_WIN;
                continue;
            }
            /* успех – выходим */
            res.score = val;
            break;
        }

        /* лучший ход храним из TT верхнего уровня */                             // 17.05 #TODO tt.h
        TT::Entry& rootTT = TT::probe(Zobrist::hash(root));
        res.best = rootTT.best;
        res.nodes += g_nodes;
    }

    return res;
}
