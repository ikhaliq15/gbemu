#ifndef GBEMU_ALU
#define GBEMU_ALU

#include "bitutils.h"

#include <stdint.h>

namespace gbemu {
namespace alu {

    struct AluFlagResult
    {
        bool isZero;
        bool operationIsSubtraction;
        bool hadHalfCarry;
        bool hadCarry;
        bool bit0Set;
        bool bit1Set;
        bool bit2Set;
        bool bit3Set;
        bool bit4Set;
        bool bit5Set;
        bool bit6Set;
        bool bit7Set;
    };

    template <typename T> 
    struct AluResult
    {
        T result;
        AluFlagResult flags;
    };

    template <typename T, typename V> 
    AluResult<T> add(T a, V b)
    {
        AluResult<T> result;
        result.result = a + b;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = addHadHalfCarry(a, b);
        result.flags.hadCarry = addHadCarry(a, b);
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    template <typename T, typename V> 
    AluResult<T> sub(T a, V b)
    {
        AluResult<T> result;
        result.result = a - b;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = true;
        result.flags.hadHalfCarry = subHadHalfCarry(a, b);
        result.flags.hadCarry = subHadCarry(a, b);
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
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

} // alu
} // gbemu

#endif // GBEMU_ALU