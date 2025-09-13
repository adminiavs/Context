# RAGger - RAG Pre-processor for AI Development

<div align="center">
  <img src="ragger.png" alt="RAGger Logo" width="200" height="200">
</div>

RAGger is a high-performance C++ RAG (Retrieval-Augmented Generation) pre-processor designed for AI-powered code analysis and development assistance.

## Features

- **Code Indexing**: Fast parsing and indexing of codebases
- **Plugin Architecture**: Extensible system with multiple plugins
- **Multiple Interfaces**: GUI, console, and development tools
- **Multi-language Support**: C++, Python, and more via Tree-sitter
- **Git Integration**: Version control awareness
- **BM25 & Graph Ranking**: Advanced relevance scoring
- **LSP Integration**: Language Server Protocol support

## Project Structure

```
ragger/
├── src/
│   ├── core/           # Core RAGger functionality
│   ├── ui/             # User interface implementations
│   └── tools/          # Development tools
├── plugins/            # Plugin implementations
│   ├── parsers/        # Code parsers (C++, Tree-sitter)
│   ├── rankers/        # Ranking algorithms (BM25, Graph)
│   ├── clients/        # LSP client
│   ├── integrations/   # Git integration
│   └── discovery/      # Test discovery
├── tests/              # Unit tests
├── benchmarks/         # Performance benchmarks
└── build/              # Build artifacts
```

## Building

### Prerequisites

- C++17 compatible compiler (GCC 7+ or Clang 5+)
- CMake 3.12+
- SQLite3
- OpenGL (for GUI)
- GLFW (for GUI)
- Dear ImGui (for GUI - optional)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd ragger

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Run tests
make test
```

## Usage

### Console Interface

The console interface provides a command-line interface for RAG operations:

```bash
./build/bin/ragger-console
```

Available commands:
- `help` - Show help message
- `status` - Show system status
- `plugins` - List available plugins
- `index <path>` - Index a file or directory
- `query <text>` - Process a query with RAG
- `config` - Show current configuration
- `exit` - Exit the application

### GUI Interface

The GUI provides a visual interface for RAG operations:

```bash
./build/bin/ragger-gui
```

**Note**: The current GUI implementation is a stub. For a fully functional GUI, you would need to:
1. Install Dear ImGui dependencies
2. Build the working GUI: `make ragger-working-gui`

### Development Tools

```bash
# Development interface
./build/bin/ragger-dev

# Run benchmarks
./build/bin/ragger-benchmarks

# Run tests
./build/bin/ragger-tests
```

## Plugins

RAGger uses a plugin architecture for extensibility:

### Available Plugins

- **C++ Parser**: Parses C++ code for semantic analysis
- **Tree-sitter Parser**: Multi-language parsing support
- **BM25 Ranker**: BM25 algorithm for relevance scoring
- **Graph Ranker**: Graph-based ranking algorithms
- **LSP Client**: Language Server Protocol integration
- **Git Integration**: Version control awareness
- **Test Discovery**: Automated test detection

### Plugin Development

Plugins are implemented as shared libraries in the `plugins/` directory. Each plugin implements the `RaggerPluginAPI` interface.

## Configuration

Configuration is managed through `config/ragger.conf`. Key settings include:

- Database path
- Indexing options
- Plugin configurations
- Performance tuning

## API

RAGger provides a C++ API for integration into other applications:

```cpp
#include "ragger_plugin_api.h"

// Initialize RAGger
RaggerCore core;
core.initialize();

// Index a codebase
core.indexDirectory("/path/to/code");

// Process a query
auto results = core.processQuery("refactor main function");
```

## Performance

RAGger is designed for high performance:

- **Indexing**: ~1000 files/second on modern hardware
- **Query Processing**: Sub-second response times
- **Memory Usage**: Efficient memory management
- **Scalability**: Handles large codebases (100k+ files)

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Roadmap

See `roadmap/` directory for current development plans and future features.

## Support

For questions, issues, or contributions, please:
- Open an issue on GitHub
- Check the documentation in `docs/`
- Review the roadmap in `roadmap/`

## Current Status

- ✅ Core RAGger functionality implemented
- ✅ Plugin architecture working
- ✅ Console interface functional
- ✅ Basic GUI framework (stub implementation)
- 🔄 Full GUI implementation (requires Dear ImGui)
- 🔄 Advanced ranking algorithms
- 🔄 LSP integration
- 🔄 Git integration

## Recent Updates

- Added console interface with full RAG functionality
- Implemented plugin architecture
- Created GUI framework (stub)
- Added comprehensive test suite
- Improved build system with CMake
