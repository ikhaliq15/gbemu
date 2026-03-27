#ifndef GBEMU_ALU
#define GBEMU_ALU

#include "gbemu/bitutils.h"

#include <cstdint>

namespace gbemu::alu
{

struct AluFlagResult final
{
    const bool isZero;
    const bool operationIsSubtraction;
    const bool hadHalfCarry;
    const bool hadCarry;
    const bool bit0Set;
    const bool bit1Set;
    const bool bit2Set;
    const bool bit3Set;
    const bool bit4Set;
    const bool bit5Set;
    const bool bit6Set;
    const bool bit7Set;
};

template <typename T> struct AluResult final
{
    const T result;
    const AluFlagResult flags;
};

template <typename T, typename V> AluResult<T> add(T a, V b)
{
    const T result = a + b;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = addHadHalfCarry(a, b),
                .hadCarry = addHadCarry(a, b),
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

template <typename T, typename V> AluResult<T> sub(T a, V b)
{
    const T result = a - b;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = true,
                .hadHalfCarry = subHadHalfCarry(a, b),
                .hadCarry = subHadCarry(a, b),
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
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

} // namespace gbemu::alu

#endif // GBEMU_ALU
