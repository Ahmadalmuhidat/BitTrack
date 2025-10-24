A comprehensive, lightweight version control system built from scratch in C++—offering full offline control without cloud dependency. BitTrack provides Git-like functionality with a focus on simplicity, performance, and educational value.

## Features

### Core Functionality
- **Repository Management**: Initialize, clone, and manage BitTrack repositories
- **Branching**: Create, switch, merge, and delete branches
- **Staging**: Add, remove, and manage staged files
- **Committing**: Create commits with messages and metadata
- **Diffing**: View changes between commits, branches, and working directory
- **Stashing**: Temporarily save and restore uncommitted changes
- **Tagging**: Create and manage lightweight and annotated tags
- **Merging**: Merge branches with conflict resolution support

### Advanced Features
- **Git-like Ignore System**: Pattern matching with precedence rules (`.bitignore`)
- **Hooks System**: Pre-commit, post-commit, and pre-push hooks
- **Repository Maintenance**: Cleanup, optimization, and integrity checking
- **Error Handling**: Comprehensive error reporting and validation
- **Remote Operations**: Push/pull to remote repositories
- **Configuration Management**: User and repository-specific settings

## Requirements

### System Requirements
- **Operating System**: macOS, Linux, or Windows
- **Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Dependencies**: 
  - OpenSSL (for hashing)
  - libcurl (for remote operations)
  - zlib (for compression)
  - Google Test (for testing)

## Installation

### Quick Start
1. **Clone the repository**
  ```bash
    git clone https://github.com/Ahmadalmuhidat/bittrack.git
    cd bittrack
  ```
2. **Build the project**
  ```bash
    make compile
  ```
3. **Run tests (optional)**
  ```bash
    make test
  ```

### Manual Build
```bash
  # Production build
  ./bin/build.production.sh
  # Test build
  ./bin/build.test.sh
```

## Testing

### Run All Tests
```bash
make test
```

### Run Specific Test Categories
```bash
# Run only branch tests
./build/run_tests --gtest_filter="*branch*"
# Run only commit tests
./build/run_tests --gtest_filter="*commit*"
# Run with verbose output
./build/run_tests --gtest_verbose
```

## Project Structure
```
bittrack/
├── bin/                    # Build scripts
├── build/                  # Compiled binaries
├── include/                # Header files
├── libs/                   # External libraries
├── src/                    # Source files
│   ├── main.cpp            # Main application
│   └── *.cpp               # Implementation files
├── tests/                  # Test files
│   ├── main.test.cpp       # Main test runner
│   └── *.test.cpp          # Individual test modules
├── .bittrack/              # BitTrack repository data
├── .gitignore              # Git ignore rules
├── Makefile                # Build configuration
├── LICENSE                 # MIT License
└── README.md               # This file
```

## Development

### Building from Source
```bash
  # Clean previous builds
  make clean
  # Build production version
  make compile
  # Build and run tests
  make test
  # Clean build artifacts
  make clean
```

### Code Quality
```bash
  # Check code style
  ./bin/check_code_style.sh
  # Check code quality
  ./bin/check_code_quality.sh
```
### Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

### License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support
If you encounter any issues or have questions:
1. Check the [Issues](https://github.com/Ahmadalmuhidat/bittrack/issues) page
2. Create a new issue with detailed information
3. Include system information and error messages
