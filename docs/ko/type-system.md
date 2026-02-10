# 타입 시스템

PosixLibPq는 `PgTypeTraits`를 사용하여 C++ 타입과 PostgreSQL 타입 간의 타입 안전 매핑을 제공합니다.

## 지원 타입

### 기본 타입 매핑

| C++ 타입 | PostgreSQL 타입 | OID | 예시 값 |
|----------|-----------------|-----|---------|
| `bool` | `BOOLEAN` | 16 | `true`, `false` |
| `int16_t` | `SMALLINT` | 21 | `-32768` ~ `32767` |
| `int32_t` / `int` | `INTEGER` | 23 | `-2147483648` ~ `2147483647` |
| `int64_t` | `BIGINT` | 20 | `-9223372036854775808` ~ `9223372036854775807` |
| `float` | `REAL` | 700 | `3.14f` |
| `double` | `DOUBLE PRECISION` | 701 | `3.14159265359` |
| `std::string` | `TEXT` | 25 | `"Hello, World!"` |
| `Date` | `DATE` | 1082 | `{year, month, day}` |
| `Time` | `TIME` | 1083 | `{hour, minute, second, millisecond}` |
| `std::chrono::system_clock::time_point` | `TIMESTAMP` | 1114 | 밀리초 정밀도 |
| `TimestampTz` | `TIMESTAMPTZ` | 1184 | UTC 시각 + 오프셋(분) |
| `Numeric` | `NUMERIC` | 1700 | 문자열 기반 정밀도 보존 |
| `Uuid` | `UUID` | 2950 | 문자열 기반 UUID |
| `Jsonb` | `JSONB` | 3802 | 문자열 기반 JSONB |

### Nullable 타입

| C++ 타입 | PostgreSQL | 설명 |
|----------|------------|------|
| `std::optional<T>` | `T` 또는 `NULL` | nullable 타입 래퍼 |

```cpp
struct User {
    int id;                           // NOT NULL
    std::string name;                 // NOT NULL (string이라도)
    std::optional<std::string> bio;   // NULL 가능
    std::optional<int> age;           // NULL 가능
};
```

## PgTypeTraits 사용법

### OID 얻기

```cpp
#include <pq/core/Types.hpp>

// PostgreSQL OID 얻기
constexpr Oid intOid = pq::PgTypeTraits<int>::pgOid;      // 23
constexpr Oid boolOid = pq::PgTypeTraits<bool>::pgOid;    // 16
constexpr Oid textOid = pq::PgTypeTraits<std::string>::pgOid;  // 25
```

### 타입 이름

```cpp
const char* name = pq::PgTypeTraits<int>::pgTypeName;  // "integer"
const char* name2 = pq::PgTypeTraits<double>::pgTypeName;  // "double precision"
const char* name3 = pq::PgTypeTraits<std::string>::pgTypeName;  // "text"
```

### Nullable 여부 확인

```cpp
// 기본 타입
constexpr bool a = pq::PgTypeTraits<int>::isNullable;  // false

// Optional 타입
constexpr bool b = pq::PgTypeTraits<std::optional<int>>::isNullable;  // true
```

### 문자열 변환

```cpp
// C++ → 문자열
std::string str1 = pq::PgTypeTraits<int>::toString(42);     // "42"
std::string str2 = pq::PgTypeTraits<bool>::toString(true);  // "t"
std::string str3 = pq::PgTypeTraits<double>::toString(3.14); // "3.14"

// 문자열 → C++
int val1 = pq::PgTypeTraits<int>::fromString("42");         // 42
bool val2 = pq::PgTypeTraits<bool>::fromString("t");        // true
double val3 = pq::PgTypeTraits<double>::fromString("3.14"); // 3.14
```

## Optional 처리

`std::optional<T>`는 NULL을 처리할 수 있습니다:

```cpp
// 값이 있는 경우
std::optional<int> val = 42;
bool isNull = !val.has_value();  // false
std::string str = pq::PgTypeTraits<std::optional<int>>::toString(val);  // "42"

// 값이 없는 경우 (NULL)
std::optional<int> nullVal = std::nullopt;
bool isNull2 = !nullVal.has_value();  // true
```

### 쿼리 파라미터에서

```cpp
// 타입 안전 쿼리
conn.executeParams(
    "INSERT INTO users (name, age) VALUES ($1, $2)",
    std::string("John"),           // NOT NULL
    std::optional<int>(std::nullopt)  // NULL로 전송
);
```

### 결과에서

```cpp
auto result = conn.execute("SELECT name, age FROM users");
if (result) {
    for (const auto& row : *result) {
        std::string name = row.get<std::string>("name");
        
        // age가 NULL이면 std::nullopt
        auto age = row.get<std::optional<int>>("age");
        if (age) {
            std::cout << name << "의 나이: " << *age << std::endl;
        } else {
            std::cout << name << "의 나이: 미상" << std::endl;
        }
    }
}
```

## Boolean 표현

PostgreSQL boolean은 다음과 같이 표현됩니다:

| C++ | PostgreSQL 문자열 |
|-----|-------------------|
| `true` | `"t"` |
| `false` | `"f"` |

```cpp
// C++ → PostgreSQL
pq::PgTypeTraits<bool>::toString(true);   // "t"
pq::PgTypeTraits<bool>::toString(false);  // "f"

// PostgreSQL → C++ (여러 형식 허용)
pq::PgTypeTraits<bool>::fromString("t");      // true
pq::PgTypeTraits<bool>::fromString("true");   // true
pq::PgTypeTraits<bool>::fromString("1");      // true
pq::PgTypeTraits<bool>::fromString("yes");    // true
pq::PgTypeTraits<bool>::fromString("f");      // false
```

## 숫자 타입

### 정수

```cpp
// 적절한 타입 사용이 중요
row.get<int16_t>("smallint_col");   // SMALLINT
row.get<int32_t>("integer_col");    // INTEGER
row.get<int64_t>("bigint_col");     // BIGINT
```

### 부동소수점

```cpp
row.get<float>("real_col");         // REAL (4바이트)
row.get<double>("double_col");      // DOUBLE PRECISION (8바이트)
```

### 정밀도 주의사항

```cpp
// float는 정밀도 손실 가능
float f = 0.1f;  // 실제로는 0.10000000149...

// 재무 데이터에는 Numeric 래퍼 사용 권장 (정밀도 보존)
pq::Numeric numeric = row.get<pq::Numeric>("price");
std::string raw = numeric.value;
```

## 문자열 타입

모든 PostgreSQL 문자열 타입은 `std::string`으로 매핑:

| PostgreSQL | C++ |
|------------|-----|
| `TEXT` | `std::string` |
| `VARCHAR(n)` | `std::string` |
| `CHAR(n)` | `std::string` |

```cpp
// 모두 std::string으로 읽기
std::string text = row.get<std::string>("text_col");
std::string varchar = row.get<std::string>("varchar_col");
```

## 파라미터 변환

쿼리 파라미터는 문자열로 변환됩니다:

```cpp
// 내부적으로 자동 변환
conn.execute("SELECT * FROM users WHERE id = $1", {"42"});

// executeParams는 타입 안전
conn.executeParams("SELECT * FROM users WHERE id = $1", 42);
// 내부적으로 PgTypeTraits<int>::toString(42) → "42"
```

## 결과 타입 변환

```cpp
auto result = conn.execute("SELECT * FROM users");
if (result) {
    Row row = result->row(0);
    
    // 인덱스로 접근
    int id = row.get<int>(0);
    
    // 컬럼 이름으로 접근
    std::string name = row.get<std::string>("name");
    
    // Optional로 nullable 처리
    auto email = row.get<std::optional<std::string>>("email");
}
```

## 커스텀 타입 확장

새로운 타입에 대한 `PgTypeTraits` 특수화 가능:

```cpp
// 예: UUID 타입 지원
struct UUID {
    std::array<uint8_t, 16> data;
    std::string toString() const;
    static UUID fromString(const std::string& str);
};

namespace pq {
template<>
struct PgTypeTraits<UUID> {
    static constexpr Oid pgOid = 2950;  // UUID OID
    static constexpr const char* pgTypeName = "uuid";
    static constexpr bool isNullable = false;
    
    static std::string toString(const UUID& value) {
        return value.toString();
    }
    
    static UUID fromString(const char* str) {
        return UUID::fromString(str);
    }
};
}
```

## 타입 안전성 팁

### 1. Nullable 컬럼에 Optional 사용

```cpp
// 나쁨: NULL일 때 예외 발생 가능
std::string maybeNull = row.get<std::string>("nullable_col");

// 좋음: NULL 안전
auto maybeNull = row.get<std::optional<std::string>>("nullable_col");
```

### 2. 정수 타입 일치시키기

```cpp
// BIGINT 컬럼에 int 사용 시 오버플로 가능
int bad = row.get<int>("bigint_col");  // 값이 크면 잘림

// 올바른 타입 사용
int64_t good = row.get<int64_t>("bigint_col");
```

### 3. Entity 필드 초기화

```cpp
struct User {
    int id{0};           // 기본값 초기화
    double score{0.0};   // 기본값 초기화
    bool active{false};  // 기본값 초기화
};
```

## OID 상수

```cpp
namespace pq::oid {
    constexpr Oid BOOL      = 16;
    constexpr Oid INT8      = 20;   // BIGINT
    constexpr Oid INT2      = 21;   // SMALLINT
    constexpr Oid INT4      = 23;   // INTEGER
    constexpr Oid TEXT      = 25;
    constexpr Oid FLOAT4    = 700;  // REAL
    constexpr Oid FLOAT8    = 701;  // DOUBLE PRECISION
    constexpr Oid VARCHAR   = 1043;
    // ... 더 많은 OID
}
```

## 확장 타입 사용 예시

```cpp
pq::Date d = row.get<pq::Date>("ship_date");
pq::Time t = row.get<pq::Time>("ship_time");
auto ts = row.get<std::chrono::system_clock::time_point>("created_at");
pq::TimestampTz tsTz = row.get<pq::TimestampTz>("updated_at");
pq::Uuid id = row.get<pq::Uuid>("external_id");
pq::Jsonb payload = row.get<pq::Jsonb>("payload");
```

nullable 컬럼은 동일하게 `std::optional<T>`를 사용합니다:

```cpp
auto maybeAmount = row.get<std::optional<pq::Numeric>>("amount");
auto maybePayload = row.get<std::optional<pq::Jsonb>>("payload");
```

## 현재 직접 미지원 타입

- `BYTEA` (바이너리)
- `ARRAY` 타입
- 사용자 정의 복합 타입(별도 `PgTypeTraits` 특수화 필요)

## 다음 단계

- [Entity 매핑](entity-mapping.md) - Entity에서 타입 사용
- [커스텀 쿼리](custom-queries.md) - 쿼리 결과 타입
