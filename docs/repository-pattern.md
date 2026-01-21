# Repository Pattern

The Repository pattern provides a clean, type-safe interface for database CRUD operations on entities.

## Basic Usage

```cpp
#include <pq/pq.hpp>

// Assuming User entity is defined and registered
pq::Connection conn("host=localhost dbname=mydb user=postgres");
pq::Repository<User, int> userRepo(conn);
```

## Repository Template

```cpp
template<typename Entity, typename PK = int>
class Repository;
```

| Parameter | Description |
|-----------|-------------|
| `Entity` | The entity type (must be registered with `PQ_REGISTER_ENTITY`) |
| `PK` | Primary key type (default: `int`) |

## CRUD Operations

### Create (save)

```cpp
// Save a single entity
User user;
user.name = "John Doe";
user.email = "john@example.com";

auto result = userRepo.save(user);
if (result) {
    User& saved = *result;
    std::cout << "Saved with ID: " << saved.id << std::endl;
} else {
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

```cpp
// Save multiple entities
std::vector<User> users = {
    {0, "Alice", "alice@example.com"},
    {0, "Bob", "bob@example.com"},
    {0, "Charlie", "charlie@example.com"}
};

auto result = userRepo.saveAll(users);
if (result) {
    for (const auto& u : *result) {
        std::cout << "Saved: " << u.name << " (ID: " << u.id << ")" << std::endl;
    }
}
```

### Read (find)

```cpp
// Find by ID
auto result = userRepo.findById(1);
if (result) {
    if (*result) {
        // Found
        User& user = **result;
        std::cout << "Found: " << user.name << std::endl;
    } else {
        // Not found (but no error)
        std::cout << "User not found" << std::endl;
    }
} else {
    // Database error
    std::cerr << "Error: " << result.error().message << std::endl;
}
```

```cpp
// Find all
auto result = userRepo.findAll();
if (result) {
    for (const auto& user : *result) {
        std::cout << user.id << ": " << user.name << std::endl;
    }
}
```

```cpp
// Check existence
auto exists = userRepo.existsById(1);
if (exists && *exists) {
    std::cout << "User exists" << std::endl;
}
```

```cpp
// Count all
auto count = userRepo.count();
if (count) {
    std::cout << "Total users: " << *count << std::endl;
}
```

### Update

```cpp
auto findResult = userRepo.findById(1);
if (findResult && *findResult) {
    User& user = **findResult;
    user.name = "Updated Name";
    user.email = "updated@example.com";
    
    auto updateResult = userRepo.update(user);
    if (updateResult) {
        std::cout << "Updated successfully" << std::endl;
    }
}
```

### Delete (remove)

```cpp
// Remove by ID
auto result = userRepo.removeById(1);
if (result) {
    std::cout << "Removed " << *result << " row(s)" << std::endl;
}

// Remove entity
auto result = userRepo.remove(user);
if (result) {
    std::cout << "Removed " << *result << " row(s)" << std::endl;
}

// Remove multiple
std::vector<User> usersToDelete = {...};
auto result = userRepo.removeAll(usersToDelete);
if (result) {
    std::cout << "Removed " << *result << " row(s)" << std::endl;
}
```

## Custom Queries

For queries beyond basic CRUD, use `executeQuery` and `executeQueryOne`:

```cpp
// Find users by email domain
auto result = userRepo.executeQuery(
    "SELECT * FROM users WHERE email LIKE $1",
    {"%@example.com"}
);

if (result) {
    for (const auto& user : *result) {
        std::cout << user.name << ": " << user.email.value_or("N/A") << std::endl;
    }
}
```

```cpp
// Find single user by email
auto result = userRepo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    {"john@example.com"}
);

if (result && *result) {
    User& user = **result;
    std::cout << "Found: " << user.name << std::endl;
}
```

## API Reference

### Constructor

```cpp
Repository(Connection& conn, const MapperConfig& config = defaultMapperConfig())
```

| Parameter | Description |
|-----------|-------------|
| `conn` | Database connection (must outlive the repository) |
| `config` | Optional mapper configuration |

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `save(entity)` | `DbResult<Entity>` | Save new entity, returns entity with generated ID |
| `saveAll(entities)` | `DbResult<vector<Entity>>` | Save multiple entities |
| `findById(id)` | `DbResult<optional<Entity>>` | Find by primary key |
| `findAll()` | `DbResult<vector<Entity>>` | Find all entities |
| `update(entity)` | `DbResult<Entity>` | Update existing entity |
| `remove(entity)` | `DbResult<int>` | Remove entity by primary key |
| `removeById(id)` | `DbResult<int>` | Remove by primary key |
| `removeAll(entities)` | `DbResult<int>` | Remove multiple entities |
| `count()` | `DbResult<int64_t>` | Count all entities |
| `existsById(id)` | `DbResult<bool>` | Check if entity exists |
| `executeQuery(sql, params)` | `DbResult<vector<Entity>>` | Execute custom query |
| `executeQueryOne(sql, params)` | `DbResult<optional<Entity>>` | Execute query expecting 0-1 results |
| `connection()` | `Connection&` | Get underlying connection |
| `config()` | `MapperConfig&` | Get/modify mapper configuration |

## Mapper Configuration

```cpp
pq::MapperConfig config;
config.strictColumnMapping = true;   // Error on unmapped columns
config.ignoreExtraColumns = false;   // Don't ignore extra columns in result

pq::Repository<User, int> userRepo(conn, config);
```

| Option | Default | Description |
|--------|---------|-------------|
| `strictColumnMapping` | `true` | Throw error if result has columns not mapped to entity |
| `ignoreExtraColumns` | `false` | When `true`, ignore extra columns instead of erroring |

## Generated SQL

The repository generates standard SQL statements:

```cpp
// save() generates:
INSERT INTO users (name, email) VALUES ($1, $2) RETURNING *

// findById() generates:
SELECT * FROM users WHERE id = $1

// findAll() generates:
SELECT * FROM users

// update() generates:
UPDATE users SET name = $1, email = $2 WHERE id = $3 RETURNING *

// remove() generates:
DELETE FROM users WHERE id = $1
```

## Error Handling

All repository methods return `DbResult<T>`, which is either a success value or an error:

```cpp
auto result = userRepo.findById(1);

// Check and access
if (result) {
    // Success - result is truthy
    auto& value = *result;  // Access the value
} else {
    // Error - result is falsy
    DbError& err = result.error();
    std::cerr << "Error: " << err.message << std::endl;
    std::cerr << "SQLSTATE: " << err.sqlState << std::endl;
}

// Or use valueOr() for defaults
auto users = userRepo.findAll().valueOr(std::vector<User>{});
```

## Best Practices

1. **Keep repository lifetime shorter than connection**
   ```cpp
   pq::Connection conn(...);
   {
       pq::Repository<User, int> repo(conn);
       // Use repo...
   }  // repo destroyed, conn still valid
   ```

2. **Use transactions for multiple operations**
   ```cpp
   pq::Transaction tx(conn);
   userRepo.save(user1);
   userRepo.save(user2);
   tx.commit();
   ```

3. **Check results before accessing values**
   ```cpp
   auto result = userRepo.findById(id);
   if (result && *result) {  // Check both Result and optional
       // Safe to use
   }
   ```

4. **Use executeQuery for complex queries**
   ```cpp
   auto result = userRepo.executeQuery(
       "SELECT * FROM users WHERE created_at > $1 ORDER BY name LIMIT $2",
       {startDate, std::to_string(limit)}
   );
   ```

## Next Steps

- [Transactions](transactions.md) - Transaction management
- [Error Handling](error-handling.md) - Detailed error handling
- [Custom Queries](custom-queries.md) - Advanced query patterns
