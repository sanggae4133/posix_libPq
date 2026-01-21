# 에러 핸들링

PosixLibPq는 에러 핸들링을 위해 `Result<T, E>` 타입을 사용하며, 이는 Rust의 `Result`나 C++23의 `std::expected`와 유사한 타입 안전한 예외 대안입니다.

## Result 타입

```cpp
template<typename T, typename E = DbError>
class Result;

// 편의 별칭
template<typename T>
using DbResult = Result<T, DbError>;
```

모든 데이터베이스 작업은 `DbResult<T>`를 반환합니다:

```cpp
DbResult<QueryResult> execute(std::string_view sql);
DbResult<Entity> save(const Entity& entity);
DbResult<std::optional<Entity>> findById(const PK& id);
DbResult<void> commit();
```

## 기본 사용법

### 성공 확인

```cpp
auto result = conn.execute("SELECT * FROM users");

// 방법 1: Boolean 변환
if (result) {
    // 성공
    QueryResult& qr = *result;
}

// 방법 2: hasValue() / hasError()
if (result.hasValue()) {
    // 성공
}
if (result.hasError()) {
    // 실패
}
```

### 값 접근

```cpp
auto result = conn.execute("SELECT * FROM users");

if (result) {
    // 역참조로 값 가져오기
    QueryResult& value = *result;
    
    // 또는 value() 사용
    QueryResult& value2 = result.value();
    
    // 멤버 접근을 위한 화살표 연산자
    int rows = result->rowCount();
}
```

### 에러 접근

```cpp
auto result = conn.execute("INVALID SQL");

if (!result) {
    DbError& err = result.error();
    
    std::cerr << "메시지: " << err.message << std::endl;
    std::cerr << "SQLSTATE: " << err.sqlState << std::endl;
    std::cerr << "코드: " << err.errorCode << std::endl;
}
```

## DbError 구조체

```cpp
struct DbError {
    std::string message;    // 사람이 읽을 수 있는 에러 메시지
    std::string sqlState;   // PostgreSQL SQLSTATE 코드 (5자)
    int errorCode{0};       // 추가 에러 코드
    
    const char* what() const noexcept;  // message.c_str() 반환
};
```

### 자주 사용되는 SQLSTATE 코드

| SQLSTATE | 설명 |
|----------|------|
| `23505` | 유니크 제약 위반 |
| `23503` | 외래 키 제약 위반 |
| `23502` | NOT NULL 제약 위반 |
| `42P01` | 정의되지 않은 테이블 |
| `42703` | 정의되지 않은 컬럼 |
| `08001` | 연결 불가 |
| `08006` | 연결 실패 |
| `25P02` | 트랜잭션 중단됨 |

## valueOr 패턴

`valueOr()`로 기본값 제공:

```cpp
// 성공이면 값 반환, 에러면 기본값
auto users = userRepo.findAll().valueOr(std::vector<User>{});

int count = countResult.valueOr(0);
std::string name = nameResult.valueOr("Unknown");
```

## Map/Transform

에러를 유지하면서 성공 값 변환:

```cpp
auto result = userRepo.findById(1);

// map은 성공 값을 변환
auto nameResult = result.map([](const std::optional<User>& opt) {
    return opt ? opt->name : "찾을 수 없음";
});

if (nameResult) {
    std::cout << "이름: " << *nameResult << std::endl;
}

// map 체이닝
auto lengthResult = result
    .map([](const auto& opt) { return opt ? opt->name : ""; })
    .map([](const std::string& s) { return s.length(); });
```

## Result 생성하기

### 성공 Result

```cpp
// 암시적 생성
DbResult<int> success = 42;

// 팩토리 메서드
auto result = DbResult<int>::ok(42);

// Void result
auto voidResult = DbResult<void>::ok();
```

### 에러 Result

```cpp
// DbError로 팩토리 메서드
auto error = DbResult<int>::error(DbError{"문제가 발생했습니다", "00000", 1});

// 메시지만으로 간단히
auto error2 = DbResult<int>::error(DbError{"실패"});

// in-place 생성
Result<int, DbError> error3(inPlaceError, "에러 메시지", "42P01", 0);
```

## 패턴: 조기 반환

```cpp
DbResult<User> createUserWithProfile(const std::string& name) {
    // 각 단계가 실패할 수 있음
    auto userResult = userRepo.save(User{0, name});
    if (!userResult) {
        return DbResult<User>::error(userResult.error());
    }
    
    auto profileResult = profileRepo.save(Profile{0, userResult->id});
    if (!profileResult) {
        return DbResult<User>::error(profileResult.error());
    }
    
    return *userResult;
}
```

## 패턴: 에러 전파

```cpp
DbResult<void> complexOperation() {
    // 여러 단계 실행, 첫 에러 전파
    
    auto result1 = step1();
    if (!result1) return DbResult<void>::error(result1.error());
    
    auto result2 = step2(*result1);
    if (!result2) return DbResult<void>::error(result2.error());
    
    auto result3 = step3(*result2);
    if (!result3) return DbResult<void>::error(result3.error());
    
    return DbResult<void>::ok();
}
```

## 패턴: 에러 수집

```cpp
std::vector<DbError> errors;

for (const auto& user : users) {
    auto result = userRepo.save(user);
    if (!result) {
        errors.push_back(result.error());
    }
}

if (!errors.empty()) {
    // 수집된 에러 처리
}
```

## Void Result

값을 반환하지 않는 작업:

```cpp
DbResult<void> commit();
DbResult<void> connect(std::string_view connStr);

auto result = tx.commit();
if (result) {
    // 성공 (접근할 값 없음)
    std::cout << "커밋됨!" << std::endl;
} else {
    std::cerr << "실패: " << result.error().message << std::endl;
}
```

## 예외와의 통합

PosixLibPq는 `Result` 타입을 사용하지만, 필요시 예외로 변환 가능:

```cpp
// 에러 시 throw
auto result = userRepo.findById(id);
if (!result) {
    throw std::runtime_error(result.error().message);
}

// 또는 헬퍼 생성
template<typename T>
T unwrap(DbResult<T> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return std::move(*result);
}

// 사용
auto user = unwrap(userRepo.findById(id));
```

## 모범 사례

### 1. 항상 결과 확인

```cpp
// 나쁨: 결과 무시
conn.execute("INSERT INTO users (name) VALUES ('test')");

// 좋음: 결과 확인
auto result = conn.execute("INSERT INTO users (name) VALUES ('test')");
if (!result) {
    log_error(result.error().message);
}
```

### 2. 성공과 에러 모두 처리

```cpp
auto result = userRepo.findById(id);
if (result) {
    if (*result) {
        // 찾음
        processUser(**result);
    } else {
        // 찾지 못함 (에러는 아님)
        return NotFound();
    }
} else {
    // 데이터베이스 에러
    return ServerError(result.error().message);
}
```

### 3. 컨텍스트와 함께 에러 로깅

```cpp
auto result = userRepo.save(user);
if (!result) {
    const auto& err = result.error();
    logger.error("사용자 '{}' 저장 실패: {} (SQLSTATE: {})",
                 user.name, err.message, err.sqlState);
}
```

### 4. 적절한 에러 메시지 사용

```cpp
// 에러 메시지에 컨텍스트 추가
if (!result) {
    // 반환 전 컨텍스트 추가
    return DbResult<Order>::error(DbError{
        "사용자 " + std::to_string(userId) + "의 주문 생성 실패: " + result.error().message,
        result.error().sqlState,
        result.error().errorCode
    });
}
```

### 5. 확인 없이 value() 사용 금지

```cpp
// 위험: 에러 시 throw
auto value = result.value();  // throw할 수 있음!

// 안전: 먼저 확인
if (result) {
    auto value = result.value();  // 안전
}

// 또는 valueOr 사용
auto value = result.valueOr(defaultValue);
```

## API 레퍼런스

### Result<T, E> 메서드

| 메서드 | 반환 | 설명 |
|--------|------|------|
| `hasValue()` | `bool` | 값을 포함하면 true |
| `hasError()` | `bool` | 에러를 포함하면 true |
| `operator bool()` | `bool` | `hasValue()`와 동일 |
| `value()` | `T&` | 값 가져오기 (에러 시 throw) |
| `error()` | `E&` | 에러 가져오기 (값이면 throw) |
| `valueOr(default)` | `T` | 값 또는 기본값 |
| `operator*()` | `T&` | `value()`와 동일 |
| `operator->()` | `T*` | 값에 대한 포인터 |
| `map(f)` | `Result<U, E>` | 값 변환 |

### 정적 팩토리 메서드

| 메서드 | 설명 |
|--------|------|
| `Result::ok(value)` | 성공 result 생성 |
| `Result::error(err)` | 에러 result 생성 |

## 다음 단계

- [Repository 패턴](repository-pattern.md) - Repository 에러 핸들링
- [트랜잭션](transactions.md) - 트랜잭션 에러 핸들링
