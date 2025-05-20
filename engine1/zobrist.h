#pragma once
#include "position.h"
#include <cstdint>

namespace Zobrist {

	inline uint64_t R[2][6][64];   // фигура-цвет-клетка
	inline uint64_t CASTLE[16];    // 4-битная маска прав
	inline uint64_t EP[8];         // файл en-passant (a-h)
	inline uint64_t SIDE;          // ход стороны

	void init();                         // вызвать 1 раз при старте
	uint64_t hash(const Position& pos);  // хеш всей позиции
} // namespace