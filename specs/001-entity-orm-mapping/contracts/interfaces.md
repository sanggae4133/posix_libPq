# API Contracts: Entity ORM Mapping

**Feature**: 001-entity-orm-mapping  
**Date**: 2026-01-20

## Core Interfaces

### IConnection

```cpp
namespace pq::core {

/**
 * @brief Database connection interface
 */
class IConnection {
public:
    virtual ~IConnection() = default;
    
    // Lifecycle
    virtual DbResult<void> connect(std::string_view connectionString) = 0;
    virtual void disconnect() noexcept = 0;
    virtual bool isConnected() const noexcept = 0;
    
    // Query execution
    virtual DbResult<QueryResult> execute(std::string_view sql) = 0;
    virtual DbResult<QueryResult> execute(
        std::string_view sql, 
        const std::vector<std::string>& params) = 0;
    virtual DbResult<int> executeUpdate(std::string_view sql) = 0;
    
    // Transaction
    virtual DbResult<void> beginTransaction() = 0;
    virtual DbResult<void> commit() = 0;
    virtual DbResult<void> rollback() = 0;
    virtual bool inTransaction() const noexcept = 0;
};

}
```

### IRepository

```cpp
namespace pq::orm {

/**
 * @brief Repository interface for entity CRUD operations
 */
template<typename Entity, typename PK>
class IRepository {
public:
    virtual ~IRepository() = default;
    
    // Create
    virtual DbResult<Entity> save(const Entity& entity) = 0;
    virtual DbResult<std::vector<Entity>> saveAll(
        const std::vector<Entity>& entities) = 0;
    
    // Read
    virtual DbResult<std::optional<Entity>> findById(const PK& id) = 0;
    virtual DbResult<std::vector<Entity>> findAll() = 0;
    virtual DbResult<int64_t> count() = 0;
    virtual DbResult<bool> existsById(const PK& id) = 0;
    
    // Update
    virtual DbResult<Entity> update(const Entity& entity) = 0;
    
    // Delete
    virtual DbResult<int> remove(const Entity& entity) = 0;
    virtual DbResult<int> removeById(const PK& id) = 0;
    virtual DbResult<int> removeAll(const std::vector<Entity>& entities) = 0;
    
    // Raw Query
    virtual DbResult<std::vector<Entity>> executeQuery(
        std::string_view sql,
        const std::vector<std::string>& params = {}) = 0;
    virtual DbResult<std::optional<Entity>> executeQueryOne(
        std::string_view sql,
        const std::vector<std::string>& params = {}) = 0;
};

}
```

### IMapper

```cpp
namespace pq::orm {

/**
 * @brief Entity mapper interface
 */
template<typename Entity>
class IMapper {
public:
    virtual ~IMapper() = default;
    
    virtual Entity mapRow(const core::Row& row) const = 0;
    virtual std::vector<Entity> mapAll(const core::QueryResult& result) const = 0;
    virtual std::optional<Entity> mapOne(const core::QueryResult& result) const = 0;
};

}
```

## Data Contracts

### DbError

```cpp
/**
 * @brief Database error information
 */
struct DbError {
    std::string message;      // Human-readable error message
    std::string sqlState;     // PostgreSQL SQLSTATE code (5 chars)
    int errorCode{0};         // Numeric error code
};
```

### ConnectionConfig

```cpp
/**
 * @brief Connection configuration
 */
struct ConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database;
    std::string user;
    std::string password;
    std::string options;           // Additional libpq options
    int connectTimeoutSec = 10;
};
```

### MapperConfig

```cpp
/**
 * @brief Entity mapper configuration
 */
struct MapperConfig {
    bool strictColumnMapping = true;   // Throw on unmapped result columns
    bool ignoreExtraColumns = false;   // Override strict for extra columns
};
```

### ColumnInfo

```cpp
/**
 * @brief Column metadata
 */
struct ColumnInfo {
    std::string_view fieldName;    // C++ field name
    std::string_view columnName;   // Database column name
    Oid pgType;                    // PostgreSQL type OID
    ColumnFlags flags;             // PRIMARY_KEY, AUTO_INCREMENT, etc.
    bool isNullable;               // Whether column can be NULL
};
```

## Method Signatures

### Connection

| Method | Input | Output | Description |
|--------|-------|--------|-------------|
| `connect` | `string_view connStr` | `DbResult<void>` | Establish connection |
| `disconnect` | - | `void` | Close connection |
| `execute` | `string_view sql` | `DbResult<QueryResult>` | Execute query |
| `execute` | `string_view sql, vector<string> params` | `DbResult<QueryResult>` | Execute parameterized query |
| `executeUpdate` | `string_view sql` | `DbResult<int>` | Execute INSERT/UPDATE/DELETE |
| `beginTransaction` | - | `DbResult<void>` | Start transaction |
| `commit` | - | `DbResult<void>` | Commit transaction |
| `rollback` | - | `DbResult<void>` | Rollback transaction |

### Repository

| Method | Input | Output | Description |
|--------|-------|--------|-------------|
| `save` | `Entity&` | `DbResult<Entity>` | Insert entity |
| `saveAll` | `vector<Entity>&` | `DbResult<vector<Entity>>` | Batch insert |
| `findById` | `PK& id` | `DbResult<optional<Entity>>` | Find by primary key |
| `findAll` | - | `DbResult<vector<Entity>>` | Get all entities |
| `update` | `Entity&` | `DbResult<Entity>` | Update entity |
| `remove` | `Entity&` | `DbResult<int>` | Delete entity |
| `removeById` | `PK& id` | `DbResult<int>` | Delete by ID |
| `removeAll` | `vector<Entity>&` | `DbResult<int>` | Batch delete |
| `count` | - | `DbResult<int64_t>` | Count entities |
| `existsById` | `PK& id` | `DbResult<bool>` | Check existence |
| `executeQuery` | `string_view sql, vector<string> params` | `DbResult<vector<Entity>>` | Raw query |
| `executeQueryOne` | `string_view sql, vector<string> params` | `DbResult<optional<Entity>>` | Raw query single |

### QueryResult

| Method | Input | Output | Description |
|--------|-------|--------|-------------|
| `rowCount` | - | `int` | Number of rows |
| `columnCount` | - | `int` | Number of columns |
| `row` | `int index` | `Row` | Get row at index |
| `operator[]` | `int index` | `Row` | Get row at index |
| `begin` | - | `RowIterator` | Iterator begin |
| `end` | - | `RowIterator` | Iterator end |
| `first` | - | `optional<Row>` | First row or empty |
| `affectedRows` | - | `int` | Rows affected by DML |

### Row

| Method | Input | Output | Description |
|--------|-------|--------|-------------|
| `get<T>` | `int columnIndex` | `T` | Get typed value |
| `get<T>` | `const char* name` | `T` | Get typed value by name |
| `isNull` | `int columnIndex` | `bool` | Check if NULL |
| `getRaw` | `int columnIndex` | `const char*` | Get raw string |
| `columnName` | `int columnIndex` | `const char*` | Get column name |
| `columnIndex` | `const char* name` | `int` | Get column index |

## Error Codes

| SQLSTATE | Description | Handling |
|----------|-------------|----------|
| `08xxx` | Connection exception | Retry or fail |
| `22xxx` | Data exception | Validation error |
| `23xxx` | Integrity constraint violation | Business logic error |
| `42xxx` | Syntax error or access violation | Programming error |

## Usage Examples

### Save Entity

```cpp
// Input
User user;
user.name = "John";

// Call
auto result = repo.save(user);

// Output (success)
// result.hasValue() == true
// result.value().id == <generated_id>

// Output (error)
// result.hasError() == true
// result.error().message == "duplicate key value..."
```

### Find By ID

```cpp
// Input
int id = 1;

// Call
auto result = repo.findById(id);

// Output (found)
// result.hasValue() == true
// result.value().has_value() == true
// result.value()->name == "John"

// Output (not found)
// result.hasValue() == true
// result.value().has_value() == false

// Output (error)
// result.hasError() == true
```

### Raw Query

```cpp
// Input
std::string_view sql = "SELECT * FROM users WHERE email = $1";
std::vector<std::string> params = {"john@example.com"};

// Call
auto result = repo.executeQueryOne(sql, params);

// Output
// Same as findById
```
