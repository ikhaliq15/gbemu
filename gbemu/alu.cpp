#include "alu.h"

namespace gbemu {
namespace alu {

    AluResult<uint8_t> adc(uint8_t a, uint8_t b, uint8_t carry)
    {
        AluResult<uint8_t> result;
        result.result = a + b + carry;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;

        // TODO: double check correctness of detecting carry's for 3 way addition
        result.flags.hadHalfCarry = addHadHalfCarry(a, b, carry);
        result.flags.hadCarry = addHadCarry(a, b, carry);

        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> sbc(uint8_t a, uint8_t b, uint8_t carry)
    {
        AluResult<uint8_t> result;
        result.result = a - b - carry;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = true;

        // TODO: double check correctness of detecting carry's for 3 way subtraction
        result.flags.hadHalfCarry = subHadHalfCarry(a, b, carry);
        result.flags.hadCarry = subHadCarry(a, b, carry);

        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> rlc(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = (a << 1) | getBit(a, 7);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> rrc(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = (a >> 1) | (getBit(a, 0) << 7);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> rl(uint8_t a, uint8_t newBit0)
    {
        AluResult<uint8_t> result;
        result.result = (a << 1) | newBit0;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> rr(uint8_t a, uint8_t newBit7)
    {
        AluResult<uint8_t> result;
        result.result = (a >> 1) | (newBit7 << 7);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_and(uint8_t a, uint8_t b)
    {
        AluResult<uint8_t> result;
        result.result = a & b;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_xor(uint8_t a, uint8_t b)
    {
        AluResult<uint8_t> result;
        result.result = a ^ b;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_or(uint8_t a, uint8_t b)
    {
        AluResult<uint8_t> result;
        result.result = a | b;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_cpl(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = ~a;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(result.result, 0) == 1;
        result.flags.bit7Set = getBit(result.result, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_swap(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = swapNibbles(a);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_set(uint8_t a, uint8_t i, uint8_t b)
    {
        AluResult<uint8_t> result;
        result.result = setBit(a, i, b);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_get(uint8_t a, uint8_t i)
    {
        AluResult<uint8_t> result;
        result.result = getBit(a, i);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit1Set = getBit(a, 1) == 1;
        result.flags.bit2Set = getBit(a, 2) == 1;
        result.flags.bit3Set = getBit(a, 3) == 1;
        result.flags.bit4Set = getBit(a, 4) == 1;
        result.flags.bit5Set = getBit(a, 5) == 1;
        result.flags.bit6Set = getBit(a, 6) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_sla(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = a << 1;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_sra(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = (a >> 1) | (a & 0x80);
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

    AluResult<uint8_t> bit_srl(uint8_t a)
    {
        AluResult<uint8_t> result;
        result.result = a >> 1;
        result.flags.isZero = result.result == 0;
        result.flags.operationIsSubtraction = false;
        result.flags.hadHalfCarry = false;
        result.flags.hadCarry = false;
        result.flags.bit0Set = getBit(a, 0) == 1;
        result.flags.bit7Set = getBit(a, 7) == 1;
        return result;
    }

} // alu
} // gbemu