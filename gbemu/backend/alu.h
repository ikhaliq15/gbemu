#ifndef GBEMU_BACKEND_ALU_H
#define GBEMU_BACKEND_ALU_H

#include "gbemu/backend/bitutils.h"

#include <cstdint>

namespace gbemu::backend::alu
{

struct AluFlagResult final
{
    bool isZero;
    bool operationIsSubtraction;
    bool hadHalfCarry;
    bool hadCarry;
    uint8_t flagBits; // individual bits referenced by Flag::A0..A7
};

template <typename T> struct AluResult final
{
    T result;
    AluFlagResult flags;
};

template <typename T, typename V> AluResult<T> add(T a, V b)
{
    const T result = a + b;
    return {result, {result == 0, false, addHadHalfCarry(a, b), addHadCarry(a, b), static_cast<uint8_t>(result)}};
}

template <typename T, typename V> AluResult<T> sub(T a, V b)
{
    const T result = a - b;
    return {result, {result == 0, true, subHadHalfCarry(a, b), subHadCarry(a, b), static_cast<uint8_t>(result)}};
}

AluResult<uint8_t> adc(uint8_t a, uint8_t b, uint8_t carry);
AluResult<uint8_t> sbc(uint8_t a, uint8_t b, uint8_t carry);

AluResult<uint8_t> rlc(uint8_t a);
AluResult<uint8_t> rrc(uint8_t a);
AluResult<uint8_t> rl(uint8_t a, uint8_t newBit0);
AluResult<uint8_t> rr(uint8_t a, uint8_t newBit7);

AluResult<uint8_t> bit_and(uint8_t a, uint8_t b);
AluResult<uint8_t> bit_xor(uint8_t a, uint8_t b);
AluResult<uint8_t> bit_or(uint8_t a, uint8_t b);
AluResult<uint8_t> bit_cpl(uint8_t a);
AluResult<uint8_t> bit_swap(uint8_t a);
AluResult<uint8_t> bit_set(uint8_t a, uint8_t i, uint8_t b);
AluResult<uint8_t> bit_get(uint8_t a, uint8_t i);
AluResult<uint8_t> bit_sla(uint8_t a);
AluResult<uint8_t> bit_sra(uint8_t a);
AluResult<uint8_t> bit_srl(uint8_t a);

} // namespace gbemu::backend::alu

#endif // GBEMU_BACKEND_ALU_H
