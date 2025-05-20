#pragma once
#include "movegen.h"
#include "eval.h"
#include <cstdint>
#include <vector>

struct SearchResult {
    Move best;
    int  score;          // � ����� �����
    uint64_t nodes;
};

SearchResult search(Position& root, int depth);