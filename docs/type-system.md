# Type System

PosixLibPq provides a compile-time type system for mapping between C++ types and PostgreSQL types.

## Type Mapping Table

| C++ Type | PostgreSQL Type | OID | Notes |
|----------|-----------------|-----|-------|
| `bool` | `BOOLEAN` | 16 | "t"/"f" text format |
| `int16_t` | `SMALLINT` | 21 | |
| `int32_t` / `int` | `INTEGER` | 23 | |
| `int64_t` | `BIGINT` | 20 | |
| `float` | `REAL` | 700 | |
| `double` | `DOUBLE PRECISION` | 701 | |
| `std::string` | `TEXT` | 25 | |
| `std::optional<T>` | Same as T | Same as T | NULL handling |

## PgTypeTraits

The `PgTypeTraits<T>` template provides type information and conversion functions:

```cpp
template<typename T>
struct PgTypeTraits {
    static constexpr Oid pgOid;           // PostgreSQL OID
    static constexpr const char* pgTypeName;  // SQL type name
    static constexpr bool isNullable;     // Can represent NULL
    
    static std::string toString(const T& value);  // C++ -> PostgreSQL text
    static T fromString(const char* str);          // PostgreSQL text -> C++
};
```

### Example Usage

```cpp
#include <pq/core/Types.hpp>

// Get PostgreSQL OID
Oid oid = pq::PgTypeTraits<int>::pgOid;  // 23

// Get type name
const char* name = pq::PgTypeTraits<int>::pgTypeName;  // "integer"

// Check if nullable
bool nullable = pq::PgTypeTraits<int>::isNullable;  // false
bool optNullable = pq::PgTypeTraits<std::optional<int>>::isNullable;  // true

// Convert to string (for SQL parameter)
std::string str = pq::PgTypeTraits<int>::toString(42);  // "42"

// Parse from string (from SQL result)
int value = pq::PgTypeTraits<int>::fromString("42");  // 42
```

## Built-in Type Traits

### Boolean

```cpp
template<>
struct PgTypeTraits<bool> {
    static constexpr Oid pgOid = oid::BOOL;  // 16
    static constexpr const char* pgTypeName = "boolean";
    static constexpr bool isNullable = false;
    
    static std::string toString(bool value) {
        return value ? "t" : "f";
    }
    
    static bool fromString(const char* str) {
        return str && (str[0] == 't' || str[0] == 'T' || str[0] == '1');
    }
};
```

### Integers

```cpp
// int16_t -> SMALLINT
PgTypeTraits<int16_t>::pgOid;      // 21
PgTypeTraits<int16_t>::pgTypeName;  // "smallint"

// int32_t / int -> INTEGER
PgTypeTraits<int32_t>::pgOid;      // 23
PgTypeTraits<int>::pgOid;          // 23
PgTypeTraits<int>::pgTypeName;     // "integer"

// int64_t -> BIGINT
PgTypeTraits<int64_t>::pgOid;      // 20
PgTypeTraits<int64_t>::pgTypeName;  // "bigint"
```

### Floating Point

```cpp
// float -> REAL
PgTypeTraits<float>::pgOid;       // 700
PgTypeTraits<float>::pgTypeName;  // "real"

// double -> DOUBLE PRECISION
PgTypeTraits<double>::pgOid;      // 701
PgTypeTraits<double>::pgTypeName; // "double precision"
```

### String

```cpp
// std::string -> TEXT
PgTypeTraits<std::string>::pgOid;      // 25
PgTypeTraits<std::string>::pgTypeName;  // "text"
```

## Optional (Nullable) Types

`std::optional<T>` wraps any type to make it nullable:

```cpp
template<typename T>
struct PgTypeTraits<std::optional<T>> {
    using InnerTraits = PgTypeTraits<T>;
    
    static constexpr Oid pgOid = InnerTraits::pgOid;
    static constexpr const char* pgTypeName = InnerTraits::pgTypeName;
    static constexpr bool isNullable = true;  // Always true for optional
    
    static std::string toString(const std::optional<T>& value) {
        if (value) {
            return InnerTraits::toString(*value);
        }
        return "";  // Empty string represents NULL
    }
    
    static std::optional<T> fromString(const char* str) {
        if (!str) {
            return std::nullopt;  // NULL
        }
        return InnerTraits::fromString(str);
    }
};
```

### Usage

```cpp
// std::optional<int> for nullable INTEGER
std::optional<int> age = 25;
std::string str = pq::PgTypeTraits<std::optional<int>>::toString(age);  // "25"

std::optional<int> noAge = std::nullopt;
std::string nullStr = pq::PgTypeTraits<std::optional<int>>::toString(noAge);  // ""

auto parsed = pq::PgTypeTraits<std::optional<int>>::fromString("42");  // optional(42)
auto parsedNull = pq::PgTypeTraits<std::optional<int>>::fromString(nullptr);  // nullopt
```

## Type Detection Helpers

```cpp
// Check if type is std::optional
template<typename T>
inline constexpr bool isOptionalV = IsOptional<T>::value;

isOptionalV<int>                      // false
isOptionalV<std::optional<int>>       // true
isOptionalV<std::optional<std::string>>  // true

// Get inner type of optional
template<typename T>
using OptionalInnerT = typename OptionalInner<T>::type;

OptionalInnerT<int>                      // int
OptionalInnerT<std::optional<int>>       // int
OptionalInnerT<std::optional<std::string>>  // std::string
```

## ParamConverter

`ParamConverter<T>` helps convert values for SQL parameters:

```cpp
template<typename T>
struct ParamConverter {
    std::string value;  // Converted string value
    const char* ptr;    // Pointer to value (or nullptr for NULL)
    bool isNull;        // True if NULL
    
    explicit ParamConverter(const T& v);
};
```

### Usage

```cpp
// Regular value
pq::ParamConverter<int> intConv(42);
intConv.ptr;    // "42"
intConv.isNull; // false

// Optional with value
std::optional<int> optVal = 99;
pq::ParamConverter<std::optional<int>> optConv(optVal);
optConv.ptr;    // "99"
optConv.isNull; // false

// Optional without value (NULL)
std::optional<int> nullVal;
pq::ParamConverter<std::optional<int>> nullConv(nullVal);
nullConv.ptr;    // nullptr
nullConv.isNull; // true
```

## NullTerminatedString

Helper class to ensure strings are null-terminated for libpq:

```cpp
class NullTerminatedString {
public:
    explicit NullTerminatedString(std::string_view sv);
    explicit NullTerminatedString(const std::string& s);
    explicit NullTerminatedString(const char* s);
    
    const char* c_str() const noexcept;
    operator const char*() const noexcept;
};
```

### Usage

```cpp
std::string_view sv = "hello";
pq::NullTerminatedString nts(sv);
const char* cstr = nts.c_str();  // Safe null-terminated string
```

## PostgreSQL OID Constants

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

## Adding Custom Type Support

To support a custom type, specialize `PgTypeTraits`:

```cpp
// Example: UUID as std::array<uint8_t, 16>
#include <array>

namespace pq {

template<>
struct PgTypeTraits<std::array<uint8_t, 16>> {
    static constexpr Oid pgOid = oid::UUID;
    static constexpr const char* pgTypeName = "uuid";
    static constexpr bool isNullable = false;
    
    static std::string toString(const std::array<uint8_t, 16>& value) {
        // Convert to UUID string format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        char buf[37];
        snprintf(buf, sizeof(buf),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            value[0], value[1], value[2], value[3],
            value[4], value[5], value[6], value[7],
            value[8], value[9], value[10], value[11],
            value[12], value[13], value[14], value[15]);
        return buf;
    }
    
    static std::array<uint8_t, 16> fromString(const char* str) {
        std::array<uint8_t, 16> result{};
        // Parse UUID string...
        return result;
    }
};

} // namespace pq
```

## Best Practices

1. **Always use `std::optional` for nullable columns**
   ```cpp
   struct User {
       int id;                           // NOT NULL
       std::optional<std::string> bio;   // NULL allowed
   };
   ```

2. **Match C++ types to PostgreSQL types**
   ```cpp
   // INTEGER column
   int32_t value;  // Good
   int value;      // Good
   
   // BIGINT column
   int64_t value;  // Good - use int64_t, not long
   
   // SMALLINT column
   int16_t value;  // Good
   ```

3. **Initialize primitive fields**
   ```cpp
   struct Entity {
       int id{0};          // Initialize to 0
       double price{0.0};  // Initialize to 0.0
       bool active{true};  // Initialize to default
   };
   ```

## Next Steps

- [Entity Mapping](entity-mapping.md) - Using types in entities
- [Custom Queries](custom-queries.md) - Type-safe query results
