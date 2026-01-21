# PosixLibPq Documentation

Welcome to the PosixLibPq documentation. This library provides a modern C++17 PostgreSQL ORM with RAII semantics, type safety, and zero-cost abstractions.

## Documentation Index

### Getting Started
- [**Getting Started**](getting-started.md) - Installation, building, and quick start guide

### Core Concepts
- [**Entity Mapping**](entity-mapping.md) - How to define and map entities to PostgreSQL tables
- [**Repository Pattern**](repository-pattern.md) - CRUD operations using the Repository pattern
- [**Error Handling**](error-handling.md) - Result types and error management

### Advanced Features
- [**Transactions**](transactions.md) - Transaction management with RAII
- [**Connection Pool**](connection-pool.md) - Connection pooling for high-performance applications
- [**Custom Queries**](custom-queries.md) - Executing custom SQL queries

### Reference
- [**API Reference**](api-reference.md) - Complete API documentation
- [**Type System**](type-system.md) - C++ to PostgreSQL type mapping

## Quick Links

```cpp
#include <pq/pq.hpp>

// Define an entity
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

// Use it
pq::Connection conn("host=localhost dbname=mydb user=postgres");
pq::Repository<User, int> userRepo(conn);

User user;
user.name = "John";
auto saved = userRepo.save(user);
```

## Project Links

- [GitHub Repository](https://github.com/your-repo/posixLibPq)
- [Issue Tracker](https://github.com/your-repo/posixLibPq/issues)
- [License (MIT)](../LICENSE)
- [한국어 문서 (Korean Documentation)](ko/README.md)