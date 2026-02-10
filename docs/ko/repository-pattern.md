# Repository 패턴

Repository 패턴은 Entity에 대한 데이터베이스 CRUD 작업을 위한 깔끔하고 타입 안전한 인터페이스를 제공합니다.

## 기본 사용법

```cpp
#include <pq/pq.hpp>

// User entity가 정의되고 등록되어 있다고 가정
pq::Connection conn("host=localhost dbname=mydb user=postgres");
pq::Repository<User, int> userRepo(conn);
```

## Repository 템플릿

```cpp
template<typename Entity, typename PK = int>
class Repository;
```

| 파라미터 | 설명 |
|----------|------|
| `Entity` | Entity 타입 (`PQ_REGISTER_ENTITY`로 등록되어야 함) |
| `PK` | 기본 키 타입 (기본값: `int`) |

복합 기본 키는 `PK`를 `std::tuple<...>`로 지정해서 사용합니다.

```cpp
using OrderItemKey = std::tuple<int, int>;  // (order_id, product_id)
pq::Repository<OrderItem, OrderItemKey> repo(conn);

// 튜플 형태
auto byTuple = repo.findById(std::make_tuple(1001, 42));

// 가변 인자 형태
auto byArgs = repo.findById(1001, 42);
```

## CRUD 작업

### 생성 (save)

```cpp
// 단일 Entity 저장
User user;
user.name = "John Doe";
user.email = "john@example.com";

auto result = userRepo.save(user);
if (result) {
    User& saved = *result;
    std::cout << "저장된 ID: " << saved.id << std::endl;
} else {
    std::cerr << "에러: " << result.error().message << std::endl;
}
```

```cpp
// 여러 Entity 저장
std::vector<User> users = {
    {0, "Alice", "alice@example.com"},
    {0, "Bob", "bob@example.com"},
    {0, "Charlie", "charlie@example.com"}
};

auto result = userRepo.saveAll(users);
if (result) {
    for (const auto& u : *result) {
        std::cout << "저장됨: " << u.name << " (ID: " << u.id << ")" << std::endl;
    }
}
```

### 조회 (find)

```cpp
// ID로 조회
auto result = userRepo.findById(1);
if (result) {
    if (*result) {
        // 찾음
        User& user = **result;
        std::cout << "찾음: " << user.name << std::endl;
    } else {
        // 찾지 못함 (에러는 아님)
        std::cout << "사용자를 찾을 수 없음" << std::endl;
    }
} else {
    // 데이터베이스 에러
    std::cerr << "에러: " << result.error().message << std::endl;
}
```

```cpp
// 전체 조회
auto result = userRepo.findAll();
if (result) {
    for (const auto& user : *result) {
        std::cout << user.id << ": " << user.name << std::endl;
    }
}
```

```cpp
// 존재 여부 확인
auto exists = userRepo.existsById(1);
if (exists && *exists) {
    std::cout << "사용자 존재함" << std::endl;
}
```

```cpp
// 전체 개수
auto count = userRepo.count();
if (count) {
    std::cout << "총 사용자 수: " << *count << std::endl;
}
```

### 수정 (update)

```cpp
auto findResult = userRepo.findById(1);
if (findResult && *findResult) {
    User& user = **findResult;
    user.name = "수정된 이름";
    user.email = "updated@example.com";
    
    auto updateResult = userRepo.update(user);
    if (updateResult) {
        std::cout << "수정 성공" << std::endl;
    }
}
```

### 삭제 (remove)

```cpp
// ID로 삭제
auto result = userRepo.removeById(1);
if (result) {
    std::cout << *result << "개 행 삭제됨" << std::endl;
}

// Entity로 삭제
auto result = userRepo.remove(user);
if (result) {
    std::cout << *result << "개 행 삭제됨" << std::endl;
}

// 여러 개 삭제
std::vector<User> usersToDelete = {...};
auto result = userRepo.removeAll(usersToDelete);
if (result) {
    std::cout << *result << "개 행 삭제됨" << std::endl;
}
```

## 커스텀 쿼리

기본 CRUD 외의 쿼리는 `executeQuery`와 `executeQueryOne`을 사용:

```cpp
// 이메일 도메인으로 사용자 찾기
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
// 이메일로 단일 사용자 찾기
auto result = userRepo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    {"john@example.com"}
);

if (result && *result) {
    User& user = **result;
    std::cout << "찾음: " << user.name << std::endl;
}
```

## API 레퍼런스

### 생성자

```cpp
Repository(Connection& conn, const MapperConfig& config = defaultMapperConfig())
```

| 파라미터 | 설명 |
|----------|------|
| `conn` | 데이터베이스 연결 (Repository보다 오래 유지되어야 함) |
| `config` | 선택적 매퍼 설정 |

### 메서드

| 메서드 | 반환 타입 | 설명 |
|--------|-----------|------|
| `save(entity)` | `DbResult<Entity>` | 새 Entity 저장, 생성된 ID 포함 반환 |
| `saveAll(entities)` | `DbResult<vector<Entity>>` | 여러 Entity 저장 |
| `findById(id)` | `DbResult<optional<Entity>>` | 기본 키 조회 (단일/튜플) |
| `findById(args...)` | `DbResult<optional<Entity>>` | 튜플/복합 PK용 가변 인자 조회 (컴파일 타임 개수/타입 검증) |
| `findAll()` | `DbResult<vector<Entity>>` | 전체 Entity 조회 |
| `update(entity)` | `DbResult<Entity>` | 기존 Entity 수정 |
| `remove(entity)` | `DbResult<int>` | 기본 키로 Entity 삭제 |
| `removeById(id)` | `DbResult<int>` | 기본 키 삭제 (단일/튜플) |
| `removeById(args...)` | `DbResult<int>` | 튜플/복합 PK용 가변 인자 삭제 (컴파일 타임 개수/타입 검증) |
| `removeAll(entities)` | `DbResult<int>` | 여러 Entity 삭제 |
| `count()` | `DbResult<int64_t>` | 전체 Entity 개수 |
| `existsById(id)` | `DbResult<bool>` | 기본 키 존재 여부 확인 (단일/튜플) |
| `existsById(args...)` | `DbResult<bool>` | 튜플/복합 PK용 가변 인자 존재 확인 (컴파일 타임 개수/타입 검증) |
| `executeQuery(sql, params)` | `DbResult<vector<Entity>>` | 커스텀 쿼리 실행 |
| `executeQueryOne(sql, params)` | `DbResult<optional<Entity>>` | 0-1개 결과 예상 쿼리 실행 |
| `connection()` | `Connection&` | 기본 연결 가져오기 |
| `config()` | `MapperConfig&` | 매퍼 설정 가져오기/수정 |

## 매퍼 설정

```cpp
pq::MapperConfig config;
config.strictColumnMapping = true;   // 매핑되지 않은 컬럼에 에러
config.ignoreExtraColumns = false;   // 결과의 추가 컬럼 무시 안 함
config.autoValidateSchema = true;    // Repository 첫 사용 시 스키마 검증
config.schemaValidationMode = pq::SchemaValidationMode::Strict; // Strict 또는 Lenient

pq::Repository<User, int> userRepo(conn, config);
```

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `strictColumnMapping` | `true` | 결과에 Entity에 매핑되지 않은 컬럼이 있으면 에러 |
| `ignoreExtraColumns` | `false` | `true`면 에러 대신 추가 컬럼 무시 |
| `autoValidateSchema` | `false` | `true`면 Repository 첫 작업 시 `SchemaValidator`를 1회 실행 |
| `schemaValidationMode` | `Strict` | 자동 검증 시 사용할 모드 (`Strict` / `Lenient`) |

## 생성되는 SQL

Repository는 표준 SQL 문을 생성합니다:

```cpp
// save()가 생성:
INSERT INTO users (name, email) VALUES ($1, $2) RETURNING *

// findById()가 생성:
SELECT * FROM users WHERE id = $1

// findAll()이 생성:
SELECT * FROM users

// update()가 생성:
UPDATE users SET name = $1, email = $2 WHERE id = $3 RETURNING *

// remove()가 생성:
DELETE FROM users WHERE id = $1

// 복합 PK 예시:
SELECT * FROM order_items WHERE order_id = $1 AND product_id = $2
UPDATE order_items SET quantity = $1 WHERE order_id = $2 AND product_id = $3 RETURNING *
DELETE FROM order_items WHERE order_id = $1 AND product_id = $2
```

## 에러 핸들링

모든 Repository 메서드는 성공 값 또는 에러를 담은 `DbResult<T>`를 반환합니다:

```cpp
auto result = userRepo.findById(1);

// 확인 및 접근
if (result) {
    // 성공 - result가 truthy
    auto& value = *result;  // 값 접근
} else {
    // 에러 - result가 falsy
    DbError& err = result.error();
    std::cerr << "에러: " << err.message << std::endl;
    std::cerr << "SQLSTATE: " << err.sqlState << std::endl;
}

// 또는 valueOr()로 기본값 사용
auto users = userRepo.findAll().valueOr(std::vector<User>{});
```

## 모범 사례

1. **Repository 수명을 Connection보다 짧게 유지**
   ```cpp
   pq::Connection conn(...);
   {
       pq::Repository<User, int> repo(conn);
       // repo 사용...
   }  // repo 소멸, conn은 여전히 유효
   ```

2. **여러 작업에는 트랜잭션 사용**
   ```cpp
   pq::Transaction tx(conn);
   userRepo.save(user1);
   userRepo.save(user2);
   tx.commit();
   ```

3. **값 접근 전 결과 확인**
   ```cpp
   auto result = userRepo.findById(id);
   if (result && *result) {  // Result와 optional 모두 확인
       // 안전하게 사용 가능
   }
   ```

4. **복잡한 쿼리에는 executeQuery 사용**
   ```cpp
   auto result = userRepo.executeQuery(
       "SELECT * FROM users WHERE created_at > $1 ORDER BY name LIMIT $2",
       {startDate, std::to_string(limit)}
   );
   ```

## 다음 단계

- [트랜잭션](transactions.md) - 트랜잭션 관리
- [에러 핸들링](error-handling.md) - 상세한 에러 처리
- [커스텀 쿼리](custom-queries.md) - 고급 쿼리 패턴
