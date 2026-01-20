# Quickstart Guide: PosixLibPq

**Feature**: 001-entity-orm-mapping  
**Date**: 2026-01-20

## Prerequisites

- C++17 compiler (GCC 8+ or Clang 7+)
- PostgreSQL libpq 15.x
- CMake 3.16+

## Installation

### Build from Source

```bash
git clone <repository-url>
cd posixLibPq
mkdir build && cd build
cmake ..
make
sudo make install
```

### CMake Integration

```cmake
find_package(pq REQUIRED)
target_link_libraries(your_target PRIVATE pq::pq)
```

## Quick Start (5 Minutes)

### 1. Include Header

```cpp
#include <pq/pq.hpp>
```

### 2. Define Entity

```cpp
struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name")
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### 3. Connect & Use Repository

```cpp
int main() {
    // Connect
    pq::Connection conn("host=localhost dbname=mydb user=postgres");
    if (!conn.isConnected()) {
        std::cerr << conn.lastError() << std::endl;
        return 1;
    }
    
    // Create repository
    pq::Repository<User, int> userRepo(conn);
    
    // Save
    User user;
    user.name = "John Doe";
    user.email = "john@example.com";
    
    auto saved = userRepo.save(user);
    if (saved) {
        std::cout << "Saved with ID: " << saved->id << std::endl;
    }
    
    // Find by ID
    auto found = userRepo.findById(saved->id);
    if (found && *found) {
        std::cout << "Found: " << (*found)->name << std::endl;
    }
    
    // Find all
    auto all = userRepo.findAll();
    if (all) {
        for (const auto& u : *all) {
            std::cout << u.name << std::endl;
        }
    }
    
    return 0;
}
```

## Common Operations

### CRUD Operations

```cpp
// Create
auto saved = repo.save(entity);
auto savedMultiple = repo.saveAll({entity1, entity2});

// Read
auto byId = repo.findById(1);
auto all = repo.findAll();
auto exists = repo.existsById(1);
auto count = repo.count();

// Update
entity.name = "Updated";
auto updated = repo.update(entity);

// Delete
repo.remove(entity);
repo.removeById(1);
repo.removeAll({entity1, entity2});
```

### Raw Query (Custom Lookups)

```cpp
// Find by custom condition
auto result = repo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    {"john@example.com"}
);

// Multiple results
auto results = repo.executeQuery(
    "SELECT * FROM users WHERE name LIKE $1",
    {"%John%"}
);
```

### Transaction

```cpp
{
    pq::Transaction tx(conn);
    if (!tx) {
        std::cerr << "Failed to begin transaction" << std::endl;
        return;
    }
    
    repo.save(user1);
    repo.save(user2);
    
    auto result = tx.commit();
    if (!result) {
        std::cerr << result.error().message << std::endl;
    }
}  // Auto-rollback if commit not called
```

### Error Handling

```cpp
auto result = repo.findById(1);

if (result) {
    // Success
    if (*result) {
        // Found
        std::cout << (*result)->name << std::endl;
    } else {
        // Not found
        std::cout << "User not found" << std::endl;
    }
} else {
    // Error
    std::cerr << result.error().message << std::endl;
}
```

### Strict vs Relaxed Mapping

```cpp
// Default: strict (throws on unmapped columns)
pq::Repository<User, int> strictRepo(conn);

// Relaxed: ignore extra columns
pq::MapperConfig config;
config.ignoreExtraColumns = true;
pq::Repository<User, int> relaxedRepo(conn, config);
```

## Entity Definition Reference

### Column Flags

| Flag | Description |
|------|-------------|
| `PQ_PRIMARY_KEY` | Mark as primary key |
| `PQ_AUTO_INCREMENT` | Auto-generated value (SERIAL) |
| `PQ_NOT_NULL` | Not nullable |
| `PQ_UNIQUE` | Unique constraint |

### Supported Types

| C++ Type | PostgreSQL Type |
|----------|-----------------|
| `bool` | boolean |
| `int16_t` | smallint |
| `int` / `int32_t` | integer |
| `int64_t` | bigint |
| `float` | real |
| `double` | double precision |
| `std::string` | text |
| `std::optional<T>` | T (nullable) |

### Complete Entity Example

```cpp
struct Product {
    int64_t id{0};
    std::string name;
    double price{0.0};
    std::optional<std::string> description;
    bool active{true};
    
    PQ_ENTITY(Product, "products")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(price, "price", PQ_NOT_NULL)
        PQ_COLUMN(description, "description")
        PQ_COLUMN(active, "is_active")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(Product)
```

## Troubleshooting

### Connection Failed

```cpp
if (!conn.isConnected()) {
    // Check connection string
    // Verify PostgreSQL is running
    // Check firewall/network
    std::cerr << conn.lastError() << std::endl;
}
```

### Mapping Error

```cpp
// "Result contains column not mapped to entity"
// → Use strictColumnMapping = false or add missing column to entity
```

### NULL in Non-Optional Field

```cpp
// "NULL value in non-nullable column"
// → Use std::optional<T> for nullable columns
```

## Next Steps

- See `examples/usage_example.cpp` for complete examples
- Read `data-model.md` for detailed API documentation
- Check `tests/` for usage patterns
