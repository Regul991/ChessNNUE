#pragma once
#include "position.h"
#include <cstdint>

namespace Zobrist {

	inline uint64_t R[2][6][64];   // ������-����-������
	inline uint64_t CASTLE[16];    // 4-������ ����� ����
	inline uint64_t EP[8];         // ���� en-passant (a-h)
	inline uint64_t SIDE;          // ��� �������

	void init();                         // ������� 1 ��� ��� ������
	uint64_t hash(const Position& pos);  // ��� ���� �������
} // namespace