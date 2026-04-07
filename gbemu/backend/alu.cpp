#include "gbemu/backend/alu.h"

#include "gbemu/backend/bitutils.h"

#include <cstdint>

namespace gbemu::backend::alu
{

auto adc(uint8_t a, uint8_t b, uint8_t carry) -> AluResult<uint8_t>
{
    const uint8_t result = a + b + carry;
    return {result, {result == 0, false, addHadHalfCarry(a, b, carry), addHadCarry(a, b, carry), result}};
}

auto sbc(uint8_t a, uint8_t b, uint8_t carry) -> AluResult<uint8_t>
{
    const uint8_t result = a - b - carry;
    return {result, {result == 0, true, subHadHalfCarry(a, b, carry), subHadCarry(a, b, carry), result}};
}

auto rlc(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a << 1U), 0, getBit(a, 7));
    return {result, {result == 0, false, false, false, a}};
}

auto rrc(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, getBit(a, 0));
    return {result, {result == 0, false, false, false, a}};
}

auto rl(uint8_t a, uint8_t newBit0) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a << 1U), 0, newBit0);
    return {result, {result == 0, false, false, false, a}};
}

auto rr(uint8_t a, uint8_t newBit7) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, newBit7);
    return {result, {result == 0, false, false, false, a}};
}

auto bit_and(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a & b;
    return {result, {result == 0, false, false, false, result}};
}

auto bit_xor(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a ^ b;
    return {result, {result == 0, false, false, false, result}};
}

auto bit_or(uint8_t a, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = a | b;
    return {result, {result == 0, false, false, false, result}};
}

auto bit_cpl(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = ~a;
    return {result, {result == 0, false, false, false, result}};
}

auto bit_swap(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = swapNibbles(a);
    return {result, {result == 0, false, false, false, result}};
}

auto bit_set(uint8_t a, uint8_t i, uint8_t b) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(a, i, b);
    return {result, {result == 0, false, false, false, result}};
}

auto bit_get(uint8_t a, uint8_t i) -> AluResult<uint8_t>
{
    const uint8_t result = getBit(a, i);
    return {result, {result == 0, false, false, false, a}};
}

auto bit_sla(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = a << 1U;
    return {result, {result == 0, false, false, false, a}};
}

auto bit_sra(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = setBit(static_cast<uint8_t>(a >> 1U), 7, getBit(a, 7));
    return {result, {result == 0, false, false, false, a}};
}

auto bit_srl(uint8_t a) -> AluResult<uint8_t>
{
    const uint8_t result = a >> 1U;
    return {result, {result == 0, false, false, false, a}};
}

} // namespace gbemu::backend::alu
