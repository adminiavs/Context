#include "EventBus.h"
#include "ragger_plugin_api.h"
#include <benchmark/benchmark.h>
#include <vector>
#include <thread>
#include <chrono>

namespace Ragger {

// Benchmark for event emission
static void BM_EventEmission(benchmark::State& state) {
    EventBus eventBus;
    
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "benchmark_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto _ : state) {
        int result = eventBus.emitEvent(&event);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventEmission);

// Benchmark for event subscription
static void BM_EventSubscription(benchmark::State& state) {
    EventBus eventBus;
    
    auto callback = [](const EventData* event, void* userData) {
        // Empty callback for benchmarking
    };

    for (auto _ : state) {
        int result = eventBus.subscribe(EVENT_CODEBLOCK_INDEXED, callback, nullptr);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventSubscription);

// Benchmark for event emission with multiple subscribers
static void BM_EventEmissionWithSubscribers(benchmark::State& state) {
    EventBus eventBus;
    
    // Add multiple subscribers
    std::vector<EventCallback> callbacks;
    for (int i = 0; i < state.range(0); ++i) {
        auto callback = [](const EventData* event, void* userData) {
            // Empty callback for benchmarking
        };
        callbacks.push_back(callback);
        eventBus.subscribe(EVENT_CODEBLOCK_INDEXED, callback, nullptr);
    }
    
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "benchmark_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto _ : state) {
        int result = eventBus.emitEvent(&event);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_EventEmissionWithSubscribers)->Range(1, 1000)->Complexity(benchmark::oN);

// Benchmark for async event emission
static void BM_AsyncEventEmission(benchmark::State& state) {
    EventBus eventBus;
    
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "benchmark_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto _ : state) {
        int result = eventBus.emitEventAsync(&event);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AsyncEventEmission);

// Benchmark for event bus statistics
static void BM_EventBusStats(benchmark::State& state) {
    EventBus eventBus;
    
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "benchmark_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Emit some events first
    for (int i = 0; i < 1000; ++i) {
        eventBus.emitEvent(&event);
    }

    for (auto _ : state) {
        const auto& stats = eventBus.getStats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventBusStats);

// Benchmark for concurrent event emission
static void BM_ConcurrentEventEmission(benchmark::State& state) {
    EventBus eventBus;
    
    EventData event;
    event.type = EVENT_CODEBLOCK_INDEXED;
    event.sourcePlugin = "benchmark_plugin";
    event.data = nullptr;
    event.dataSize = 0;
    event.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (auto _ : state) {
        std::vector<std::thread> threads;
        
        for (int i = 0; i < state.range(0); ++i) {
            threads.emplace_back([&eventBus, &event]() {
                eventBus.emitEvent(&event);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ConcurrentEventEmission)->Range(1, 16)->Complexity(benchmark::oN);

} // namespace Ragger
