#include "ConfigManager.h"
#include "FileUtils.h"
#include "ragger_plugin_api.h"
#include <benchmark/benchmark.h>
#include <string>

namespace Ragger {

// Simple benchmark for config operations
static void BM_ConfigStringOperations(benchmark::State& state) {
    ConfigManager config;
    
    for (auto _ : state) {
        config.setString("benchmark.key", "benchmark_value");
        const char* value = nullptr;
        int result = config.getString("benchmark.key", &value);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConfigStringOperations);

// Benchmark for file extension detection
static void BM_FileExtensionDetection(benchmark::State& state) {
    std::vector<std::string> testFiles = {
        "test.cpp", "test.c", "test.py", "test.js", "test.java",
        "test.go", "test.rs", "test.json", "test.yaml", "test.xml"
    };
    
    for (auto _ : state) {
        for (const auto& file : testFiles) {
            std::string extension = FileUtils::getFileExtension(file);
            benchmark::DoNotOptimize(extension);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * testFiles.size());
}
BENCHMARK(BM_FileExtensionDetection);

// Benchmark for language detection
static void BM_LanguageDetection(benchmark::State& state) {
    std::vector<std::string> testFiles = {
        "test.cpp", "test.c", "test.py", "test.js", "test.java",
        "test.go", "test.rs", "test.json", "test.yaml", "test.xml"
    };
    
    for (auto _ : state) {
        for (const auto& file : testFiles) {
            std::string language = FileUtils::detectLanguageFromPath(file);
            benchmark::DoNotOptimize(language);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * testFiles.size());
}
BENCHMARK(BM_LanguageDetection);

// Benchmark for config int operations
static void BM_ConfigIntOperations(benchmark::State& state) {
    ConfigManager config;
    
    for (auto _ : state) {
        config.setInt("benchmark.number", 42);
        int value = 0;
        int result = config.getInt("benchmark.number", &value);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConfigIntOperations);

} // namespace Ragger
