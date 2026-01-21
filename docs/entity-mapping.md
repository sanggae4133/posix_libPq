# Entity Mapping

PosixLibPq uses a macro-based declarative approach to map C++ structs to PostgreSQL tables, similar to JPA annotations in Java.

## Basic Entity Definition

```cpp
#include <pq/pq.hpp>

struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    // Entity mapping declaration
    PQ_ENTITY(User, "users")                                    // Entity class, table name
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT) // Field, column, flags
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")                               // No flags = nullable
    PQ_ENTITY_END()
};

// Must be called outside the struct
PQ_REGISTER_ENTITY(User)
```

## Entity Macros

### PQ_ENTITY(ClassName, TableName)

Begins an entity mapping block.

| Parameter | Description |
|-----------|-------------|
| `ClassName` | The C++ struct/class name |
| `TableName` | The PostgreSQL table name (string literal) |

### PQ_COLUMN(Field, ColumnName, Flags...)

Maps a C++ field to a database column.

| Parameter | Description |
|-----------|-------------|
| `Field` | The C++ member variable name |
| `ColumnName` | The database column name (string literal) |
| `Flags` | Optional column flags (can be combined with `\|`) |

### PQ_ENTITY_END()

Ends the entity mapping block.

### PQ_REGISTER_ENTITY(ClassName)

Registers the entity for use with repositories. **Must be called outside the struct definition.**

## Column Flags

| Flag | Description |
|------|-------------|
| `PQ_PRIMARY_KEY` | Marks as primary key |
| `PQ_AUTO_INCREMENT` | Auto-generated value (SERIAL/BIGSERIAL) |
| `PQ_NOT_NULL` | Column cannot be NULL |
| `PQ_UNIQUE` | Column has unique constraint |
| `PQ_INDEX` | Column should be indexed |

Flags can be combined:

```cpp
PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
PQ_COLUMN(email, "email", PQ_NOT_NULL | PQ_UNIQUE)
```

## Supported Types

### Basic Types

| C++ Type | PostgreSQL Type | OID |
|----------|-----------------|-----|
| `bool` | `BOOLEAN` | 16 |
| `int16_t` | `SMALLINT` | 21 |
| `int32_t` / `int` | `INTEGER` | 23 |
| `int64_t` | `BIGINT` | 20 |
| `float` | `REAL` | 700 |
| `double` | `DOUBLE PRECISION` | 701 |
| `std::string` | `TEXT` | 25 |

### Nullable Types

Use `std::optional<T>` for nullable columns:

```cpp
struct User {
    int id;                           // NOT NULL
    std::string name;                 // NOT NULL
    std::optional<std::string> bio;   // NULL allowed
    std::optional<int> age;           // NULL allowed
};
```

When the database returns NULL:
- `std::optional` fields become `std::nullopt`
- Non-optional fields throw `MappingException`

## Complete Example

```cpp
#include <pq/pq.hpp>
#include <chrono>

struct Product {
    int64_t id{0};
    std::string name;
    std::string sku;
    double price{0.0};
    std::optional<std::string> description;
    bool active{true};
    int stockCount{0};
    
    PQ_ENTITY(Product, "products")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(sku, "sku", PQ_NOT_NULL | PQ_UNIQUE)
        PQ_COLUMN(price, "price", PQ_NOT_NULL)
        PQ_COLUMN(description, "description")
        PQ_COLUMN(active, "is_active")
        PQ_COLUMN(stockCount, "stock_count", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(Product)
```

Corresponding SQL table:

```sql
CREATE TABLE products (
    id BIGSERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    sku TEXT NOT NULL UNIQUE,
    price DOUBLE PRECISION NOT NULL,
    description TEXT,
    is_active BOOLEAN DEFAULT true,
    stock_count INTEGER NOT NULL DEFAULT 0
);
```

## Column Name vs Field Name

The column name in the database can differ from the C++ field name:

```cpp
struct User {
    int userId;                    // C++ field: userId
    std::string firstName;         // C++ field: firstName
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(userId, "user_id", PQ_PRIMARY_KEY)      // DB column: user_id
        PQ_COLUMN(firstName, "first_name", PQ_NOT_NULL)   // DB column: first_name
    PQ_ENTITY_END()
};
```

## Accessing Metadata

You can access entity metadata programmatically:

```cpp
// Get metadata
const auto& meta = pq::orm::EntityMeta<User>::metadata();

// Table name
std::cout << "Table: " << meta.tableName() << std::endl;

// Iterate columns
for (const auto& col : meta.columns()) {
    std::cout << "Column: " << col.info.columnName 
              << " (field: " << col.info.fieldName << ")"
              << " Type OID: " << col.info.pgType
              << std::endl;
}

// Get primary key
const auto* pk = meta.primaryKey();
if (pk) {
    std::cout << "Primary key: " << pk->info.columnName << std::endl;
}

// Find column by name
const auto* emailCol = meta.findColumn("email");
```

## Type Traits

The library uses `PgTypeTraits<T>` to convert between C++ and PostgreSQL types:

```cpp
// Get PostgreSQL OID for a type
Oid oid = pq::PgTypeTraits<int>::pgOid;  // 23 (INT4)

// Get type name
const char* name = pq::PgTypeTraits<int>::pgTypeName;  // "integer"

// Check if nullable
bool nullable = pq::PgTypeTraits<std::optional<int>>::isNullable;  // true

// Convert to string
std::string str = pq::PgTypeTraits<int>::toString(42);  // "42"

// Parse from string
int value = pq::PgTypeTraits<int>::fromString("42");  // 42
```

## Best Practices

1. **Always use `std::optional` for nullable columns** - Prevents runtime errors when NULL is returned

2. **Initialize primitive fields** - Provide default values to avoid undefined behavior:
   ```cpp
   int count{0};        // Good
   double price{0.0};   // Good
   bool active{true};   // Good
   ```

3. **Use appropriate integer types** - Match the PostgreSQL column type:
   ```cpp
   int32_t for INTEGER
   int64_t for BIGINT
   int16_t for SMALLINT
   ```

4. **Keep entities simple** - Entities should be plain data structures (POD-like)

5. **Register entities once** - Call `PQ_REGISTER_ENTITY` in a single translation unit or header

## Next Steps

- [Repository Pattern](repository-pattern.md) - CRUD operations
- [Custom Queries](custom-queries.md) - Complex queries with entity mapping
