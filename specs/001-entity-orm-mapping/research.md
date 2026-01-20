# Research: Entity ORM Mapping

**Feature**: 001-entity-orm-mapping  
**Date**: 2026-01-20

## R-001: RAII Wrapper for libpq Resources

### Decision
Custom Deleter를 사용한 `std::unique_ptr` 래퍼 (Rule of Zero 지향)

### Rationale
- libpq의 `PGconn*`과 `PGresult*`는 수동 해제 필요 (`PQfinish`, `PQclear`)
- Rule of Zero: 복잡한 소멸자/복사 생성자 없이 RAII 달성
- `std::unique_ptr`이 move semantics 자동 제공
- 예외 발생 시에도 리소스 누수 방지

### Alternatives Considered
1. **Raw pointer + manual delete**: RAII 원칙 위반, 예외 안전성 없음
2. **std::shared_ptr**: 불필요한 참조 카운팅 오버헤드
3. **Custom RAII class (Rule of Five)**: 보일러플레이트 코드 증가

### Best Practices
```cpp
struct PgConnDeleter {
    void operator()(PGconn* conn) const noexcept {
        if (conn) PQfinish(conn);
    }
};
using PgConnPtr = std::unique_ptr<PGconn, PgConnDeleter>;

// Factory function for clarity
[[nodiscard]] inline PgConnPtr makePgConn(const char* conninfo) noexcept {
    return PgConnPtr(PQconnectdb(conninfo));
}
```

---

## R-002: Error Handling Strategy

### Decision
`Result<T, E>` 타입 - std::variant 기반 구현

### Rationale
- `std::expected`는 C++23 전용 - C++17 환경에서 사용 불가
- 예외(exceptions)는 hot path에서 성능 오버헤드 발생
- 명시적 에러 처리 강제로 실수 방지
- Rust의 `Result` 타입과 유사한 ergonomics

### Alternatives Considered
1. **예외 기반**: 성능 오버헤드, 에러 무시 가능성
2. **errno + 반환값**: C 스타일, 타입 안전성 없음
3. **std::optional + 별도 에러 조회**: 에러 정보 손실 가능

### Best Practices
```cpp
template<typename T, typename E = DbError>
class Result {
    std::variant<T, E> data_;
public:
    [[nodiscard]] bool hasValue() const noexcept;
    [[nodiscard]] T& value() &;
    [[nodiscard]] E& error() &;
    
    // Monadic operations
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>, E>;
    
    template<typename U>
    T valueOr(U&& defaultValue) const&;
};
```

---

## R-003: Entity Mapping Mechanism

### Decision
Macro 기반 선언적 매핑

### Rationale
- 컴파일 타임에 테이블명/컬럼 정보 추출 가능
- JPA의 `@Entity`, `@Column` 어노테이션과 유사한 사용성
- 템플릿 메타프로그래밍보다 코드 가독성 우수
- 런타임 리플렉션 불필요

### Alternatives Considered
1. **템플릿 메타프로그래밍**: 복잡도 높음, 에러 메시지 난해
2. **런타임 리플렉션 (libclang 등)**: 빌드 복잡도 증가, 의존성 추가
3. **외부 코드 생성기**: 빌드 파이프라인 복잡화

### Best Practices
```cpp
struct User {
    int id;
    std::string name;
    std::optional<std::string> email;
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)  // EntityMeta<User> 특수화 생성
```

---

## R-004: string_view and Null Termination

### Decision
`NullTerminatedString` 헬퍼 클래스로 안전한 변환

### Rationale
- `std::string_view`는 성능 우수 (복사 없음)
- libpq C API는 null-terminated string 필수
- `string_view.data()`는 null-termination 보장 안함
- 필요 시에만 std::string으로 복사

### Alternatives Considered
1. **항상 std::string 사용**: 불필요한 복사 발생
2. **string_view.data() 직접 사용**: 미정의 동작 위험
3. **c_str() 호출 시점에 복사**: API 설계 일관성 저하

### Best Practices
```cpp
class NullTerminatedString {
    std::string storage_;  // string_view가 아닌 경우만 사용
    const char* ptr_;
public:
    explicit NullTerminatedString(std::string_view sv)
        : storage_(sv), ptr_(storage_.c_str()) {}
    
    explicit NullTerminatedString(const std::string& s)
        : ptr_(s.c_str()) {}  // storage_ 사용 안함
    
    [[nodiscard]] const char* c_str() const noexcept { return ptr_; }
};
```

---

## R-005: TypeTraits System Design

### Decision
템플릿 특수화 기반 TypeTraits

### Rationale
- 컴파일 타임 타입 매핑
- 각 타입별 PostgreSQL OID 연결
- toString/fromString 변환 함수 제공
- SQL Injection 방지 (타입별 적절한 처리)

### Alternatives Considered
1. **런타임 타입 맵**: 컴파일 타임 검증 불가
2. **if-else 체인**: 확장성 없음, 새 타입 추가 시 수정 필요
3. **std::any + 타입 체크**: 타입 안전성 저하

### Type Mapping Table

| C++ Type | PostgreSQL Type | OID |
|----------|-----------------|-----|
| bool | boolean | 16 |
| int16_t | smallint | 21 |
| int32_t / int | integer | 23 |
| int64_t | bigint | 20 |
| float | real | 700 |
| double | double precision | 701 |
| std::string | text | 25 |
| std::optional<T> | T (nullable) | T's OID |

---

## R-006: Repository findById() Inclusion

### Decision
`findById(PK id)`를 Repository 기본 메서드로 포함

### Rationale
- 가장 자주 사용되는 조회 패턴
- Raw Query 작성 부담 감소
- 사용자 피드백에서 명시적 요청
- Prepared Statement 캐싱으로 성능 최적화 가능

### Specification Alignment
- Spec에서는 "조건부 조회는 Raw Query"로 명시
- 피드백에서 findById()는 기본 기능으로 포함 요청
- **결정**: findById()만 기본 제공, 다른 조건 조회는 Raw Query 유지

### Implementation
```cpp
template<typename Entity, typename PK>
class Repository {
    DbResult<std::optional<Entity>> findById(const PK& id) {
        auto sql = sqlBuilder_.selectByIdSql();
        return executeQueryOne(sql, {std::to_string(id)});
    }
};
```

---

## R-007: FR-014 Strict Mapping with Flexibility

### Decision
`MapperConfig`로 런타임 설정 가능, 기본값은 엄격 모드

### Rationale
- **기본값 (엄격)**: 매핑되지 않은 컬럼 → 예외 발생
  - 의도하지 않은 데이터 무시 방지
  - 스키마 변경 감지
- **완화 옵션**: 레거시 시스템, 마이그레이션 중 필요
  - `ignoreExtraColumns = true`

### Implementation
```cpp
struct MapperConfig {
    bool strictColumnMapping = true;   // 기본: 엄격
    bool ignoreExtraColumns = false;   // true면 추가 컬럼 무시
};

// Usage
MapperConfig relaxed;
relaxed.ignoreExtraColumns = true;
Repository<User, int> repo(conn, relaxed);
```

---

## R-008: Connection Pool Design

### Decision
RAII 기반 Connection Pool with `PooledConnection` wrapper

### Rationale
- 연결 생성 오버헤드 최소화
- 자동 반환 (스코프 종료 시)
- 최대 연결 수 제한
- 유휴 연결 유효성 검사

### Implementation Sketch
```cpp
class ConnectionPool {
    std::vector<PgConnPtr> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
    
public:
    // RAII wrapper that returns connection to pool on destruction
    class PooledConnection {
        ConnectionPool* pool_;
        PgConnPtr conn_;
    public:
        ~PooledConnection() { pool_->release(std::move(conn_)); }
    };
    
    DbResult<PooledConnection> acquire(std::chrono::milliseconds timeout);
};
```

---

## Summary

| Research Item | Decision | Key Benefit |
|---------------|----------|-------------|
| R-001 | unique_ptr + Custom Deleter | Rule of Zero, RAII |
| R-002 | Result<T, E> | 명시적 에러 처리 |
| R-003 | Macro-based Entity | JPA-like ergonomics |
| R-004 | NullTerminatedString | string_view 성능 + C API 호환 |
| R-005 | TypeTraits template | 컴파일 타임 타입 안전성 |
| R-006 | findById() 포함 | 사용성 향상 |
| R-007 | MapperConfig | 엄격함 + 유연성 |
| R-008 | RAII Connection Pool | 성능 + 자동 관리 |
