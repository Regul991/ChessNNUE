// engine/main.cpp
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>      // memset
#include "bitboard.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "zobrist.h"
#include "tt.h"



/* --------------------------------------------------------
 *  Преобразование строки «e2e4», «a7a8q» → Move
 *  (без проверки легальности: GUI гарантирует корректность)
 * --------------------------------------------------------*/
static Move uci_to_move(const std::string& s)
{
    if (s.length() < 4) return 0;
    int fromFile = s[0] - 'a';
    int fromRank = s[1] - '1';
    int toFile   = s[2] - 'a';
    int toRank   = s[3] - '1';
    Square from  = Square(fromFile + 8 * fromRank);
    Square to    = Square(toFile   + 8 * toRank);

    int promo = 0;
    if (s.length() == 5) {
        switch (std::tolower(s[4])) {
        case 'n': promo = KNIGHT; break;
        case 'b': promo = BISHOP; break;
        case 'r': promo = ROOK;   break;
        case 'q': promo = QUEEN;  break;
        default:  promo = 0;
        }
    }
    return make_move(from, to, promo);
}

/* --------------------------------------------------------
 *  Вспомогательный perft (для отладки)
 * --------------------------------------------------------*/
static uint64_t perft(Position& pos, int depth)
{
    if (depth == 0) return 1ULL;
    std::vector<Move> list;
    generate_moves(pos, list);

    uint64_t nodes = 0;
    Position nxt;
    for (Move m : list) {
        pos.make_move(m, nxt);
        nodes += perft(nxt, depth - 1);
    }
    return nodes;
}


/* --------------------------------------------------------
 *  Применяем список ходов в UCI-формате к позиции
 *  (предполагаем, что ходы легальны — GUI отвечает за это).
 * --------------------------------------------------------*/
static void apply_move_list(Position& pos, std::istream& in)
{
    std::string mvStr;
    while (in >> mvStr) {
        if (mvStr == "go" || mvStr == "d" || mvStr == "perft" ||
            mvStr == "stop" || mvStr == "quit" || mvStr == "uci" ||
            mvStr == "isready" || mvStr == "position")          // следующий токен
        {
            /* Вернули лишний токен обратно во входной поток */
            for (int i = int(mvStr.size()) - 1; i >= 0; --i)
                in.putback(mvStr[i]);
            break;
        }
        Move mv = uci_to_move(mvStr);
        if (mv == 0) {
            std::cerr << "info string bad move " << mvStr << '\n';
            break;
        }
        Position nxt;
        pos.make_move(mv, nxt);
        pos = nxt;
    }
}

void print_board(const Position& p) {
    static const char sym[6] = { 'p','n','b','r','q','k' };
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) {
            Square s = Square(f + 8 * r);
            char c = '.';
            for (int color = 0; color < 2; ++color) {
                for (int pt = 0; pt < 6; ++pt) {
                    if (p.bb[color][pt] & one(s)) {
                        c = sym[pt];
                        if (color == WHITE) c = char(::toupper(c));
                    }
                }
            }
            std::cout << c;
        }
        std::cout << '\n';
    }
    std::cout << std::flush;
}

/* --------------------------------------------------------
 *  Главный цикл UCI
 * --------------------------------------------------------*/
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    init_attack_tables();
    Zobrist::init();

    Position pos;
    pos.set_startpos();          // текущая позиция
    TT::table[0] = {};           // просто «будоражим» таблицу, она inline

    std::string token;
    while (std::cin >> token)
    {
        /* ---------- базовые UCI-команды ---------- */
        if (token == "uci") {
            std::cout << "id name MyNNUEEngine 0.2\n"
                         "id author Danil Skvortsov 83151\n"
                         "uciok\n";
            continue;
        }
        if (token == "isready") {
            std::cout << "readyok\n";
            continue;
        }
        if (token == "quit") break;
        if (token == "ucinewgame") {
            pos.set_startpos();
            std::memset(TT::table, 0, sizeof(TT::table));
            continue;
        }

        /* ---------- позиция ---------- */
        if (token == "position")
        {
            Position tmp;
            std::string sub;  std::cin >> sub;

            /* --- startpos или fen --- */
            if (sub == "startpos") {
                tmp.set_startpos();
            }
            else if (sub == "fen") {
                std::string fen, part;
                int fields = 0;
                while (fields < 6 && std::cin >> part) {
                    if (part == "moves") {                      // досрочно встретили "moves"
                        for (int i = int(part.size()) - 1; i >= 0; --i)
                            std::cin.putback(part[i]);          // вернули его назад
                        break;
                    }
                    fen += part + ' ';
                    ++fields;
                }
                if (!position_from_fen(tmp, fen))
                    std::cerr << "info string bad FEN\n";
            }
            else {
                std::cerr << "info string expected 'startpos' or 'fen'\n";
                continue;
            }

            /* --- возможный хвост moves --- */
            std::string word;
            if (std::cin >> word) {
                if (word == "moves")
                    apply_move_list(tmp, std::cin);             // читаем все ходы
                else                                            // это уже другой токен
                    for (int i = int(word.size()) - 1; i >= 0; --i)
                        std::cin.putback(word[i]);
            }

            pos = tmp;                                          // делаем новую позицию
            continue;
        }

        if (token == "d") {
            print_board(pos);
            continue;
        }

        /* ---------- поиск ---------- */
        if (token == "go")
        {
            int depth = 4;          // значение по умолчанию
            std::string sub;
            while (std::cin >> sub) {
                if (sub == "depth") {
                    std::cin >> depth;
                }
                else if (sub == "movetime" || sub == "wtime" || sub == "btime") {
                    // эти параметры пока игнорируем
                    int skip; std::cin >> skip;
                }
                else {
                    // дошли до конца строки
                    if (sub != "infinite") {
                        // sub уже относится к следующей команде → вернуть в поток
                        std::cin.putback('\n');
                        for (int i = sub.size() - 1; i >= 0; --i) std::cin.putback(sub[i]);
                    }
                    break;
                }
            }
            auto res = search(pos, depth);
            std::cout << "bestmove " << uci_move(res.best) << '\n';
            continue;
        }

        /* ---------- perft N ---------- (для тестов) */
        if (token == "perft") {
            int d; std::cin >> d;
            Position tmp = pos;
            uint64_t n = perft(tmp, d);
            std::cout << "info nodes " << n << std::endl;
            continue;
        }

        /* ---------- неизвестная команда ---------- */
        std::cerr << "info string unknown token '" << token << "'\n";
    }

    return 0;
}
