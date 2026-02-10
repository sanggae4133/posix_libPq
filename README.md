# PosixLibPq

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A modern C++17 PostgreSQL ORM library featuring RAII resource management, compile-time type safety, and zero-cost abstractions.

[한국어](README.ko.md) | [Documentation](docs/README.md) | [한국어 문서](docs/ko/README.md)

## Features

- **RAII-First Design** - All resources (connections, results, transactions) are automatically managed
- **Type Safety** - Compile-time type checking with `std::optional` for NULL handling
- **Zero-Cost Abstractions** - Minimal overhead compared to raw libpq usage
- **JPA-Like API** - Declarative entity mapping with macros and Repository pattern
- **Connection Pooling** - Thread-safe connection pool for high-performance applications
- **Modern C++** - Uses C++17 features like `std::optional`, `std::string_view`, structured bindings

## Requirements

| Requirement | Version |
|-------------|---------|
| C++ Standard | C++17 |
| Compiler | GCC 8+, Clang 7+, MSVC 2019+ |
| PostgreSQL libpq | 15.x+ |
| CMake | 3.16+ |
| Google Test | (optional, for tests) |

## Quick Start

### Installation

**Using CMake FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
    posixlibpq
    GIT_REPOSITORY https://github.com/your-repo/posixLibPq.git
    GIT_TAG main
)
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(posixlibpq)

target_link_libraries(your_app PRIVATE pq::pq)
```

**Using as subdirectory:**

```cmake
add_subdirectory(external/posixLibPq)
target_link_libraries(your_app PRIVATE pq::pq)
```

### Define Entity

```cpp
#include <pq/pq.hpp>

struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;  // Nullable
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### CRUD Operations

```cpp
// Connect
pq::Connection conn("host=localhost dbname=mydb user=postgres");

// Create repository
pq::Repository<User, int> userRepo(conn);

// Create
User user;
user.name = "John";
user.email = "john@example.com";
auto saved = userRepo.save(user);
if (saved) {
    std::cout << "Saved with ID: " << saved->id << std::endl;
}

// Read
auto found = userRepo.findById(1);
if (found && *found) {
    std::cout << "Found: " << (*found)->name << std::endl;
}

// Update
if (saved) {
    saved->name = "Jane";
    userRepo.update(*saved);
}

// Delete
userRepo.removeById(1);
```

### Transactions

```cpp
{
    pq::Transaction tx(conn);
    if (!tx) {
        // Handle error
        return;
    }
    
    userRepo.save(user1);
    userRepo.save(user2);
    
    tx.commit();  // Explicit commit
}  // Auto-rollback if commit() not called
```

### Connection Pool

```cpp
pq::PoolConfig config;
config.connectionString = "host=localhost dbname=mydb";
config.minSize = 2;
config.maxSize = 10;

pq::ConnectionPool pool(config);

{
    auto conn = pool.acquire();
    if (conn) {
        conn->execute("SELECT * FROM users");
    }
}  // Connection returned to pool
```

## Building from Source

```bash
# Clone
git clone https://github.com/your-repo/posixLibPq.git
cd posixLibPq

# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure

# Or build with auto-test execution
cmake .. -DPQ_RUN_TESTS=ON
cmake --build .
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `PQ_BUILD_EXAMPLES` | ON | Build example programs |
| `PQ_BUILD_TESTS` | ON | Build unit tests |
| `PQ_RUN_TESTS` | OFF | Run tests after build |
| `PQ_INSTALL` | ON | Generate install target for `cmake --install` |
| `PQ_LIBPQ_INCLUDE_DIR` | (auto) | Manual path to `libpq-fe.h` directory |
| `PQ_LIBPQ_LIBRARY` | (auto) | Manual path to libpq library |

### Manual PostgreSQL Path

If CMake cannot find libpq automatically:

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

## Project Structure

```
posixLibPq/
├── include/pq/
│   ├── core/                 # Core components
│   │   ├── PqHandle.hpp      # RAII wrappers
│   │   ├── Result.hpp        # Result<T,E> error handling
│   │   ├── Types.hpp         # Type traits system
│   │   ├── Connection.hpp    # Database connection
│   │   ├── QueryResult.hpp   # Query results
│   │   ├── Transaction.hpp   # Transaction management
│   │   └── ConnectionPool.hpp# Connection pooling
│   ├── orm/                  # ORM layer
│   │   ├── Entity.hpp        # Entity macros
│   │   ├── Mapper.hpp        # Result-Entity mapping
│   │   └── Repository.hpp    # Repository pattern
│   └── pq.hpp               # Convenience header
├── src/core/                # Implementation files
├── cmake/                   # CMake package config
│   └── pqConfig.cmake.in    # find_package(pq) support template
├── docs/                    # Documentation
│   └── ko/                  # Korean documentation
├── examples/                # Example programs
├── tests/                   # Unit tests
└── CMakeLists.txt
```

### cmake/ Directory

The `cmake/pqConfig.cmake.in` file is a template for CMake package configuration. When you run `cmake --install`, it generates `pqConfig.cmake` that allows other projects to use this library via:

```cmake
find_package(pq REQUIRED)
target_link_libraries(your_app PRIVATE pq::pq)
```

## Documentation

- [Getting Started](docs/getting-started.md)
- [Entity Mapping](docs/entity-mapping.md)
- [Repository Pattern](docs/repository-pattern.md)
- [Transactions](docs/transactions.md)
- [Connection Pool](docs/connection-pool.md)
- [Error Handling](docs/error-handling.md)
- [Custom Queries](docs/custom-queries.md)
- [API Reference](docs/api-reference.md)
- [Type System](docs/type-system.md)
- [Stability Fixes (2026-02)](docs/stability-fixes-2026-02.md)

## Design Principles

### RAII (Resource Acquisition Is Initialization)

All libpq resources are managed with RAII:

```cpp
{
    pq::Connection conn("...");  // PQconnectdb
    // Use connection...
}  // Automatic PQfinish

{
    pq::Transaction tx(conn);  // BEGIN
    // Operations...
}  // Automatic ROLLBACK if not committed
```

### Result Type Error Handling

Operations return `DbResult<T>` instead of throwing exceptions:

```cpp
auto result = userRepo.findById(1);
if (result) {
    // Success
    auto& user = *result;
} else {
    // Error
    std::cerr << result.error().message << std::endl;
}
```

### Type-Safe NULL Handling

Use `std::optional` for nullable columns:

```cpp
struct User {
    int id;                           // NOT NULL
    std::optional<std::string> bio;   // NULL allowed
};

// NULL column mapped to non-optional throws MappingException
```

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

MIT License - see [LICENSE](LICENSE) file for details.
