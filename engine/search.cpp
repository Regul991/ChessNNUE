// engine/search.cpp
#include "search.h"
#include "order.h"      // move_score()
#include "zobrist.h"
#include "tt.h"
#include "eval.h"       // material_score()
#include <algorithm>

static uint64_t g_nodes = 0;
static constexpr int INF = 30000;

/* ---------------- КВИССЕНСИЯ ---------------- */
static int quiescence(Position& pos, int alpha, int beta)
{
    int stand = material_score(pos);
    if (stand >= beta) return beta;
    if (stand > alpha) alpha = stand;

    std::vector<Move> moves;
    generate_moves(pos, moves);

    /* оставляем только взятия */
    moves.erase(std::remove_if(moves.begin(), moves.end(),
        [&](Move m) { return !(pos.occ[pos.stm ^ 1] & one(to_sq(m))); }),
        moves.end());

    for (Move m : moves) {
        Position nxt;
        pos.make_move(m, nxt);
        ++g_nodes;
        int score = -quiescence(nxt, -beta, -alpha);
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

static constexpr int NULL_MOVE_REDUCTION = 2;

/* ---------------- АЛЬФА-БЕТА ---------------- */
static int alphabeta(Position& pos, int depth, int alpha, int beta)
{

    // 0) Null-move-отсечение
    if (depth >= NULL_MOVE_REDUCTION + 1) {
        // получаем квадрат короля для стороны to-move
        Square kingSq = Square(lsb_index(pos.bb[pos.stm][KING]));
        // только если король не под атакой
        if (!pos.attacked(kingSq, Side(1 - pos.stm))) {
            Position nullPos = pos;
            nullPos.stm = Side(1 - pos.stm);
            nullPos.ep = SQ_NONE;
            int score = -alphabeta(nullPos,
                depth - 1 - NULL_MOVE_REDUCTION,
                -beta, -beta + 1);
            if (score >= beta)
                return beta;
        }
    }

    /* ---- TT probe ---- */
    uint64_t key = Zobrist::hash(pos);
    TT::Entry& e = TT::probe(key);
    if (e.key == key && e.depth >= depth) {
        if (e.flag == TT::EXACT)  return e.score;
        if (e.flag == TT::LOWER && e.score >= beta)  return e.score;
        if (e.flag == TT::UPPER && e.score <= alpha) return e.score;
    }

    if (depth == 0)
        return quiescence(pos, alpha, beta);

    std::vector<Move> moves;
    generate_moves(pos, moves);
    if (moves.empty())                // мат или пат
        return material_score(pos);

    std::stable_sort(moves.begin(), moves.end(),
        [&](Move a, Move b) { return move_score(pos, a) > move_score(pos, b); });

    Move bestMove = 0;
    for (Move m : moves) {
        Position nxt;
        pos.make_move(m, nxt);
        ++g_nodes;
        int score = -alphabeta(nxt, depth - 1, -beta, -alpha);

        if (score > alpha) {
            alpha = score;
            bestMove = m;
            if (score >= beta) break;       // β-срез
        }
    }

    /* ---- TT store ---- */
    e.key = key;
    e.depth = static_cast<int8_t>(depth);
    e.score = static_cast<int16_t>(alpha);
    e.best = bestMove;
    e.flag = (alpha >= beta) ? TT::LOWER
        : (bestMove ? TT::EXACT : TT::UPPER);

    return alpha;
}

/* ------------ ИТЕРАТИВНОЕ УГЛУБЛЕНИЕ ------------ */
SearchResult search(Position& root, int depth)
{
    SearchResult res{ 0, 0, 0 };

    for (int d = 1; d <= depth; ++d)
    {
        g_nodes = 0;

        /* список ходов корня */
        std::vector<Move> moves;
        generate_moves(root, moves);
        std::stable_sort(moves.begin(), moves.end(),
            [&](Move a, Move b) { return move_score(root, a) > move_score(root, b); });

        Move best = 0;
        int  bestScore = -INF;
        int  alpha = -INF, beta = INF;

        for (Move m : moves)
        {
            Position nxt;
            root.make_move(m, nxt);
            ++g_nodes;
            int score = -alphabeta(nxt, d - 1, -beta, -alpha);

            if (score > bestScore) {
                bestScore = score;
                best = m;
            }
        }

        res.best = best;
        res.score = bestScore;
        res.nodes += g_nodes;
    }
    return res;
}
