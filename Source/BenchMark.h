#pragma once
#include "Defines.h"

class Benchmark
{
public:
    static void Run();

    static constexpr u64 NUM_OPERATIONS = 50'000;
    static constexpr u32 NUM_WARMUP_RUNS = 3;
    static constexpr u32 NUM_BENCHMARK_RUNS = 10;

    static constexpr u32 NUM_PRICE_LEVELS_TO_TEST = 50;
    static constexpr u32 ORDERS_PER_PRICE_LEVEL = 1'000;
private:
    static void BenchmarkAdd();
    static void BenchmarkCancel();
    static void BenchmarkMatch();

    static void BenchmarkDenseBitmapSearch();
    static void BenchmarkSparseBitmapSearch();
};