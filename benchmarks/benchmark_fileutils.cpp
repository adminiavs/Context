#include "FileUtils.h"
#include "ragger_plugin_api.h"
#include <benchmark/benchmark.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace Ragger {

// Benchmark for file reading
static void BM_FileRead(benchmark::State& state) {
    // Create test file
    std::string testFile = "benchmark_test.txt";
    std::string content(state.range(0), 'A');
    
    std::ofstream file(testFile);
    file << content;
    file.close();
    
    for (auto _ : state) {
        char* data = nullptr;
        size_t size = 0;
        int result = FileUtils::readFile(testFile, &data, &size);
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(size);
        delete[] data;
    }
    
    // Clean up
    std::filesystem::remove(testFile);
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_FileRead)->Range(1024, 1024 * 1024); // 1KB to 1MB

// Benchmark for file writing
static void BM_FileWrite(benchmark::State& state) {
    std::string testFile = "benchmark_write_test.txt";
    std::string content(state.range(0), 'B');
    
    for (auto _ : state) {
        int result = FileUtils::writeFile(testFile, content.c_str(), content.length());
        benchmark::DoNotOptimize(result);
    }
    
    // Clean up
    std::filesystem::remove(testFile);
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_FileWrite)->Range(1024, 1024 * 1024); // 1KB to 1MB

// Benchmark for file hashing
static void BM_FileHash(benchmark::State& state) {
    // Create test file
    std::string testFile = "benchmark_hash_test.txt";
    std::string content(state.range(0), 'C');
    
    std::ofstream file(testFile);
    file << content;
    file.close();
    
    for (auto _ : state) {
        char hash[65];
        int result = FileUtils::getFileHash(testFile, hash, sizeof(hash));
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(hash);
    }
    
    // Clean up
    std::filesystem::remove(testFile);
    
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_FileHash)->Range(1024, 1024 * 1024); // 1KB to 1MB

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

// Benchmark for file extension extraction
static void BM_FileExtension(benchmark::State& state) {
    std::vector<std::string> testFiles = {
        "test.cpp", "test.c", "test.py", "test.js", "test.java",
        "test.go", "test.rs", "test.json", "test.yaml", "test.xml",
        "test.tar.gz", "test.backup", "test", "test."
    };
    
    for (auto _ : state) {
        for (const auto& file : testFiles) {
            std::string extension = FileUtils::getFileExtension(file);
            benchmark::DoNotOptimize(extension);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * testFiles.size());
}
BENCHMARK(BM_FileExtension);

// Benchmark for directory operations
static void BM_DirectoryCreation(benchmark::State& state) {
    std::string baseDir = "benchmark_dir_test";
    
    for (auto _ : state) {
        std::string testDir = baseDir + "/" + std::to_string(state.iterations());
        bool result = FileUtils::ensureDirectoryExists(testDir);
        benchmark::DoNotOptimize(result);
    }
    
    // Clean up
    std::filesystem::remove_all(baseDir);
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_DirectoryCreation);

// Benchmark for path expansion
static void BM_PathExpansion(benchmark::State& state) {
    std::vector<std::string> testPaths = {
        "~/test", "./test", "/absolute/path", "relative/path",
        "~/Documents/test", "~/.config/test", "./subdir/test"
    };
    
    for (auto _ : state) {
        for (const auto& path : testPaths) {
            std::string expanded = FileUtils::expandUserPath(path);
            benchmark::DoNotOptimize(expanded);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * testPaths.size());
}
BENCHMARK(BM_PathExpansion);

// Benchmark for concurrent file operations
static void BM_ConcurrentFileRead(benchmark::State& state) {
    // Create test files
    std::vector<std::string> testFiles;
    for (int i = 0; i < state.range(0); ++i) {
        std::string filename = "benchmark_concurrent_" + std::to_string(i) + ".txt";
        testFiles.push_back(filename);
        
        std::ofstream file(filename);
        file << "Test content for file " << i;
        file.close();
    }
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        
        for (const auto& filename : testFiles) {
            threads.emplace_back([filename]() {
                char* data = nullptr;
                size_t size = 0;
                FileUtils::readFile(filename, &data, &size);
                delete[] data;
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Clean up
    for (const auto& filename : testFiles) {
        std::filesystem::remove(filename);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ConcurrentFileRead)->Range(1, 16)->Complexity(benchmark::oN);

} // namespace Ragger
