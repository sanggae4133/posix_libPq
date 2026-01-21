# Getting Started

This guide will help you get PosixLibPq up and running in your project.

## Requirements

| Requirement | Version |
|-------------|---------|
| C++ Standard | C++17 or higher |
| Compiler | GCC 8+, Clang 7+, or MSVC 2019+ |
| PostgreSQL libpq | 15.x+ (PQlibVersion >= 150007) |
| CMake | 3.16+ |
| Google Test | (optional, for tests) |

## Installation

### Option 1: Using CMake FetchContent (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    posixlibpq
    GIT_REPOSITORY https://github.com/your-repo/posixLibPq.git
    GIT_TAG main
)

# Disable examples and tests for library-only build
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(posixlibpq)

# Link to your target
target_link_libraries(your_app PRIVATE pq::pq)
```

### Option 2: Using as a Subdirectory

Clone or copy the repository into your project:

```bash
git submodule add https://github.com/your-repo/posixLibPq.git external/posixLibPq
```

Then in your `CMakeLists.txt`:

```cmake
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/posixLibPq)

target_link_libraries(your_app PRIVATE pq::pq)
```

### Option 3: System Installation

```bash
# Clone the repository
git clone https://github.com/your-repo/posixLibPq.git
cd posixLibPq

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Install (may require sudo)
cmake --install .
```

Then use in your project:

```cmake
find_package(pq REQUIRED)
target_link_libraries(your_app PRIVATE pq::pq)
```

## Building from Source

### Prerequisites

Install PostgreSQL development libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libpq-dev

# RHEL/CentOS
sudo yum install postgresql-devel

# macOS (Homebrew)
brew install libpq
```

### Build Commands

```bash
mkdir build && cd build

# Basic build
cmake ..
cmake --build .

# With tests
cmake .. -DPQ_BUILD_TESTS=ON
cmake --build .
ctest --output-on-failure

# With automatic test execution after build
cmake .. -DPQ_RUN_TESTS=ON
cmake --build .

# Release build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `PQ_BUILD_EXAMPLES` | ON | Build example programs |
| `PQ_BUILD_TESTS` | ON | Build unit tests |
| `PQ_RUN_TESTS` | OFF | Run tests automatically after build |
| `PQ_INSTALL` | ON | Generate install target for `cmake --install` |
| `PQ_LIBPQ_INCLUDE_DIR` | (auto) | Manual path to `libpq-fe.h` directory |
| `PQ_LIBPQ_LIBRARY` | (auto) | Manual path to libpq library file |

#### Option Details

- **`PQ_INSTALL`**: When `ON`, generates install targets that allow `cmake --install .` to install headers, libraries, and CMake config files to the system. Set to `OFF` when using as a subdirectory in another project.

- **`PQ_LIBPQ_INCLUDE_DIR` / `PQ_LIBPQ_LIBRARY`**: Manually specify PostgreSQL paths when automatic detection fails. Both must be set together.

### Manual PostgreSQL Path Configuration

If CMake cannot find libpq automatically, you can specify paths manually:

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/path/to/include \
  -DPQ_LIBPQ_LIBRARY=/path/to/libpq.so
```

### macOS with Homebrew libpq

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/opt/homebrew/opt/libpq/include \
  -DPQ_LIBPQ_LIBRARY=/opt/homebrew/opt/libpq/lib/libpq.dylib
```

### Linux Common Paths

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/usr/include/postgresql \
  -DPQ_LIBPQ_LIBRARY=/usr/lib/x86_64-linux-gnu/libpq.so
```

## Quick Start

### 1. Include the Header

```cpp
#include <pq/pq.hpp>
```

### 2. Define Your Entity

```cpp
struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### 3. Connect to Database

```cpp
// Using connection string
pq::Connection conn("host=localhost dbname=mydb user=postgres password=secret");

// Or using ConnectionConfig
pq::ConnectionConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "mydb";
config.user = "postgres";
config.password = "secret";

pq::Connection conn(config);
```

### 4. Perform CRUD Operations

```cpp
pq::Repository<User, int> userRepo(conn);

// Create
User user;
user.name = "John Doe";
user.email = "john@example.com";
auto savedResult = userRepo.save(user);
if (savedResult) {
    std::cout << "Saved user with ID: " << savedResult->id << std::endl;
}

// Read
auto findResult = userRepo.findById(1);
if (findResult && *findResult) {
    User& found = **findResult;
    std::cout << "Found: " << found.name << std::endl;
}

// Update
if (savedResult) {
    savedResult->name = "Jane Doe";
    userRepo.update(*savedResult);
}

// Delete
userRepo.removeById(1);
```

## Next Steps

- [Entity Mapping](entity-mapping.md) - Learn how to map complex entities
- [Repository Pattern](repository-pattern.md) - Full repository API
- [Transactions](transactions.md) - Transaction management
- [Error Handling](error-handling.md) - Handling errors properly
