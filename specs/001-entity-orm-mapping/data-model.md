# Data Model: Entity ORM Mapping

**Feature**: 001-entity-orm-mapping  
**Date**: 2026-01-20

## Core Entities

### 1. Connection

데이터베이스 연결을 관리하는 핵심 클래스

```cpp
namespace pq::core {

struct ConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database;
    std::string user;
    std::string password;
    int connectTimeoutSec = 10;
    
    std::string toConnectionString() const;
};

class Connection {
    PgConnPtr conn_;
    bool inTransaction_{false};
    
public:
    // Lifecycle
    Connection() = default;
    explicit Connection(std::string_view connectionString);
    explicit Connection(const ConnectionConfig& config);
    
    DbResult<void> connect(std::string_view connectionString);
    void disconnect() noexcept;
    bool isConnected() const noexcept;
    
    // Query execution
    DbResult<QueryResult> execute(std::string_view sql);
    DbResult<QueryResult> execute(std::string_view sql, 
                                  const std::vector<std::string>& params);
    
    template<typename... Args>
    DbResult<QueryResult> executeParams(std::string_view sql, Args&&... args);
    
    DbResult<int> executeUpdate(std::string_view sql);
    
    // Prepared statements
    DbResult<void> prepare(std::string_view name, std::string_view sql);
    DbResult<QueryResult> executePrepared(std::string_view name,
                                           const std::vector<std::string>& params);
    
    // Transaction
    DbResult<void> beginTransaction();
    DbResult<void> commit();
    DbResult<void> rollback();
    bool inTransaction() const noexcept;
    
    // Utilities
    std::string escapeString(std::string_view value) const;
    std::string escapeIdentifier(std::string_view identifier) const;
    int serverVersion() const noexcept;
};

}
```

### 2. QueryResult

쿼리 결과를 담는 RAII 래퍼

```cpp
namespace pq::core {

class Row {
    PGresult* result_;
    int rowIndex_;
    
public:
    int columnCount() const noexcept;
    bool isNull(int columnIndex) const noexcept;
    const char* getRaw(int columnIndex) const noexcept;
    const char* columnName(int columnIndex) const noexcept;
    int columnIndex(const char* name) const noexcept;
    
    template<typename T>
    T get(int columnIndex) const;
    
    template<typename T>
    T get(const char* name) const;
};

class QueryResult {
    PgResultPtr result_;
    int rowCount_;
    int columnCount_;
    
public:
    explicit QueryResult(PgResultPtr result);
    
    // Status
    bool isValid() const noexcept;
    bool isSuccess() const noexcept;
    ExecStatusType status() const noexcept;
    const char* errorMessage() const noexcept;
    
    // Data access
    int rowCount() const noexcept;
    int columnCount() const noexcept;
    bool empty() const noexcept;
    int affectedRows() const noexcept;
    
    Row row(int index) const;
    Row operator[](int index) const;
    
    // Iteration
    RowIterator begin() const;
    RowIterator end() const;
    
    std::optional<Row> first() const;
};

}
```

### 3. Transaction

RAII 트랜잭션 관리

```cpp
namespace pq::core {

class Transaction {
    Connection* conn_;
    bool committed_{false};
    bool valid_{false};
    
public:
    explicit Transaction(Connection& conn);
    ~Transaction();  // Auto-rollback if not committed
    
    Transaction(Transaction&&) noexcept;
    Transaction& operator=(Transaction&&) noexcept;
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    
    bool isValid() const noexcept;
    explicit operator bool() const noexcept;
    
    DbResult<void> commit();
    DbResult<void> rollback();
};

class Savepoint {
    Connection* conn_;
    std::string name_;
    bool released_{false};
    
public:
    Savepoint(Connection& conn, std::string_view name);
    ~Savepoint();  // Auto-rollback to savepoint
    
    DbResult<void> release();
    DbResult<void> rollbackTo();
};

}
```

### 4. Entity Metadata

Entity 매핑 메타데이터

```cpp
namespace pq::orm {

enum class ColumnFlags : uint32_t {
    None           = 0,
    PrimaryKey     = 1 << 0,
    AutoIncrement  = 1 << 1,
    NotNull        = 1 << 2,
    Unique         = 1 << 3,
};

struct ColumnInfo {
    std::string_view fieldName;
    std::string_view columnName;
    Oid pgType;
    ColumnFlags flags;
    bool isNullable;
    
    bool isPrimaryKey() const noexcept;
    bool isAutoIncrement() const noexcept;
};

template<typename Entity>
class EntityMetadata {
    std::string_view tableName_;
    std::vector<ColumnDescriptor<Entity>> columns_;
    const ColumnDescriptor<Entity>* primaryKey_;
    
public:
    EntityMetadata(std::string_view tableName);
    
    template<typename FieldType>
    void addColumn(std::string_view fieldName,
                   std::string_view columnName,
                   FieldType Entity::* memberPtr,
                   ColumnFlags flags = ColumnFlags::None);
    
    std::string_view tableName() const noexcept;
    const auto& columns() const noexcept;
    const ColumnDescriptor<Entity>* primaryKey() const noexcept;
};

template<typename T>
struct EntityMeta {
    static constexpr std::string_view tableName;
    static EntityMetadata<T>& metadata();
};

}
```

### 5. Repository

Entity별 데이터 접근 객체

```cpp
namespace pq::orm {

struct MapperConfig {
    bool strictColumnMapping = true;
    bool ignoreExtraColumns = false;
};

template<typename Entity, typename PK = int>
class Repository {
    core::Connection& conn_;
    EntityMapper<Entity> mapper_;
    SqlBuilder<Entity> sqlBuilder_;
    MapperConfig config_;
    
public:
    explicit Repository(core::Connection& conn,
                        const MapperConfig& config = {});
    
    // Create
    DbResult<Entity> save(const Entity& entity);
    DbResult<std::vector<Entity>> saveAll(const std::vector<Entity>& entities);
    
    // Read
    DbResult<std::optional<Entity>> findById(const PK& id);
    DbResult<std::vector<Entity>> findAll();
    DbResult<int64_t> count();
    DbResult<bool> existsById(const PK& id);
    
    // Update
    DbResult<Entity> update(const Entity& entity);
    
    // Delete
    DbResult<int> remove(const Entity& entity);
    DbResult<int> removeById(const PK& id);
    DbResult<int> removeAll(const std::vector<Entity>& entities);
    
    // Raw Query
    DbResult<std::vector<Entity>> executeQuery(
        std::string_view sql,
        const std::vector<std::string>& params = {});
    
    DbResult<std::optional<Entity>> executeQueryOne(
        std::string_view sql,
        const std::vector<std::string>& params = {});
};

}
```

### 6. Result<T, E>

에러 핸들링 타입

```cpp
namespace pq {

struct DbError {
    std::string message;
    std::string sqlState;
    int errorCode{0};
    
    const char* what() const noexcept;
};

template<typename T, typename E = DbError>
class Result {
    std::variant<T, E> data_;
    
public:
    // Construction
    Result(const T& value);
    Result(T&& value);
    static Result ok(T value);
    static Result error(E err);
    
    // Observers
    bool hasValue() const noexcept;
    bool hasError() const noexcept;
    explicit operator bool() const noexcept;
    
    // Access
    T& value() &;
    const T& value() const&;
    E& error() &;
    const E& error() const&;
    
    // Utilities
    template<typename U>
    T valueOr(U&& defaultValue) const&;
    
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>, E>;
    
    // Pointer-like
    T* operator->();
    T& operator*();
};

template<typename T>
using DbResult = Result<T, DbError>;

}
```

### 7. TypeTraits

타입 매핑 시스템

```cpp
namespace pq {

namespace oid {
    constexpr Oid BOOL      = 16;
    constexpr Oid INT2      = 21;
    constexpr Oid INT4      = 23;
    constexpr Oid INT8      = 20;
    constexpr Oid FLOAT4    = 700;
    constexpr Oid FLOAT8    = 701;
    constexpr Oid TEXT      = 25;
    constexpr Oid VARCHAR   = 1043;
    constexpr Oid TIMESTAMP = 1114;
}

template<typename T, typename Enable = void>
struct PgTypeTraits;

template<>
struct PgTypeTraits<int> {
    static constexpr Oid pgOid = oid::INT4;
    static constexpr const char* pgTypeName = "integer";
    static constexpr bool isNullable = false;
    
    static std::string toString(int value);
    static int fromString(const char* str);
};

// Similar specializations for: bool, int16_t, int64_t, float, double, 
// std::string, std::optional<T>

}
```

## Entity Relationships

```
┌─────────────┐      uses       ┌─────────────┐
│  Repository │ ───────────────▶│  Connection │
└─────────────┘                 └─────────────┘
       │                              │
       │ maps                         │ creates
       ▼                              ▼
┌─────────────┐               ┌─────────────┐
│   Entity    │               │ Transaction │
│  (User-     │               │   (RAII)    │
│  defined)   │               └─────────────┘
└─────────────┘                      │
       │                             │
       │ metadata                    │ uses
       ▼                             ▼
┌─────────────┐               ┌─────────────┐
│ EntityMeta  │               │ QueryResult │
│  <Entity>   │               │    + Row    │
└─────────────┘               └─────────────┘
```

## State Transitions

### Connection State

```
[Disconnected] ──connect()──▶ [Connected]
                                  │
                      ◀──disconnect()──┘
```

### Transaction State

```
[None] ──begin()──▶ [Active] ──commit()──▶ [Committed]
                       │
                       └──rollback()──▶ [Rolledback]
                       │
                       └──(destructor)──▶ [Auto-rolledback]
```

## Validation Rules

| Entity | Field | Rule |
|--------|-------|------|
| Connection | connectionString | Non-empty, valid format |
| Entity | Primary Key | Required, unique |
| Entity | Column mapping | Type must match PostgreSQL |
| Repository | Entity | Must be registered with PQ_REGISTER_ENTITY |
| Transaction | Connection | Must be valid and connected |
