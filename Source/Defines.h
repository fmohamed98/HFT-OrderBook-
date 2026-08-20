#pragma once
#include <cstdint>

using u32 = uint32_t;
using u64 = uint64_t;
using Price = int32_t;

constexpr Price ToPrice(double price)
{
    return static_cast<Price>(price * 100);
}

enum class Side
{
    Buy,
    Sell
};