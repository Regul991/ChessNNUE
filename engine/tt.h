#pragma once
#include "move.h"
#include <cstdint>

namespace TT {

    enum Flag : uint8_t { EXACT, LOWER, UPPER };

    struct Entry {
        uint64_t key = 0;
        int8_t   depth = 0;
        int16_t  score = 0;
        uint8_t  flag = EXACT;
        Move     best = 0;
    };

    constexpr size_t SIZE = 1 << 20;              // 1 М слотов ≈ 8 МБ
    inline Entry table[SIZE];

    inline Entry& probe(uint64_t key) { return table[key & (SIZE - 1)]; }

} // namespace