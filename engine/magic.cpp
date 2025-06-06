// engine/magic.cpp
#include "magic.h"
#include "types.h"     // Square, Side
#include "bitboard.h"  // Bitboard
#include <vector>
#include <array>
#include <random>
#include <cstring>     // memset
#include <intrin.h> // для _BitScanForward64
#include <iostream> 


/* --------------------------------------------------------
 *  Вспомогательные inline-утилиты
 * --------------------------------------------------------*/
static inline int popcount(uint64_t x) {
    // SWAR-popcount, population_count читает, сколько битов установлено в 1 в 64-битном числе
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return int((x * 0x0101010101010101ULL) >> 56);
}

static inline int lsb(uint64_t b) { 
    // Least Significant Bit - возвращает индекс младшего установленного бита (нужен для перебора маски)
    unsigned long idx;
    _BitScanForward64(&idx, b);   // гарантируем b != 0
    return int(idx);
}

static inline Bitboard one(int sq) { return 1ULL << sq; }


/* --------------------------------------------------------
 *  Маски лучей
 * --------------------------------------------------------*/
Bitboard RookMask[64]{}, BishopMask[64]{};

static Bitboard mask_rook(Square sq) {
    Bitboard m = 0;
    int f = sq & 7, r = sq >> 3;
    for (int ff = f + 1; ff <= 6; ++ff) m |= one(r * 8 + ff);
    for (int ff = f - 1; ff >= 1; --ff) m |= one(r * 8 + ff);
    for (int rr = r + 1; rr <= 6; ++rr) m |= one(rr * 8 + f);
    for (int rr = r - 1; rr >= 1; --rr) m |= one(rr * 8 + f);
    return m;
}
static Bitboard mask_bishop(Square sq) {
    Bitboard m = 0;
    int f = sq & 7, r = sq >> 3;
    for (int ff = f + 1, rr = r + 1; ff <= 6 && rr <= 6; ++ff, ++rr) m |= one(rr * 8 + ff);
    for (int ff = f - 1, rr = r + 1; ff >= 1 && rr <= 6; --ff, ++rr) m |= one(rr * 8 + ff);
    for (int ff = f + 1, rr = r - 1; ff <= 6 && rr >= 1; ++ff, --rr) m |= one(rr * 8 + ff);
    for (int ff = f - 1, rr = r - 1; ff >= 1 && rr >= 1; --ff, --rr) m |= one(rr * 8 + ff);
    return m;
}

/* --------------------------------------------------------
 * атаки для генерации таблиц
 * --------------------------------------------------------*/
static Bitboard rook_attack_on_the_fly(Square sq, Bitboard occ) {
    Bitboard a = 0;
    int f = sq & 7, r = sq >> 3;
    for (int ff = f + 1; ff <= 7; ++ff) { a |= one(r * 8 + ff); if (occ & one(r * 8 + ff)) break; }
    for (int ff = f - 1; ff >= 0; --ff) { a |= one(r * 8 + ff); if (occ & one(r * 8 + ff)) break; }
    for (int rr = r + 1; rr <= 7; ++rr) { a |= one(rr * 8 + f); if (occ & one(rr * 8 + f)) break; }
    for (int rr = r - 1; rr >= 0; --rr) { a |= one(rr * 8 + f); if (occ & one(rr * 8 + f)) break; }
    return a;
}
static Bitboard bishop_attack_on_the_fly(Square sq, Bitboard occ) {
    Bitboard a = 0;
    int f = sq & 7, r = sq >> 3;
    for (int ff = f + 1, rr = r + 1; ff <= 7 && rr <= 7; ++ff, ++rr) { a |= one(rr * 8 + ff); if (occ & one(rr * 8 + ff)) break; }
    for (int ff = f - 1, rr = r + 1; ff >= 0 && rr <= 7; --ff, ++rr) { a |= one(rr * 8 + ff); if (occ & one(rr * 8 + ff)) break; }
    for (int ff = f + 1, rr = r - 1; ff <= 7 && rr >= 0; ++ff, --rr) { a |= one(rr * 8 + ff); if (occ & one(rr * 8 + ff)) break; }
    for (int ff = f - 1, rr = r - 1; ff >= 0 && rr >= 0; --ff, --rr) { a |= one(rr * 8 + ff); if (occ & one(rr * 8 + ff)) break; }
    return a;
}

/* --------------------------------------------------------
 *  Таблицы и «магия)))
 * --------------------------------------------------------*/
 /* Предрасчитанные магические числа и сдвиги для ладей и слонов
  * Взяты из открытых источников (Pradyumna Kannan, используются в Crafty и др)
  * Гарантируют уникальные индексы для occupancy без коллизий. Короче до этого я их считал в брутфорсом каждый запуск, ниже функция find_magic.
  */
static const uint64_t RookMagic[64] = {
    0x0080001020400080ULL, 0x0040001000200040ULL, 0x0080081000200080ULL, 0x0080040800100080ULL,
    0x0080020400080080ULL, 0x0080010200040080ULL, 0x0080008001000200ULL, 0x0080002040800100ULL,
    0x0000800020400080ULL, 0x0000400020005000ULL, 0x0000801000200080ULL, 0x0000800800100080ULL,
    0x0000800400080080ULL, 0x0000800200040080ULL, 0x0000800100020080ULL, 0x0000800040800100ULL,
    0x0000208000400080ULL, 0x0000404000201000ULL, 0x0000808010002000ULL, 0x0000808008001000ULL,
    0x0000808004000800ULL, 0x0000808002000400ULL, 0x0000010100020004ULL, 0x0000020000408104ULL,
    0x0000208080004000ULL, 0x0000200040005000ULL, 0x0000100080200080ULL, 0x0000080080100080ULL,
    0x0000040080080080ULL, 0x0000020080040080ULL, 0x0000010080800200ULL, 0x0000800080004100ULL,
    0x0000204000800080ULL, 0x0000200040401000ULL, 0x0000100080802000ULL, 0x0000080080801000ULL,
    0x0000040080800800ULL, 0x0000020080800400ULL, 0x0000020001010004ULL, 0x0000800040800100ULL,
    0x0000204000808000ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000010002008080ULL, 0x0000004081020004ULL,
    0x0000204000800080ULL, 0x0000200040008080ULL, 0x0000100020008080ULL, 0x0000080010008080ULL,
    0x0000040008008080ULL, 0x0000020004008080ULL, 0x0000800100020080ULL, 0x0000800041000080ULL,
    0x00FFFCDDFCED714AULL, 0x007FFCDDFCED714AULL, 0x003FFFCDFFD88096ULL, 0x0000040810002101ULL,
    0x0001000204080011ULL, 0x0001000204000801ULL, 0x0001000082000401ULL, 0x0001FFFAABFAD1A2ULL
};
static const uint64_t BishopMagic[64] = {
    0x0002020202020200ULL, 0x0002020202020000ULL, 0x0004010202000000ULL, 0x0004040080000000ULL,
    0x0001104000000000ULL, 0x0000821040000000ULL, 0x0000410410400000ULL, 0x0000104104104000ULL,
    0x0000040404040400ULL, 0x0000020202020200ULL, 0x0000040102020000ULL, 0x0000040400800000ULL,
    0x0000011040000000ULL, 0x0000008210400000ULL, 0x0000004104104000ULL, 0x0000002082082000ULL,
    0x0004000808080800ULL, 0x0002000404040400ULL, 0x0001000202020200ULL, 0x0000800802004000ULL,
    0x0000800400A00000ULL, 0x0000200100884000ULL, 0x0000400082082000ULL, 0x0000200041041000ULL,
    0x0002080010101000ULL, 0x0001040008080800ULL, 0x0000208004010400ULL, 0x0000404004010200ULL,
    0x0000840000802000ULL, 0x0000404002011000ULL, 0x0000808001041000ULL, 0x0000404000820800ULL,
    0x0001041000202000ULL, 0x0000820800101000ULL, 0x0000104400080800ULL, 0x0000020080080080ULL,
    0x0000404040040100ULL, 0x0000808100020100ULL, 0x0001010100020800ULL, 0x0000808080010400ULL,
    0x0000820820004000ULL, 0x0000410410002000ULL, 0x0000082088001000ULL, 0x0000002011000800ULL,
    0x0000080100400400ULL, 0x0001010101000200ULL, 0x0002020202000400ULL, 0x0001010101000200ULL,
    0x0000410410400000ULL, 0x0000208208200000ULL, 0x0000002084100000ULL, 0x0000000020880000ULL,
    0x0000001002020000ULL, 0x0000040408020000ULL, 0x0004040404040000ULL, 0x0002020202020000ULL,
    0x0000104104104000ULL, 0x0000002082082000ULL, 0x0000000020841000ULL, 0x0000000000208800ULL,
    0x0000000010020200ULL, 0x0000000404080200ULL, 0x0000040404040400ULL, 0x0002020202020200ULL
};
static const uint8_t RookShift[64] = {
    52, 53, 53, 53, 53, 53, 53, 52,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 53, 53, 53, 53, 53
};
static const uint8_t BishopShift[64] = {
    58, 59, 59, 59, 59, 59, 59, 58,
    59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59,
    58, 59, 59, 59, 59, 59, 59, 58
};


Bitboard RookAttack[64][4096];
Bitboard BishopAttack[64][512];

/* --------------------------------------------------------
 *  вспомогательные функции
 * --------------------------------------------------------*/
static Bitboard index_to_occ(int index, Bitboard mask) {
    Bitboard occ = 0;
    while (mask) {
        int sq = lsb(mask);
        mask &= mask - 1;
        if (index & 1) occ |= one(sq);
        index >>= 1;
    }
    return occ;
}
/*----- Поиск чисел -----*/                                                   /*!!!какое то говно тут!!!*/  /*UPDATE исправил*/
//static uint64_t find_magic(Square sq, int bits, bool rook) {
//    const char* piece = rook ? "Rook" : "Bishop";
//    std::mt19937_64 gen(uint64_t(sq) * 918273645ULL + 1234567);
//    int size = 1 << bits;
//
//    // вредвычисляем эталонные атаки
//    std::vector<Bitboard> ref(size);
//    Bitboard mask = rook ? RookMask[sq] : BishopMask[sq];
//    for (int i = 0; i < size; ++i)
//        ref[i] = rook
//        ? rook_attack_on_the_fly(sq, index_to_occ(i, mask))
//        : bishop_attack_on_the_fly(sq, index_to_occ(i, mask));
//
//    std::cerr << piece << " find_magic: sq=" << int(sq)
//        << " bits=" << bits << " entries=" << size << "\n";
//
//    uint64_t tries = 0;
//    const uint64_t TRY_LIMIT = 5'000'000;   // допустимый максимум эээ
//
//    while (true) {
//        ++tries;
//
//        // каждый мильен
//        if ((tries & 0xFFFFF) == 0)
//            std::cerr << piece << " sq=" << int(sq)
//            << " tries=" << tries << "\n";
//
//        // ограничение по попыткам
//        if (tries > TRY_LIMIT) {
//            std::cerr << piece << " sq=" << int(sq)
//                << " reached TRY_LIMIT=" << TRY_LIMIT
//                << " Loosening the filter\n";
//            // больше не фильтруем по старшим битам
//            // просто возвращаем первое попавшееся число (по итогу не работает xd)         
//            return random_u64(gen);
//        }
//
//        uint64_t magic = random_u64(gen) & random_u64(gen) & random_u64(gen);
//
//        // !!!ОСЛАБЛЕННЫЙ!!! ФИЛЬТР требуем только минимум 4 старших единицы проверить на коллизии эти числа
//        if (popcount((magic * mask) & 0xFF00000000000000ULL) < 4)
//            continue;
//
//        // проверяем на коллизии
//        std::vector<Bitboard> used(size, 0);
//        bool good = true; //коллизии нет
//        for (int i = 0; i < size; ++i) {
//            Bitboard occ = index_to_occ(i, mask);
//            int idx = int((occ * magic) >> (64 - bits));
//            if (used[idx] == 0)
//                used[idx] = ref[i];
//            else if (used[idx] != ref[i]) {
//                good = false; //коллизия есть
//                break;
//            }
//        }
//
//        if (good) {
//            std::cerr << piece << " sq=" << int(sq)
//                << " magic found after " << tries << " tries\n";
//            return magic;
//        }
//    }
//}



/* --------------------------------------------------------
 *  Инициализация (вызывать один раз)
 * --------------------------------------------------------*/
void init_magic() {
    // 1) Сначала — маски лучей, как было
    for (int i = 0; i < 64; ++i) {
        Square sq = Square(i);
        RookMask[i] = mask_rook(sq);
        BishopMask[i] = mask_bishop(sq);
    }

    // 2) Заполняем таблицы атак, используя предрасчитанные магии и сдвиги
    for (int i = 0; i < 64; ++i) {
        Square sq = Square(i);

        // число релевантных бит в масках (для проверки размеров таблицы)
        int rBits = popcount(RookMask[i]);
        int bBits = popcount(BishopMask[i]);

        int rSize = 1 << rBits;
        for (int idx = 0; idx < rSize; ++idx) {
            Bitboard occ = index_to_occ(idx, RookMask[i]);
            // используем заранее заданное RookMagic[i] и RookShift[i]
            int key = int((occ * RookMagic[i]) >> RookShift[i]);
            RookAttack[i][key] = rook_attack_on_the_fly(sq, occ);
        }

        int bSize = 1 << bBits;
        for (int idx = 0; idx < bSize; ++idx) {
            Bitboard occ = index_to_occ(idx, BishopMask[i]);
            int key = int((occ * BishopMagic[i]) >> BishopShift[i]);
            BishopAttack[i][key] = bishop_attack_on_the_fly(sq, occ);
        }
    }
}


/* --------------------------------------------------------
 *  доступ к заранее сгенерированным атакам
 * --------------------------------------------------------*/
Bitboard rook_attacks(Square sq, Bitboard occ) {
    occ &= RookMask[sq];
    occ *= RookMagic[sq];
    occ >>= RookShift[sq];
    return RookAttack[sq][occ];
    
}
Bitboard bishop_attacks(Square sq, Bitboard occ) {
    occ &= BishopMask[sq];
    occ *= BishopMagic[sq];
    occ >>= BishopShift[sq];
    return BishopAttack[sq][occ];
    
}
/* -----------------------------------------------------------------
 *  Попросил нейронку убрать нерабочие MSVC-intrinsics для popcount,
 *  вместо них используется портируемый SWAR-алгоритм (не знаю что это пока что)
 * -------------------------------------------------------------------*/
