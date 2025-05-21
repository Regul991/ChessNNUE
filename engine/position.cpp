#include "position.h"
#include "bitboard.h"
#include "move.h"
#include <sstream> 


/* атакует ли сторона поле sq? */
bool Position::attacked(Square sq, Side by) const
{
    /* пешки */
    if (by == WHITE && (PawnAttB[sq] & bb[WHITE][PAWN])) return true;
    if (by == BLACK && (PawnAttW[sq] & bb[BLACK][PAWN])) return true;
    /* кони / король из таблиц */
    if (KnightAtt[sq] & bb[by][KNIGHT]) return true;
    if (KingAtt[sq] & bb[by][KING])   return true;

    /* слон / ферзь по диагонали */
    const int diagDir[4] = { 9, 7, -7, -9 };
    for (int d : diagDir)
    {
        int s2 = sq;
        while (true)
        {
            int f = s2 & 7, r = s2 >> 3;
            if ((d == 9 && (f == 7 || r == 7)) ||
                (d == 7 && (f == 0 || r == 7)) ||
                (d == -7 && (f == 7 || r == 0)) ||
                (d == -9 && (f == 0 || r == 0)))
                break;

            s2 += d;
            Bitboard b = one(Square(s2));

            if (b & occ_all) {
                if (b & (bb[by][BISHOP] | bb[by][QUEEN])) return true;
                break;                          // блокирует любая фигура
            }
        }
    }

    /* ладья / ферзь по прямой */
    const int ortDir[4] = { 8, -8, 1, -1 };
    for (int d : ortDir)
    {
        int s2 = sq;
        while (true)
        {
            int f = s2 & 7, r = s2 >> 3;
            if ((d == 1 && f == 7) || (d == -1 && f == 0) ||
                (d == 8 && r == 7) || (d == -8 && r == 0))
                break;

            s2 += d;
            Bitboard b = one(Square(s2));

            if (b & occ_all) {
                if (b & (bb[by][ROOK] | bb[by][QUEEN])) return true;
                break;
            }
        }
    }
    return false;
}


/*---------- применение хода (без «unmake») ----------*/
void Position::make_move(Move m, Position& nxt) const
{
    nxt = *this;                       // копия
    Square from = from_sq(m);
    Square to = to_sq(m);
    int promo = promo_of(m);

    Side us = this->stm;
    Side them = Side(us ^ 1);

    /* определить тип фигуры, снять с from */
    PieceType pt = NO_PIECE;
    for (int t = 0; t < 6; ++t)
        if (bb[us][t] & one(from)) { pt = PieceType(t); break; }

    nxt.bb[us][pt] ^= one(from);
    nxt.occ[us] ^= one(from);

    /* взятие? */
    for (int t = 0; t < 6; ++t)
        if (nxt.bb[them][t] & one(to)) {
            nxt.bb[them][t] ^= one(to);
            nxt.occ[them] ^= one(to);
            break;
        }
    /* --- если съели угловую ладью делитаем право рокировки --- */
    if (to == H1) nxt.cr &= ~WOO;
    else if (to == A1) nxt.cr &= ~WOOO;
    else if (to == H8) nxt.cr &= ~BOO;
    else if (to == A8) nxt.cr &= ~BOOO;

    /* en-passant взятие */
    if (pt == PAWN && to == ep && ep != SQ_NONE) {
        Square cap = (us == WHITE) ? Square(to - 8) : Square(to + 8);
        nxt.bb[them][PAWN] ^= one(cap);
        nxt.occ[them] ^= one(cap);
    }

    /* поместить фигуру на to */
    PieceType final_pt = promo ? PieceType(promo) : pt;
    nxt.bb[us][final_pt] |= one(to);
    nxt.occ[us] |= one(to);

    /* обновляем битборды занятости */
    nxt.occ_all = nxt.occ[WHITE] | nxt.occ[BLACK];

    /* рокировочные права */
    if (pt == KING) {
        nxt.cr &= (us == WHITE) ? ~(WOO | WOOO) : ~(BOO | BOOO);
        /* рокировка: перемещаем ладью */
        if (from == E1 && to == G1) { nxt.bb[WHITE][ROOK] ^= one(H1) | one(F1); nxt.occ[WHITE] ^= one(H1) | one(F1); }
        if (from == E1 && to == C1) { nxt.bb[WHITE][ROOK] ^= one(A1) | one(D1); nxt.occ[WHITE] ^= one(A1) | one(D1); }
        if (from == E8 && to == G8) { nxt.bb[BLACK][ROOK] ^= one(H8) | one(F8); nxt.occ[BLACK] ^= one(H8) | one(F8); }
        if (from == E8 && to == C8) { nxt.bb[BLACK][ROOK] ^= one(A8) | one(D8); nxt.occ[BLACK] ^= one(A8) | one(D8); }
    }
    if (pt == ROOK) {
        if (from == H1) nxt.cr &= ~WOO;
        if (from == A1) nxt.cr &= ~WOOO;
        if (from == H8) nxt.cr &= ~BOO;
        if (from == A8) nxt.cr &= ~BOOO;
    }

    /* en-passant поле -------------------------------------------------- */
    nxt.ep = SQ_NONE;

    if (pt == PAWN && abs(to - from) == 16)         // двойной шаг
    {
        Square e = (us == WHITE) ? Square(from + 8)   // поле, через которое перепрыгнули
            : Square(from - 8);

        bool can_ep = false;
        if (us == WHITE) {           // смотрим ЧЕРНЫЕ пешки на d4/f4 против e3 и т.п.
            if ((e & 7) != 0 && (nxt.bb[them][PAWN] & one(Square(e + 7)))) can_ep = true;
            if ((e & 7) != 7 && (nxt.bb[them][PAWN] & one(Square(e + 9)))) can_ep = true;
        }
        else {                       // ходили черные: ищем БЕЛЫЕ пешки на d5/f5 против e6
            if ((e & 7) != 7 && (nxt.bb[them][PAWN] & one(Square(e - 7)))) can_ep = true;
            if ((e & 7) != 0 && (nxt.bb[them][PAWN] & one(Square(e - 9)))) can_ep = true;
        }

        if (can_ep) nxt.ep = e;
    }

    nxt.occ_all = nxt.occ[WHITE] | nxt.occ[BLACK];

    /* смена хода */
    nxt.stm = them;
}

/* ------------------------------------------------------------
 *  position_from_fen – парсер стандартного FEN
 *  Возвращает true, если парсинг успешен
 * ------------------------------------------------------------*/
bool position_from_fen(Position& p, const std::string& fen)
{
    Position tmp;  // локальный
    tmp = Position();  // обнуляем
    for (int c = 0; c < 2; ++c)
        for (int t = 0; t < 6; ++t)
            tmp.bb[c][t] = 0;

    std::istringstream ss(fen);
    std::string board, turn, castling, ep, halfmove, fullmove;
    if (!(ss >> board >> turn >> castling >> ep >> halfmove >> fullmove))
        return false;                       // не 6 полей

    /* ----------- поле 1: расположение фигур ----------- */
    int sq = 56;                            // a8
    for (char ch : board)
    {
        if (ch == '/') { sq -= 16; continue; }
        if (std::isdigit(ch)) { sq += ch - '0'; continue; }

        int color = std::isupper(ch) ? WHITE : BLACK;
        PieceType pt;
        switch (std::tolower(ch)) {
        case 'p': pt = PAWN;   break;
        case 'n': pt = KNIGHT; break;
        case 'b': pt = BISHOP; break;
        case 'r': pt = ROOK;   break;
        case 'q': pt = QUEEN;  break;
        case 'k': pt = KING;   break;
        default:  return false;
        }
        if (sq < 0 || sq >= 64) return false;
        tmp.bb[color][pt] |= one(Square(sq));
        sq++;
    }

    /* ----------- поле 2: side to move ----------- */
    if (turn == "w") tmp.stm = WHITE;
    else if (turn == "b") tmp.stm = BLACK;
    else return false;

    /* ----------- поле 3: castling ----------- */
    tmp.cr = 0;
    if (castling.find('K') != std::string::npos) tmp.cr |= WOO;
    if (castling.find('Q') != std::string::npos) tmp.cr |= WOOO;
    if (castling.find('k') != std::string::npos) tmp.cr |= BOO;
    if (castling.find('q') != std::string::npos) tmp.cr |= BOOO;

    /* ----------- поле 4: en-passant ----------- */
    if (ep == "-") tmp.ep = SQ_NONE;
    else if (ep.size() == 2 &&
        ep[0] >= 'a' && ep[0] <= 'h' &&
        ep[1] >= '1' && ep[1] <= '8')
    {
        int file = ep[0] - 'a';
        int rank = ep[1] - '1';
        tmp.ep = Square(file + 8 * rank);
    }
    else return false;

    /* ----------- пересчитываем occ / occ_all ----------- */
    tmp.occ[WHITE] = tmp.occ[BLACK] = 0;
    for (int t = 0; t < 6; ++t) {
        tmp.occ[WHITE] |= tmp.bb[WHITE][t];
        tmp.occ[BLACK] |= tmp.bb[BLACK][t];
    }
    tmp.occ_all = tmp.occ[WHITE] | tmp.occ[BLACK];

    /* --- все прошло копируем во внешнюю позицию --- */
    p = tmp;
    return true;
}


void Position::set_startpos()
{
    bb[WHITE][PAWN] = 0x000000000000FF00ULL;
    bb[WHITE][KNIGHT] = 0x0000000000000042ULL;
    bb[WHITE][BISHOP] = 0x0000000000000024ULL;
    bb[WHITE][ROOK] = 0x0000000000000081ULL;
    bb[WHITE][QUEEN] = 0x0000000000000008ULL;
    bb[WHITE][KING] = 0x0000000000000010ULL;

    bb[BLACK][PAWN] = 0x00FF000000000000ULL;
    bb[BLACK][KNIGHT] = 0x4200000000000000ULL;
    bb[BLACK][BISHOP] = 0x2400000000000000ULL;
    bb[BLACK][ROOK] = 0x8100000000000000ULL;
    bb[BLACK][QUEEN] = 0x0800000000000000ULL;
    bb[BLACK][KING] = 0x1000000000000000ULL;

    occ[WHITE] = bb[WHITE][PAWN] | bb[WHITE][KNIGHT] | bb[WHITE][BISHOP] |
        bb[WHITE][ROOK] | bb[WHITE][QUEEN] | bb[WHITE][KING];
    occ[BLACK] = bb[BLACK][PAWN] | bb[BLACK][KNIGHT] | bb[BLACK][BISHOP] |
        bb[BLACK][ROOK] | bb[BLACK][QUEEN] | bb[BLACK][KING];

    occ_all = occ[WHITE] | occ[BLACK];
    stm = WHITE;
}
