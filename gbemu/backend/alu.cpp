#include "gbemu/backend/alu.h"

#include "gbemu/backend/bitutils.h"

#include <cstdint>

namespace gbemu::backend::alu
{

auto adc(uint8_t a, uint8_t b, uint8_t carry) -> AluResult<uint8_t>
{
    const uint8_t result = a + b + carry;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = addHadHalfCarry(a, b, carry),
                .hadCarry = addHadCarry(a, b, carry),
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto sbc(uint8_t a, uint8_t b, uint8_t carry) -> AluResult<uint8_t>
{
    const uint8_t result = a - b - carry;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = true,
                .hadHalfCarry = subHadHalfCarry(a, b, carry),
                .hadCarry = subHadCarry(a, b, carry),
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto rlc(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a << 1U), 0, getBit(a, 7));
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto rrc(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, getBit(a, 0));
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto rl(uint8_t a, uint8_t newBit0) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a << 1U), 0, newBit0);
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto rr(uint8_t a, uint8_t newBit7) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, newBit7);
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto bit_and(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a & b;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_xor(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a ^ b;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_or(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a | b;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_cpl(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = ~a;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_swap(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = swapNibbles(a);
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_set(uint8_t a, uint8_t i, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(a, i, b);
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(result, 0) == 1,
                .bit7Set = getBit(result, 7) == 1,
            },
    };
}

auto bit_get(uint8_t a, uint8_t i) -> AluResult<uint8_t>
{
    const uint8_t result = getBit(a, i);
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit1Set = getBit(a, 1) == 1,
                .bit2Set = getBit(a, 2) == 1,
                .bit3Set = getBit(a, 3) == 1,
                .bit4Set = getBit(a, 4) == 1,
                .bit5Set = getBit(a, 5) == 1,
                .bit6Set = getBit(a, 6) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto bit_sla(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = a << 1U;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto bit_sra(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, getBit(a, 7));
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

auto bit_srl(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = a >> 1U;
    return AluResult{
        .result = result,
        .flags =
            {
                .isZero = result == 0,
                .operationIsSubtraction = false,
                .hadHalfCarry = false,
                .hadCarry = false,
                .bit0Set = getBit(a, 0) == 1,
                .bit7Set = getBit(a, 7) == 1,
            },
    };
}

} // namespace gbemu::backend::alu
