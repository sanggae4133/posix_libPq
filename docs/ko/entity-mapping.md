# Entity 매핑

PosixLibPq는 Java의 JPA 어노테이션과 유사하게, 매크로 기반의 선언적 방식으로 C++ 구조체를 PostgreSQL 테이블에 매핑합니다.

## 기본 Entity 정의

```cpp
#include <pq/pq.hpp>

struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    // Entity 매핑 선언
    PQ_ENTITY(User, "users")                                    // Entity 클래스, 테이블명
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT) // 필드, 컬럼, 플래그
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")                               // 플래그 없음 = nullable
    PQ_ENTITY_END()
};

// 반드시 구조체 외부에서 호출
PQ_REGISTER_ENTITY(User)
```

## Entity 매크로

### PQ_ENTITY(ClassName, TableName)

Entity 매핑 블록을 시작합니다.

| 파라미터 | 설명 |
|----------|------|
| `ClassName` | C++ 구조체/클래스 이름 |
| `TableName` | PostgreSQL 테이블 이름 (문자열 리터럴) |

### PQ_COLUMN(Field, ColumnName, Flags...)

C++ 필드를 데이터베이스 컬럼에 매핑합니다.

| 파라미터 | 설명 |
|----------|------|
| `Field` | C++ 멤버 변수 이름 |
| `ColumnName` | 데이터베이스 컬럼 이름 (문자열 리터럴) |
| `Flags` | 선택적 컬럼 플래그 (`\|`로 조합 가능) |

### PQ_ENTITY_END()

Entity 매핑 블록을 종료합니다.

### PQ_REGISTER_ENTITY(ClassName)

Entity를 Repository에서 사용할 수 있도록 등록합니다. **반드시 구조체 정의 외부에서 호출해야 합니다.**

## 컬럼 플래그

| 플래그 | 설명 |
|--------|------|
| `PQ_PRIMARY_KEY` | 기본 키로 지정 |
| `PQ_AUTO_INCREMENT` | 자동 생성 값 (SERIAL/BIGSERIAL) |
| `PQ_NOT_NULL` | NULL 불가 컬럼 |
| `PQ_UNIQUE` | 유니크 제약 조건 |
| `PQ_INDEX` | 인덱스 대상 컬럼 |

플래그 조합 예시:

```cpp
PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
PQ_COLUMN(email, "email", PQ_NOT_NULL | PQ_UNIQUE)
```

## 지원 타입

### 기본 타입

| C++ 타입 | PostgreSQL 타입 | OID |
|----------|-----------------|-----|
| `bool` | `BOOLEAN` | 16 |
| `int16_t` | `SMALLINT` | 21 |
| `int32_t` / `int` | `INTEGER` | 23 |
| `int64_t` | `BIGINT` | 20 |
| `float` | `REAL` | 700 |
| `double` | `DOUBLE PRECISION` | 701 |
| `std::string` | `TEXT` | 25 |

### Nullable 타입

NULL 가능 컬럼에는 `std::optional<T>`를 사용:

```cpp
struct User {
    int id;                           // NOT NULL
    std::string name;                 // NOT NULL
    std::optional<std::string> bio;   // NULL 허용
    std::optional<int> age;           // NULL 허용
};
```

데이터베이스에서 NULL을 반환할 때:
- `std::optional` 필드는 `std::nullopt`가 됨
- non-optional 필드는 `MappingException` 발생

## 전체 예제

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

대응하는 SQL 테이블:

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

## 컬럼 이름 vs 필드 이름

데이터베이스의 컬럼 이름이 C++ 필드 이름과 다를 수 있습니다:

```cpp
struct User {
    int userId;                    // C++ 필드: userId
    std::string firstName;         // C++ 필드: firstName
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(userId, "user_id", PQ_PRIMARY_KEY)      // DB 컬럼: user_id
        PQ_COLUMN(firstName, "first_name", PQ_NOT_NULL)   // DB 컬럼: first_name
    PQ_ENTITY_END()
};
```

## 메타데이터 접근

프로그래밍 방식으로 Entity 메타데이터에 접근할 수 있습니다:

```cpp
// 메타데이터 가져오기
const auto& meta = pq::orm::EntityMeta<User>::metadata();

// 테이블 이름
std::cout << "테이블: " << meta.tableName() << std::endl;

// 컬럼 순회
for (const auto& col : meta.columns()) {
    std::cout << "컬럼: " << col.info.columnName 
              << " (필드: " << col.info.fieldName << ")"
              << " 타입 OID: " << col.info.pgType
              << std::endl;
}

// 기본 키 가져오기
const auto* pk = meta.primaryKey();
if (pk) {
    std::cout << "기본 키: " << pk->info.columnName << std::endl;
}

// 이름으로 컬럼 찾기
const auto* emailCol = meta.findColumn("email");
```

## 타입 트레이트

라이브러리는 `PgTypeTraits<T>`를 사용하여 C++와 PostgreSQL 타입 간 변환을 수행합니다:

```cpp
// 타입에 대한 PostgreSQL OID 가져오기
Oid oid = pq::PgTypeTraits<int>::pgOid;  // 23 (INT4)

// 타입 이름 가져오기
const char* name = pq::PgTypeTraits<int>::pgTypeName;  // "integer"

// nullable 여부 확인
bool nullable = pq::PgTypeTraits<std::optional<int>>::isNullable;  // true

// 문자열로 변환
std::string str = pq::PgTypeTraits<int>::toString(42);  // "42"

// 문자열에서 파싱
int value = pq::PgTypeTraits<int>::fromString("42");  // 42
```

## 모범 사례

1. **nullable 컬럼에는 항상 `std::optional` 사용** - NULL 반환 시 런타임 에러 방지

2. **기본 타입 필드 초기화** - 정의되지 않은 동작 방지:
   ```cpp
   int count{0};        // 좋음
   double price{0.0};   // 좋음
   bool active{true};   // 좋음
   ```

3. **적절한 정수 타입 사용** - PostgreSQL 컬럼 타입에 맞추기:
   ```cpp
   int32_t  // INTEGER용
   int64_t  // BIGINT용
   int16_t  // SMALLINT용
   ```

4. **Entity는 단순하게 유지** - Entity는 단순한 데이터 구조체(POD 유사)여야 함

5. **Entity는 한 번만 등록** - `PQ_REGISTER_ENTITY`는 단일 번역 단위나 헤더에서 호출

## 다음 단계

- [Repository 패턴](repository-pattern.md) - CRUD 작업
- [커스텀 쿼리](custom-queries.md) - Entity 매핑이 있는 복잡한 쿼리
