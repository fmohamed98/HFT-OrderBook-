#include "Benchmark.h"
#include "OrderBook.h"

#include <algorithm>
#include <chrono>
#include <print>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;

    struct BenchmarkResult
    {
        u64 m_Minimum = 0;
        u64 m_Maximum = 0;
        double m_Average = 0.0;
        double m_Median = 0.0;
    };

    template<typename SetupFunction, typename BenchmarkFunction>
    BenchmarkResult RunBenchmark(
        SetupFunction&& setup,
        BenchmarkFunction&& benchmark)
    {
        std::vector<u64> results;
        results.reserve(Benchmark::NUM_BENCHMARK_RUNS);

        // Warm-up
        for (u32 i = 0; i < Benchmark::NUM_WARMUP_RUNS; ++i)
        {
            auto orderBook = setup();
            benchmark(orderBook);
        }

        // Measured runs
        for (u32 i = 0; i < Benchmark::NUM_BENCHMARK_RUNS; ++i)
        {
            auto orderBook = setup();

            const auto start = Clock::now();

            benchmark(orderBook);

            const auto end = Clock::now();

            const u64 elapsed =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    end - start).count();

            results.push_back(elapsed);
        }

        std::sort(results.begin(), results.end());

        BenchmarkResult result;

        result.m_Minimum = results.front();
        result.m_Maximum = results.back();

        u64 total = 0;

        for (const u64 value : results)
        {
            total += value;
        }

        result.m_Average = static_cast<double>(total) / results.size();

        const std::size_t middle = results.size() / 2;

        if (results.size() % 2 == 0)
        {
            result.m_Median = (static_cast<double>(results[middle - 1]) + static_cast<double>(results[middle])) / 2.0;
        }
        else
        {
            result.m_Median = static_cast<double>(results[middle]);
        }

        return result;
    }

    void PrintResult(
        const BenchmarkResult& result,
        const char* operation)
    {
        std::println("{}", operation);
        std::println(
            "  Min:         {} ns",
            result.m_Minimum);
        std::println(
            "  Median:      {:.2f} ns",
            result.m_Median);
        std::println(
            "  Average:     {:.2f} ns",
            result.m_Average);
        std::println(
            "  Max:         {} ns",
            result.m_Maximum);
        std::println();
    }
}

void Benchmark::Run()
{
    std::println("========================================");
    std::println("         ORDER BOOK BENCHMARK");
    std::println("========================================");
    std::println("Operations: {}", NUM_OPERATIONS);
    std::println("Warm-up runs: {}", NUM_WARMUP_RUNS);
    std::println("Benchmark runs: {}", NUM_BENCHMARK_RUNS);
    std::println();

    BenchmarkAdd();
    BenchmarkCancel();
    BenchmarkMatch();

    BenchmarkDenseBitmapSearch();
    BenchmarkSparseBitmapSearch();
}

void Benchmark::BenchmarkAdd()
{
    const BenchmarkResult result = RunBenchmark(
        []()
        {
            return OrderBook{};
        },
        [](OrderBook& orderBook)
        {
            for (u64 i = 0; i < NUM_OPERATIONS; ++i)
            {
                orderBook.AddOrder(
                    i,
                    Side::Buy,
                    ToPrice(100.00),
                    100);
            }
        });

    const double nsPerOperation =
        result.m_Median / NUM_OPERATIONS;

    const double operationsPerSecond =
        1'000'000'000.0 / nsPerOperation;

    PrintResult(result, "AddOrder");

    std::println(
        "  Median ns/op: {:.2f}",
        nsPerOperation);

    std::println(
        "  Operations/sec: {:.2f}",
        operationsPerSecond);

    std::println();
}

void Benchmark::BenchmarkCancel()
{
    const BenchmarkResult result = RunBenchmark(
        []()
        {
            OrderBook orderBook;

            for (u64 i = 0; i < NUM_OPERATIONS; ++i)
            {
                orderBook.AddOrder(
                    i,
                    Side::Buy,
                    ToPrice(100.00),
                    100);
            }

            return orderBook;
        },
        [](OrderBook& orderBook)
        {
            for (u64 i = 0; i < NUM_OPERATIONS; ++i)
            {
                orderBook.CancelOrder(i);
            }
        });

    const double nsPerOperation =
        result.m_Median / NUM_OPERATIONS;

    const double operationsPerSecond =
        1'000'000'000.0 / nsPerOperation;

    PrintResult(result, "CancelOrder");

    std::println(
        "  Median ns/op: {:.2f}",
        nsPerOperation);

    std::println(
        "  Operations/sec: {:.2f}",
        operationsPerSecond);

    std::println();
}

void Benchmark::BenchmarkMatch()
{
    const BenchmarkResult result = RunBenchmark(
        []()
        {
            OrderBook orderBook;

            for (u64 i = 0; i < NUM_OPERATIONS; ++i)
            {
                orderBook.AddOrder(
                    i,
                    Side::Sell,
                    ToPrice(101.00),
                    1);
            }

            return orderBook;
        },
        [](OrderBook& orderBook)
        {
            orderBook.AddOrder(
                NUM_OPERATIONS,
                Side::Buy,
                ToPrice(101.01),
                NUM_OPERATIONS);
        });

    const double nsPerMatch =
        result.m_Median / NUM_OPERATIONS;

    const double matchesPerSecond =
        1'000'000'000.0 / nsPerMatch;

    PrintResult(result, "MatchOrder");

    std::println(
        "  Median ns/match: {:.2f}",
        nsPerMatch);

    std::println(
        "  Matches/sec: {:.2f}",
        matchesPerSecond);

    std::println();
}

void Benchmark::BenchmarkDenseBitmapSearch()
{
    const BenchmarkResult result = RunBenchmark(
        []()
        {
            OrderBook orderBook;

            // Populate 50 closely packed price levels.
            for (u32 level = 0; level < NUM_PRICE_LEVELS_TO_TEST; ++level)
            {
                const Price price = ToPrice(
                    100.00 + static_cast<double>(level) * 0.01);

                for (u32 order = 0; order < ORDERS_PER_PRICE_LEVEL; ++order)
                {
                    const u64 orderId =
                        static_cast<u64>(level) * ORDERS_PER_PRICE_LEVEL + order;

                    orderBook.AddOrder(
                        orderId,
                        Side::Sell,
                        price,
                        1);
                }
            }

            return orderBook;
        },
        [](OrderBook& orderBook)
        {
            // Cross the best ask.
            //
            // Quantity = 1 means exactly ONE resting order is matched.
            orderBook.AddOrder(
                50'000,
                Side::Buy,
                ToPrice(100.01),
                1);
        });

    const double nsPerOperation = result.m_Median;

    std::println("Dense Bitmap Search");
    std::println("  Min:         {} ns", result.m_Minimum);
    std::println("  Median:      {:.2f} ns", result.m_Median);
    std::println("  Average:     {:.2f} ns", result.m_Average);
    std::println("  Max:         {} ns", result.m_Maximum);
    std::println("  Median ns/op: {:.2f}", nsPerOperation);
    std::println();
}

void Benchmark::BenchmarkSparseBitmapSearch()
{
    const BenchmarkResult result = RunBenchmark(
        []()
        {
            OrderBook orderBook;

            // Populate 50 widely separated price levels.
            //
            // The first occupied level is deliberately far from
            // MIN_PRICE so FindNextOccupied() has to search
            // through empty bitmap words.
            constexpr Price PRICE_STEP = 40;

            for (u32 level = 0; level < NUM_PRICE_LEVELS_TO_TEST; ++level)
            {
                const Price price =
                    static_cast<Price>(10000 + level * PRICE_STEP);

                for (u32 order = 0; order < ORDERS_PER_PRICE_LEVEL; ++order)
                {
                    const u64 orderId =
                        static_cast<u64>(level) * ORDERS_PER_PRICE_LEVEL + order;

                    orderBook.AddOrder(
                        orderId,
                        Side::Sell,
                        price,
                        1);
                }
            }

            return orderBook;
        },
        [](OrderBook& orderBook)
        {
            // First occupied price is 10000 + 40 = 10040.
            //
            // Only one order is matched.
            orderBook.AddOrder(
                50'000,
                Side::Buy,
                10040,
                1);
        });

    const double nsPerOperation = result.m_Median;

    std::println("Sparse Bitmap Search");
    std::println("  Min:         {} ns", result.m_Minimum);
    std::println("  Median:      {:.2f} ns", result.m_Median);
    std::println("  Average:     {:.2f} ns", result.m_Average);
    std::println("  Max:         {} ns", result.m_Maximum);
    std::println("  Median ns/op: {:.2f}", nsPerOperation);
    std::println();
}
