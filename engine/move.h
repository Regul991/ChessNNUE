#pragma once
#include "types.h"
#include <cstdint>
#include <string>

using Move = uint32_t;          // 0-15: from, 6-11: to, 12-14: промо-фигура (0 = нет)

enum Promo : int { NO_PR = 0, PRN = KNIGHT, PRB = BISHOP, PRR = ROOK, PRQ = QUEEN };

constexpr Move make_move(Square from, Square to, int promo = 0)
{
    return (from) | (to << 6) | (promo << 12);
}
inline constexpr Square from_sq(Move m) { return Square(m & 0x3F); }
inline constexpr Square to_sq(Move m) { return Square((m >> 6) & 0x3F); }
inline constexpr int promo_of(Move m) { return  (m >> 12) & 7; }

// =========== popcount (64-бит) ===========
#ifdef _MSC_VER          // MSVC
#include <intrin.h>
inline int popcount(Bitboard b) { return (int)__popcnt64(b); }
#else                    // GCC/Clang
inline int popcount(Bitboard b) { return __builtin_popcountll(b); }
#endif

// =========== перевод Move → "e2e4" ===========
inline std::string uci_move(Move m)
{
    Square f = from_sq(m), t = to_sq(m);
    char s[6];                                   // 4 символа + '\0' (+ промо)
    s[0] = 'a' + (f & 7);
    s[1] = '1' + (f >> 3);
    s[2] = 'a' + (t & 7);
    s[3] = '1' + (t >> 3);
    int promo = promo_of(m);
    if (promo) {
        static const char pc[6] = { ' ', 'n', 'b', 'r', 'q', 'k' };
        s[4] = pc[promo];
        s[5] = '\0';
        return std::string(s, 5);
    }
    s[4] = '\0';
    return std::string(s, 4);
}

/*-------------------------------------------------------------
 *  MVV/LVA — Most Valuable Victim / Least Valuable Attacker
 *------------------------------------------------------------*/
inline int mvv_lva_score(PieceType victim, PieceType attacker)
{
    static constexpr int V[6] = { 100, 320, 330, 500, 900, 20000 }; // K=бесконечность
    return V[victim] - attacker;   // чем больше, тем «лучше» захват
}
