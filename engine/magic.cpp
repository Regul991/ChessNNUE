// engine/magic.cpp
#include "magic.h"
#include "types.h"     // Square, Side
#include "bitboard.h"  // Bitboard
#include <vector>
#include <array>
#include <random>
#include <cstring>     // memset
#include <intrin.h>// для _BitScanForward64
#include <iostream> 


/* --------------------------------------------------------
 *  Вспомогательные inline-утилиты
 * --------------------------------------------------------*/
static inline int popcount(uint64_t x) {
    // SWAR-popcount
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return int((x * 0x0101010101010101ULL) >> 56);
}

static inline int lsb(uint64_t b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);   // гарантируем b ≠ 0
    return int(idx);
}

static inline Bitboard one(int sq) { return 1ULL << sq; }

static Bitboard random_u64(std::mt19937_64& gen) {
    return gen() & gen() & gen();
}

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
 *  «На-лету» атаки для генерации таблиц
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
 *  Таблицы и «магии»
 * --------------------------------------------------------*/
static std::array<uint64_t, 64> RookMagic{}, BishopMagic{};
static uint8_t RookShift[64]{}, BishopShift[64]{};

Bitboard RookAttack[64][4096];
Bitboard BishopAttack[64][512];

/* --------------------------------------------------------
 *  Вспомогательные функции
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
/*----- Поиск чисел -----*/                                                   /*!!!какое то говно тут!!!*/
static uint64_t find_magic(Square sq, int bits, bool rook) {
    const char* piece = rook ? "Rook" : "Bishop";
    std::mt19937_64 gen(uint64_t(sq) * 918273645ULL + 1234567);
    int size = 1 << bits;

    // Предвычисляем эталонные атаки
    std::vector<Bitboard> ref(size);
    Bitboard mask = rook ? RookMask[sq] : BishopMask[sq];
    for (int i = 0; i < size; ++i)
        ref[i] = rook
        ? rook_attack_on_the_fly(sq, index_to_occ(i, mask))
        : bishop_attack_on_the_fly(sq, index_to_occ(i, mask));

    std::cerr << piece << " find_magic: sq=" << int(sq)
        << " bits=" << bits << " entries=" << size << "\n";

    uint64_t tries = 0;
    const uint64_t TRY_LIMIT = 5'000'000;   // допустимый максимум

    while (true) {
        ++tries;

        // отчёт каждые миллион попыток
        if ((tries & 0xFFFFF) == 0)
            std::cerr << piece << " sq=" << int(sq)
            << " tries=" << tries << "\n";

        // ограничение по попыткам
        if (tries > TRY_LIMIT) {
            std::cerr << piece << " sq=" << int(sq)
                << " reached TRY_LIMIT=" << TRY_LIMIT
                << " Loosening the filter\n";
            // больше не фильтруем по старшим битам
            // просто возвращаем первое попавшееся magic
            // чтобы двигаться дальше и завершить init_magic()
            return random_u64(gen);
        }

        uint64_t magic = random_u64(gen) & random_u64(gen) & random_u64(gen);

        // ОСЛАБЛЁННЫЙ ФИЛЬТР: требуем только минимум 4 старших единицы
        if (popcount((magic * mask) & 0xFF00000000000000ULL) < 4)
            continue;

        // проверяем на коллизии
        std::vector<Bitboard> used(size, 0);
        bool good = true;
        for (int i = 0; i < size; ++i) {
            Bitboard occ = index_to_occ(i, mask);
            int idx = int((occ * magic) >> (64 - bits));
            if (used[idx] == 0)
                used[idx] = ref[i];
            else if (used[idx] != ref[i]) {
                good = false;
                break;
            }
        }

        if (good) {
            std::cerr << piece << " sq=" << int(sq)
                << " magic found after " << tries << " tries\n";
            return magic;
        }
    }
}



/* --------------------------------------------------------
 *  Инициализация (вызывать один раз)
 * --------------------------------------------------------*/
void init_magic() {
    /* 1) Маски */
    for (int i = 0; i < 64; ++i) {
        Square sq = Square(i);
        RookMask[i] = mask_rook(sq);
        BishopMask[i] = mask_bishop(sq);
    }

    /* 2) Магии и таблицы */
    for (int i = 0; i < 64; ++i) {
        Square sq = Square(i);
        int rb = popcount(RookMask[i]);
        int bb = popcount(BishopMask[i]);

        RookShift[i] = 64 - rb;
        BishopShift[i] = 64 - bb;

        RookMagic[i] = find_magic(sq, rb, true);
        BishopMagic[i] = find_magic(sq, bb, false);

        int rsize = 1 << rb;
        for (int idx = 0; idx < rsize; ++idx) {
            Bitboard occ = index_to_occ(idx, RookMask[i]);
            int key = int((occ * RookMagic[i]) >> RookShift[i]);
            RookAttack[i][key] = rook_attack_on_the_fly(sq, occ);
        }

        int bsize = 1 << bb;
        for (int idx = 0; idx < bsize; ++idx) {
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