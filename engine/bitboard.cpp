#include "bitboard.h"

/* ========== Вспомогательные макросы ========== */


std::array<Bitboard, 64> KnightAtt, KingAtt, PawnAttW, PawnAttB; // Объявляем 4 глобальных массива

static inline bool on_board(int sq) { return sq >= 0 && sq < 64; } // Проверяем что номер квадрата в [0;63] (static - видимость только в этом файле

/* ---- Смещения фигур ---- */
static constexpr int KNIGHT_D[8] = { 17,15,10,6,-17,-15,-10,-6 }; // Смещения индексов для возможных ходов (17 - +2 по вертикали, +1 по горизонтали) на 8x8 доске, если считать квадраты подряд
static constexpr int KING_D[8] = { 8,1,-8,-1,9,7,-9,-7 };

void init_attack_tables()
{
    for (int s = 0; s < 64; ++s) // Перебираем каждый квадрат 
    {
        Bitboard bb = 0; // Очищаем веременный битборт
        /* Конь */ 
        bb = 0;
        for (int d : KNIGHT_D) { // Для каждого смещения D 
            int to = s + d; // Вычисляем целевой квадрат 
            int f_diff = abs((s % 8) - (to % 8)); // Считаем разницу, чтобы конь не перебежал за край доски
            if (on_board(to) && f_diff <= 2) bb |= one(Square(to)); // Проверяем что конь на доске и не слишком далеко убежал || устанавливаем бит в битборде bb для квадрата to
        }
        KnightAtt[s] = bb; // Сохраняем результат в глобальный массив атак коня

        /* Король */
        bb = 0;
        for (int d : KING_D) {
            int to = s + d;
            int f_diff = abs((s % 8) - (to % 8));
            if (on_board(to) && f_diff <= 1) bb |= one(Square(to));
        }
        KingAtt[s] = bb;

        /* Пешки */
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
// Аналогично с конем, кроме пешек тут просто +1 +2 если по сдвигам