#include "internal/Orderbook.h"

#include <benchmark/benchmark.h>
#include <memory>
#include <random>

static void BM_SimpleAddOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        Orderbook orderbook;
        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);
        benchmark::DoNotOptimize(orderbook.AddOrder(order));
    }
}
BENCHMARK(BM_SimpleAddOrder);

static void BM_AddOrderNoMatch(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                static_cast<uint64_t>(i) + 1000000,
                i % 2 == 0 ? Side::Buy : Side::Sell,
                100 + i,
                10
            );
            benchmark::DoNotOptimize(orderbook.AddOrder(order));
        }
    }
}
BENCHMARK(BM_AddOrderNoMatch)->RangeMultiplier(2)->Range(10, 1000);

static void BM_AddOrderWithFullMatch(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                static_cast<uint64_t>(i),
                Side::Buy,
                100,
                10
            );
            orderbook.AddOrder(order);
        }
        state.ResumeTiming();
        
        auto sell = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            static_cast<uint64_t>(state.range(0)) + 1000000,
            Side::Sell,
            100,
            10 * state.range(0)
        );
        benchmark::DoNotOptimize(orderbook.AddOrder(sell));
    }
}
BENCHMARK(BM_AddOrderWithFullMatch)->RangeMultiplier(2)->Range(10, 100);

static void BM_AddOrderWithPartialMatch(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                static_cast<uint64_t>(i),
                Side::Buy,
                100,
                10
            );
            orderbook.AddOrder(order);
        }
        state.ResumeTiming();
        
        auto sell = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            static_cast<uint64_t>(state.range(0)) + 1000000,
            Side::Sell,
            100,
            5 * state.range(0)
        );
        benchmark::DoNotOptimize(orderbook.AddOrder(sell));
    }
}
BENCHMARK(BM_AddOrderWithPartialMatch)->RangeMultiplier(2)->Range(10, 100);

static void BM_CancelOrderEmpty(benchmark::State& state)
{
    Orderbook orderbook;
    for (auto _ : state)
    {
        orderbook.CancelOrder(999999);
    }
}
BENCHMARK(BM_CancelOrderEmpty);

static void BM_CancelOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel, i, Side::Buy, 100, 10);
            orderbook.AddOrder(order);
        }
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            orderbook.CancelOrder(i);
        }
    }
}
BENCHMARK(BM_CancelOrder)->RangeMultiplier(2)->Range(10, 1000);

static void BM_CancelOrderWorstCase(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;

        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel, i, Side::Buy, 100, 10);
            orderbook.AddOrder(order);
        }
        state.ResumeTiming();
        
        orderbook.CancelOrder(state.range(0) / 2);
    }
}
BENCHMARK(BM_CancelOrderWorstCase)->RangeMultiplier(2)->Range(10, 1000);

static void BM_MatchOrder(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel, 1, Side::Buy, 100, 10);
        orderbook.AddOrder(order);
        state.ResumeTiming();
        
        OrderModify modify(1, Side::Buy, 101, 20);
        benchmark::DoNotOptimize(orderbook.MatchOrder(modify));
    }
}
BENCHMARK(BM_MatchOrder);

static void BM_GetOrderInfos(benchmark::State& state)
{
    Orderbook orderbook;
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            i,
            i % 2 == 0 ? Side::Buy : Side::Sell,
            100 + (i / 2),
            10
        );
        orderbook.AddOrder(order);
    }
    
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(orderbook.GetOrderInfos());
    }
}
BENCHMARK(BM_GetOrderInfos)->RangeMultiplier(2)->Range(10, 1000);

static void BM_Size(benchmark::State& state)
{
    Orderbook orderbook;
    for (int i = 0; i < state.range(0); ++i)
    {
        auto order = std::make_shared<Order>(
            OrderType::GoodTillCancel, i, Side::Buy, 100, 10);
        orderbook.AddOrder(order);
    }
    
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(orderbook.Size());
    }
}
BENCHMARK(BM_Size)->RangeMultiplier(2)->Range(10, 1000);

static void BM_MixedWorkload(benchmark::State& state)
{
    // 60% add, 30% cancel, 10% query
    std::mt19937 rng(42);
    std::uniform_int_distribution<> op_dist(1, 10);
    std::uniform_int_distribution<> price_dist(95, 105);
    
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        uint64_t order_id = 0;
        std::vector<uint64_t> active_orders;
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            int op = op_dist(rng);
            
            if (op <= 6)
            {
                auto order = std::make_shared<Order>(
                    OrderType::GoodTillCancel,
                    order_id,
                    order_id % 2 == 0 ? Side::Buy : Side::Sell,
                    price_dist(rng),
                    10
                );
                orderbook.AddOrder(order);
                active_orders.push_back(order_id);
                order_id++;
            }
            else if (op <= 9 && !active_orders.empty())
            {
                size_t idx = rng() % active_orders.size();
                orderbook.CancelOrder(active_orders[idx]);
                active_orders.erase(active_orders.begin() + idx);
            }
            else
            {
                benchmark::DoNotOptimize(orderbook.GetOrderInfos());
            }
        }
    }
}
BENCHMARK(BM_MixedWorkload)->RangeMultiplier(2)->Range(100, 1000);

static void BM_HighFrequencyTrading(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        uint64_t order_id = 0;
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                order_id,
                i % 2 == 0 ? Side::Buy : Side::Sell,
                100 + (i % 2 == 0 ? -1 : 1),
                10
            );
            orderbook.AddOrder(order);
            
            // Immediately cancel 50% of orders
            if (i % 2 == 0 && i > 0)
            {
                orderbook.CancelOrder(order_id - 1);
            }
            order_id++;
        }
    }
}
BENCHMARK(BM_HighFrequencyTrading)->RangeMultiplier(2)->Range(100, 1000);

static void BM_FillAndKillMatch(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;

        auto resting = std::make_shared<Order>(
            OrderType::GoodTillCancel, 1, Side::Buy, 100, 100);
        orderbook.AddOrder(resting);
        state.ResumeTiming();
        
        auto fak = std::make_shared<Order>(
            OrderType::FillAndKill, 2, Side::Sell, 100, 50);
        benchmark::DoNotOptimize(orderbook.AddOrder(fak));
    }
}
BENCHMARK(BM_FillAndKillMatch);

static void BM_FillAndKillNoMatch(benchmark::State& state)
{
    for (auto _ : state)
    {
        Orderbook orderbook;

        auto fak = std::make_shared<Order>(
            OrderType::FillAndKill, 1, Side::Buy, 100, 10);
        benchmark::DoNotOptimize(orderbook.AddOrder(fak));
    }
}
BENCHMARK(BM_FillAndKillNoMatch);

static void BM_DeepOrderBook(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                i,
                i % 2 == 0 ? Side::Buy : Side::Sell,
                100 + i,
                10
            );
            benchmark::DoNotOptimize(orderbook.AddOrder(order));
        }
    }
}
BENCHMARK(BM_DeepOrderBook)->RangeMultiplier(2)->Range(100, 1000);

static void BM_WideOrderBook(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        Orderbook orderbook;
        state.ResumeTiming();
        
        for (int i = 0; i < state.range(0); ++i)
        {
            auto order = std::make_shared<Order>(
                OrderType::GoodTillCancel,
                i,
                Side::Buy,
                100,
                10
            );
            benchmark::DoNotOptimize(orderbook.AddOrder(order));
        }
    }
}
BENCHMARK(BM_WideOrderBook)->RangeMultiplier(2)->Range(100, 1000);

BENCHMARK_MAIN();