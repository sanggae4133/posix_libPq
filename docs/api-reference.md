# API Reference

Complete API reference for PosixLibPq.

## Namespaces

```cpp
namespace pq {           // Root namespace (convenience types re-exported here)
namespace pq::core {     // Core database components
namespace pq::orm {      // ORM components
}
```

---

## Core Components

### Connection

Database connection management with RAII.

```cpp
namespace pq::core {

class Connection {
public:
    // Constructors
    Connection();                                      // Unconnected
    explicit Connection(std::string_view connStr);    // Connect immediately
    explicit Connection(const ConnectionConfig& config);
    
    // Move-only
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;
    
    // Connection management
    DbResult<void> connect(std::string_view connStr);
    DbResult<void> connect(const ConnectionConfig& config);
    void disconnect() noexcept;
    
    // State
    bool isConnected() const noexcept;
    ConnStatusType status() const noexcept;
    const char* lastError() const noexcept;
    int serverVersion() const noexcept;
    bool inTransaction() const noexcept;
    
    // Query execution
    DbResult<QueryResult> execute(std::string_view sql);
    DbResult<QueryResult> execute(std::string_view sql, 
                                   std::initializer_list<std::string> params);
    DbResult<QueryResult> execute(std::string_view sql,
                                   const std::vector<std::string>& params);
    
    template<typename... Args>
    DbResult<QueryResult> executeParams(std::string_view sql, Args&&... args);
    
    DbResult<int> executeUpdate(std::string_view sql);
    DbResult<int> executeUpdate(std::string_view sql,
                                 std::initializer_list<std::string> params);
    
    // Prepared statements
    DbResult<void> prepare(std::string_view name, std::string_view sql);
    DbResult<QueryResult> executePrepared(std::string_view name,
                                           std::initializer_list<std::string> params);
    
    // Transactions
    DbResult<void> beginTransaction();
    DbResult<void> commit();
    DbResult<void> rollback();
    
    // Escaping
    std::string escapeString(std::string_view value) const;
    std::string escapeIdentifier(std::string_view identifier) const;
    
    // Raw access
    PGconn* raw() const noexcept;
};

} // namespace pq::core
```

### ConnectionConfig

```cpp
namespace pq::core {

struct ConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database;
    std::string user;
    std::string password;
    std::string options;
    int connectTimeoutSec = 10;
    
    std::string toConnectionString() const;
    static ConnectionConfig fromConnectionString(std::string_view connStr);
};

} // namespace pq::core
```

### QueryResult

```cpp
namespace pq::core {

class QueryResult {
public:
    explicit QueryResult(PgResultPtr result);
    
    // Move-only
    QueryResult(QueryResult&&) = default;
    QueryResult& operator=(QueryResult&&) = default;
    
    // Status
    bool isValid() const noexcept;
    bool isSuccess() const noexcept;
    ExecStatusType status() const noexcept;
    const char* errorMessage() const noexcept;
    const char* sqlState() const noexcept;
    
    // Dimensions
    int rowCount() const noexcept;
    int columnCount() const noexcept;
    bool empty() const noexcept;
    int affectedRows() const noexcept;
    
    // Column metadata
    const char* columnName(int index) const noexcept;
    int columnIndex(const char* name) const noexcept;
    Oid columnType(int index) const noexcept;
    std::vector<std::string> columnNames() const;
    
    // Row access
    Row row(int index) const;
    Row operator[](int index) const;
    std::optional<Row> first() const;
    
    // Iteration
    RowIterator begin() const;
    RowIterator end() const;
    
    // Raw access
    PGresult* raw() const noexcept;
};

} // namespace pq::core
```

### Row

```cpp
namespace pq::core {

class Row {
public:
    Row(PGresult* result, int rowIndex);
    
    // Column count
    int columnCount() const noexcept;
    
    // NULL checking
    bool isNull(int columnIndex) const noexcept;
    
    // Raw access
    const char* getRaw(int columnIndex) const noexcept;
    
    // Column names
    const char* columnName(int columnIndex) const noexcept;
    int columnIndex(const char* name) const noexcept;
    
    // Typed access
    template<typename T> T get(int columnIndex) const;
    template<typename T> T get(const char* name) const;
    template<typename T> T get(std::string_view name) const;
};

} // namespace pq::core
```

### Transaction

```cpp
namespace pq::core {

class Transaction {
public:
    explicit Transaction(Connection& conn);
    
    // Move-only
    Transaction(Transaction&&) noexcept;
    Transaction& operator=(Transaction&&) noexcept;
    
    ~Transaction();  // Auto-rollback if not committed
    
    // State
    bool isValid() const noexcept;
    explicit operator bool() const noexcept;
    bool isCommitted() const noexcept;
    
    // Operations
    DbResult<void> commit();
    DbResult<void> rollback();
    
    // Access
    Connection& connection() noexcept;
    const Connection& connection() const noexcept;
};

} // namespace pq::core
```

### Savepoint

```cpp
namespace pq::core {

class Savepoint {
public:
    Savepoint(Connection& conn, std::string_view name);
    
    // Move-only
    Savepoint(Savepoint&&) noexcept;
    Savepoint& operator=(Savepoint&&) noexcept;
    
    ~Savepoint();  // Auto-rollback if not released
    
    // State
    bool isValid() const noexcept;
    explicit operator bool() const noexcept;
    
    // Operations
    DbResult<void> release();
    DbResult<void> rollbackTo();
};

} // namespace pq::core
```

### ConnectionPool

```cpp
namespace pq::core {

struct PoolConfig {
    std::string connectionString;
    size_t maxSize = 10;
    size_t minSize = 1;
    std::chrono::milliseconds acquireTimeout{5000};
    std::chrono::milliseconds idleTimeout{60000};
    bool validateOnAcquire = true;
};

class PooledConnection {
public:
    // Move-only
    PooledConnection(PooledConnection&&) noexcept;
    PooledConnection& operator=(PooledConnection&&) noexcept;
    
    ~PooledConnection();  // Returns to pool
    
    // Access
    Connection& operator*() noexcept;
    const Connection& operator*() const noexcept;
    Connection* operator->() noexcept;
    const Connection* operator->() const noexcept;
    
    bool isValid() const noexcept;
    void release();
};

class ConnectionPool {
public:
    explicit ConnectionPool(const PoolConfig& config);
    ~ConnectionPool();
    
    // Non-copyable, non-movable
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    // Acquire
    DbResult<PooledConnection> acquire();
    DbResult<PooledConnection> acquire(std::chrono::milliseconds timeout);
    
    // Statistics
    size_t idleCount() const noexcept;
    size_t activeCount() const noexcept;
    size_t totalCount() const noexcept;
    size_t maxSize() const noexcept;
    
    // Management
    void drain();
    void shutdown();
};

} // namespace pq::core
```

---

## Result Types

### Result<T, E>

```cpp
namespace pq {

template<typename T, typename E = DbError>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;
    
    // Constructors
    Result(const T& value);
    Result(T&& value);
    template<typename... Args>
    Result(InPlaceError, Args&&... args);
    
    // Factory methods
    static Result ok(T value);
    static Result error(E err);
    template<typename... Args>
    static Result error(Args&&... args);
    
    // Observers
    bool hasValue() const noexcept;
    bool hasError() const noexcept;
    explicit operator bool() const noexcept;
    
    // Value access
    T& value() &;
    const T& value() const&;
    T&& value() &&;
    
    // Error access
    E& error() &;
    const E& error() const&;
    
    // Default value
    template<typename U> T valueOr(U&& defaultValue) const&;
    template<typename U> T valueOr(U&& defaultValue) &&;
    
    // Transform
    template<typename F> auto map(F&& f) const& -> Result<...>;
    template<typename F> auto map(F&& f) && -> Result<...>;
    
    // Pointer-like access
    T* operator->();
    const T* operator->() const;
    T& operator*() &;
    const T& operator*() const&;
};

// Void specialization
template<typename E>
class Result<void, E>;

// Convenience alias
template<typename T>
using DbResult = Result<T, DbError>;

} // namespace pq
```

### DbError

```cpp
namespace pq {

struct DbError {
    std::string message;
    std::string sqlState;  // PostgreSQL SQLSTATE (5 chars)
    int errorCode{0};
    
    DbError() = default;
    explicit DbError(std::string msg, std::string state = "", int code = 0);
    
    const char* what() const noexcept;
};

} // namespace pq
```

---

## ORM Components

### Entity Macros

```cpp
// Define entity mapping
#define PQ_ENTITY(ClassName, TableName)

// Map column
#define PQ_COLUMN(Field, ColumnName, ...)

// End entity definition
#define PQ_ENTITY_END()

// Register entity (required, outside struct)
#define PQ_REGISTER_ENTITY(ClassName)

// Column flags
#define PQ_PRIMARY_KEY     // ColumnFlags::PrimaryKey
#define PQ_AUTO_INCREMENT  // ColumnFlags::AutoIncrement
#define PQ_NOT_NULL        // ColumnFlags::NotNull
#define PQ_UNIQUE          // ColumnFlags::Unique
#define PQ_INDEX           // ColumnFlags::Index
```

### ColumnFlags

```cpp
namespace pq::orm {

enum class ColumnFlags : uint32_t {
    None           = 0,
    PrimaryKey     = 1 << 0,
    AutoIncrement  = 1 << 1,
    NotNull        = 1 << 2,
    Unique         = 1 << 3,
    Index          = 1 << 4,
};

// Bitwise operators
ColumnFlags operator|(ColumnFlags a, ColumnFlags b);
ColumnFlags operator&(ColumnFlags a, ColumnFlags b);
bool hasFlag(ColumnFlags flags, ColumnFlags flag);

} // namespace pq::orm
```

### Repository

```cpp
namespace pq::orm {

template<typename Entity, typename PK = int>
class Repository {
public:
    using EntityType = Entity;
    using PrimaryKeyType = PK;
    
    explicit Repository(core::Connection& conn, 
                        const MapperConfig& config = defaultMapperConfig());
    
    // Create
    DbResult<Entity> save(const Entity& entity);
    DbResult<std::vector<Entity>> saveAll(const std::vector<Entity>& entities);
    
    // Read
    DbResult<std::optional<Entity>> findById(const PK& id);
    template<typename... Args>
    DbResult<std::optional<Entity>> findById(Args&&... ids);
    DbResult<std::vector<Entity>> findAll();
    DbResult<int64_t> count();
    DbResult<bool> existsById(const PK& id);
    template<typename... Args>
    DbResult<bool> existsById(Args&&... ids);
    
    // Update
    DbResult<Entity> update(const Entity& entity);
    
    // Delete
    DbResult<int> remove(const Entity& entity);
    DbResult<int> removeById(const PK& id);
    template<typename... Args>
    DbResult<int> removeById(Args&&... ids);
    DbResult<int> removeAll(const std::vector<Entity>& entities);
    
    // Custom queries
    DbResult<std::vector<Entity>> executeQuery(
        std::string_view sql,
        const std::vector<std::string>& params = {});
    DbResult<std::optional<Entity>> executeQueryOne(
        std::string_view sql,
        const std::vector<std::string>& params = {});
    
    // Access
    core::Connection& connection() noexcept;
    MapperConfig& config() noexcept;
};

} // namespace pq::orm
```

### MapperConfig

```cpp
namespace pq::orm {

struct MapperConfig {
    bool strictColumnMapping = true;
    bool ignoreExtraColumns = false;
    bool autoValidateSchema = false;
    SchemaValidationMode schemaValidationMode = SchemaValidationMode::Strict;
};

MapperConfig& defaultMapperConfig();

} // namespace pq::orm
```

### SchemaValidator

```cpp
namespace pq::orm {

enum class SchemaValidationMode : uint8_t {
    Strict,
    Lenient
};

enum class ValidationIssueType : uint8_t {
    ConnectionError,
    TableNotFound,
    ColumnNotFound,
    TypeMismatch,
    NullableMismatch,
    LengthMismatch,
    ExtraColumn
};

struct ValidationIssue {
    ValidationIssueType type;
    std::string entityName;
    std::string tableName;
    std::string columnName;
    std::string expected;
    std::string actual;
    std::string message;
};

struct ValidationResult {
    std::vector<ValidationIssue> errors;
    std::vector<ValidationIssue> warnings;

    bool isValid() const noexcept;
    std::size_t errorCount() const noexcept;
    std::size_t warningCount() const noexcept;
    std::string summary() const;
};

class SchemaValidator {
public:
    explicit SchemaValidator(SchemaValidationMode mode = SchemaValidationMode::Strict);

    SchemaValidationMode mode() const noexcept;

    template<typename Entity>
    ValidationResult validate(core::Connection& conn) const;
};

} // namespace pq::orm
```

### EntityMetadata

```cpp
namespace pq::orm {

template<typename Entity>
class EntityMetadata {
public:
    EntityMetadata(std::string_view tableName);
    
    template<typename FieldType>
    void addColumn(std::string_view fieldName,
                   std::string_view columnName,
                   FieldType Entity::* memberPtr,
                   ColumnFlags flags = ColumnFlags::None,
                   int maxLength = -1);
    
    std::string_view tableName() const noexcept;
    const std::vector<ColumnDescriptor<Entity>>& columns() const noexcept;
    const ColumnDescriptor<Entity>* primaryKey() const noexcept;
    const std::vector<std::size_t>& primaryKeyIndices() const noexcept;
    std::vector<const ColumnDescriptor<Entity>*> primaryKeys() const;
    const ColumnDescriptor<Entity>* findColumn(std::string_view name) const;
};

// Access metadata
template<typename T>
struct EntityMeta {
    static constexpr std::string_view tableName;
    static const EntityMetadata<T>& metadata();
};

} // namespace pq::orm
```

---

## Type System

### PgTypeTraits

```cpp
namespace pq {

// Primary template
template<typename T, typename Enable = void>
struct PgTypeTraits;

// Specializations for:
// - bool
// - int16_t
// - int32_t / int
// - int64_t
// - float
// - double
// - std::string
// - Date
// - Time
// - std::chrono::system_clock::time_point (timestamp)
// - TimestampTz (timestamptz)
// - Numeric (numeric)
// - Uuid (uuid)
// - Jsonb (jsonb)
// - std::optional<T>

template<typename T>
struct PgTypeTraits {
    static constexpr Oid pgOid;
    static constexpr const char* pgTypeName;
    static constexpr bool isNullable;
    
    static std::string toString(const T& value);
    static T fromString(const char* str);
};

} // namespace pq
```

### PostgreSQL OIDs

```cpp
namespace pq::oid {
    constexpr Oid BOOL      = 16;
    constexpr Oid BYTEA     = 17;
    constexpr Oid CHAR      = 18;
    constexpr Oid INT8      = 20;   // bigint
    constexpr Oid INT2      = 21;   // smallint
    constexpr Oid INT4      = 23;   // integer
    constexpr Oid TEXT      = 25;
    constexpr Oid OID       = 26;
    constexpr Oid FLOAT4    = 700;  // real
    constexpr Oid FLOAT8    = 701;  // double precision
    constexpr Oid VARCHAR   = 1043;
    constexpr Oid DATE      = 1082;
    constexpr Oid TIME      = 1083;
    constexpr Oid TIMESTAMP = 1114;
    constexpr Oid TIMESTAMPTZ = 1184;
    constexpr Oid NUMERIC   = 1700;
    constexpr Oid UUID      = 2950;
    constexpr Oid JSONB     = 3802;
}
```

---

## Convenience Header

```cpp
// pq/pq.hpp - Include everything
#include <pq/pq.hpp>

// Brings into pq namespace:
namespace pq {
    using core::Connection;
    using core::ConnectionConfig;
    using core::QueryResult;
    using core::Row;
    using core::Transaction;
    using core::Savepoint;
    using core::ConnectionPool;
    using core::PooledConnection;
    using core::PoolConfig;
    using orm::Repository;
    using orm::MapperConfig;
    using orm::ColumnFlags;
    using orm::SchemaValidator;
    using orm::SchemaValidationMode;
    using orm::ValidationIssue;
    using orm::ValidationIssueType;
    using orm::ValidationResult;
}
```
