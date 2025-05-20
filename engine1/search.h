#pragma once
#include "movegen.h"
#include "eval.h"
#include <cstdint>
#include <vector>

struct SearchResult {
    Move best;
    int  score;          // в сотых пешки
    uint64_t nodes;
};

SearchResult search(Position& root, int depth);