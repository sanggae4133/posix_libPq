# Implementation Plan: Entity ORM Mapping

**Branch**: `001-entity-orm-mapping` | **Date**: 2026-01-20 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `/specs/001-entity-orm-mapping/spec.md`

## Summary

C++17 기반 PostgreSQL ORM 라이브러리 구현. libpq를 RAII 래퍼로 감싸고, 매크로 기반의 선언적 Entity 매핑을 제공하며, Repository 패턴을 통한 CUD 작업과 Raw Query 실행을 지원한다.

**핵심 기술 접근법**:
- Custom Deleter를 사용한 `std::unique_ptr` 기반 RAII 래퍼
- `PQ_ENTITY`, `PQ_COLUMN` 매크로를 통한 컴파일 타임 메타데이터 추출
- `Result<T, E>` 기반 에러 핸들링 (std::variant 활용)
- TypeTraits 시스템으로 C++ ↔ PostgreSQL OID 매핑

## Technical Context

**Language/Version**: C++17 (ISO/IEC 14882:2017) - C++20/23 기능 사용 금지  
**Primary Dependencies**: libpq 15.x (PQlibVersion >= 150007)  
**Storage**: PostgreSQL (libpq C API 직접 사용)  
**Testing**: Google Test 1.11+  
**Target Platform**: RedHat Enterprise Linux 8+ (POSIX compliant), GCC 8+/Clang 7+  
**Project Type**: Single library project  
**Performance Goals**: libpq 직접 사용 대비 5% 이내 오버헤드, 1만 건 조회 10초 이내  
**Constraints**: Zero-cost abstraction, RAII 필수, 예외 안전성 보장  
**Scale/Scope**: 단일 데이터베이스, 단일 PK, 관계 매핑 미지원 (v1.0)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Implementation |
|-----------|--------|----------------|
| **I. RAII-First** | ✅ PASS | `PgConnPtr`, `PgResultPtr` with custom deleters; Transaction auto-rollback |
| **II. SOLID Compliance** | ✅ PASS | Connection, Repository, Mapper 분리; 인터페이스 기반 설계 |
| **III. Type Safety First** | ✅ PASS | `std::optional` for NULL, TypeTraits system, Prepared statements |
| **IV. Zero-Cost Abstraction** | ✅ PASS | `std::string_view`, move semantics, Connection Pool |
| **V. JPA-Like Ergonomics** | ✅ PASS | Macro-based Entity, Repository pattern |
| **VI. POSIX/Platform** | ✅ PASS | C++17, POSIX API only, CMake build |
| **VII. Test-Driven** | ✅ PASS | Google Test, 80% coverage target |

**Gate Result**: ALL PASS - Proceed to Phase 0

## Project Structure

### Documentation (this feature)

```text
specs/001-entity-orm-mapping/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output (C++ interface headers)
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (repository root)

```text
posixLibPq/
├── include/pq/
│   ├── pq.hpp                    # Convenience header
│   ├── core/
│   │   ├── PqHandle.hpp          # RAII wrappers (PgConnPtr, PgResultPtr)
│   │   ├── Result.hpp            # Result<T, E> error handling
│   │   ├── Types.hpp             # TypeTraits system
│   │   ├── Connection.hpp        # Database connection
│   │   ├── QueryResult.hpp       # Query result + Row iteration
│   │   ├── Transaction.hpp       # RAII transaction
│   │   └── ConnectionPool.hpp    # Connection pooling
│   └── orm/
│       ├── Entity.hpp            # PQ_ENTITY, PQ_COLUMN macros
│       ├── Mapper.hpp            # Row → Entity mapping
│       └── Repository.hpp        # Repository<T, PK> template
├── src/
│   └── core/
│       ├── Connection.cpp
│       ├── Transaction.cpp
│       └── ConnectionPool.cpp
├── tests/
│   ├── unit/
│   │   ├── test_result.cpp
│   │   ├── test_types.cpp
│   │   ├── test_entity.cpp
│   │   └── test_repository.cpp
│   └── integration/
│       └── test_crud.cpp
├── examples/
│   └── usage_example.cpp
├── cmake/
│   └── pqConfig.cmake.in
├── CMakeLists.txt
└── README.md
```

**Structure Decision**: Single library project. Header-only ORM layer with minimal `.cpp` files for core connection handling.

## Phase 0: Research Decisions

### R-001: RAII Wrapper Strategy

**Decision**: Custom Deleter를 사용한 `std::unique_ptr` 래퍼

**Rationale**:
- Rule of Zero 지향 - 복잡한 소멸자/복사 생성자 불필요
- libpq의 `PQfinish()`, `PQclear()` 호출 자동화
- Move semantics 자연스럽게 지원

**Implementation**:
```cpp
struct PgConnDeleter {
    void operator()(PGconn* conn) const noexcept {
        if (conn) PQfinish(conn);
    }
};
using PgConnPtr = std::unique_ptr<PGconn, PgConnDeleter>;
```

### R-002: Error Handling Strategy

**Decision**: `Result<T, E>` 타입 (std::variant 기반)

**Rationale**:
- std::expected는 C++23 - 사용 불가
- 예외는 hot path에서 오버헤드 발생
- 명시적 에러 처리 강제

**Implementation**:
```cpp
template<typename T, typename E = DbError>
class Result {
    std::variant<T, E> data_;
public:
    bool hasValue() const;
    T& value();
    E& error();
    // Monadic operations: map(), valueOr()
};
```

### R-003: Entity Mapping Strategy

**Decision**: Macro 기반 선언적 매핑

**Rationale**:
- 컴파일 타임에 메타데이터 추출 가능
- JPA 어노테이션과 유사한 사용성
- 템플릿 메타프로그래밍보다 직관적

**Implementation**:
```cpp
struct User {
    int id;
    std::string name;
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name")
    PQ_ENTITY_END()
};
PQ_REGISTER_ENTITY(User)
```

### R-004: string_view + Null Termination

**Decision**: `NullTerminatedString` 헬퍼 클래스

**Rationale**:
- libpq는 null-terminated C string 요구
- std::string_view는 null-termination 보장 안함
- 성능을 위해 string_view 적극 활용하되, API 호출 전 변환

**Implementation**:
```cpp
class NullTerminatedString {
    std::string storage_;
    const char* ptr_;
public:
    explicit NullTerminatedString(std::string_view sv);
    const char* c_str() const noexcept;
};
```

### R-005: TypeTraits System

**Decision**: 특수화 기반 TypeTraits 템플릿

**Rationale**:
- 컴파일 타임 타입 매핑
- SQL Injection 방지 (타입별 적절한 이스케이핑)
- 확장 가능한 구조

**Implementation**:
```cpp
template<typename T>
struct PgTypeTraits {
    static constexpr Oid pgOid;
    static std::string toString(const T& value);
    static T fromString(const char* str);
};
```

### R-006: Repository Enhancement (findById)

**Decision**: Repository에 `findById()` 기본 포함

**Rationale**:
- 가장 자주 사용되는 조회 패턴
- Raw Query 작성 부담 감소
- Spec에서는 제외했으나 피드백에서 포함 요청

**Implementation**:
```cpp
template<typename Entity, typename PK>
class Repository {
    DbResult<std::optional<Entity>> findById(const PK& id);
    // + save, saveAll, findAll, update, remove, removeAll
};
```

### R-007: FR-014 엄격한 매핑 + 유연성

**Decision**: `MapperConfig`로 런타임 설정 가능

**Rationale**:
- 기본값: 엄격한 매핑 (매핑되지 않은 컬럼 → 예외)
- 옵션: `ignoreExtraColumns = true`로 완화 가능
- 마이그레이션 중 유연성 필요한 경우 대응

**Implementation**:
```cpp
struct MapperConfig {
    bool strictColumnMapping = true;
    bool ignoreExtraColumns = false;  // Override strict
};
```

## Phase 1: Design Artifacts

### Component Dependency Graph

```
┌─────────────────────────────────────────────────────────────┐
│                        pq::pq.hpp                            │
└─────────────────────────────────────────────────────────────┘
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│   pq::core::    │ │   pq::orm::     │ │   pq::util::    │
│   Connection    │ │   Repository    │ │   Types.hpp     │
│   Transaction   │ │   Entity        │ │   Result.hpp    │
│   QueryResult   │ │   Mapper        │ │   PqHandle.hpp  │
└─────────────────┘ └─────────────────┘ └─────────────────┘
          │                   │                   │
          └───────────────────┼───────────────────┘
                              ▼
                    ┌─────────────────┐
                    │     libpq       │
                    │   (PGconn,      │
                    │    PGresult)    │
                    └─────────────────┘
```

### Implementation Phases

| Phase | Component | Priority | Dependencies |
|-------|-----------|----------|--------------|
| **1** | PqHandle.hpp (RAII) | P1 | libpq |
| **2** | Result.hpp | P1 | - |
| **3** | Types.hpp (TypeTraits) | P1 | - |
| **4** | Connection.hpp/cpp | P1 | PqHandle, Result |
| **5** | QueryResult.hpp | P1 | PqHandle, Types |
| **6** | Transaction.hpp/cpp | P4 | Connection |
| **7** | Entity.hpp (Macros) | P1 | Types |
| **8** | Mapper.hpp | P1 | Entity, QueryResult |
| **9** | Repository.hpp | P2 | Entity, Mapper, Connection |
| **10** | ConnectionPool.hpp | P5 | Connection |

### API Contract Summary

| Component | Key APIs |
|-----------|----------|
| **Connection** | `connect()`, `execute()`, `executeParams()`, `prepare()`, `executePrepared()` |
| **Transaction** | `Transaction(conn)`, `commit()`, `rollback()`, RAII auto-rollback |
| **Repository\<T,PK\>** | `save()`, `saveAll()`, `findById()`, `findAll()`, `update()`, `remove()`, `removeAll()`, `executeQuery()` |
| **Entity Macros** | `PQ_ENTITY()`, `PQ_COLUMN()`, `PQ_ENTITY_END()`, `PQ_REGISTER_ENTITY()` |

## Complexity Tracking

> No Constitution violations requiring justification.

| Decision | Justification |
|----------|---------------|
| findById() 추가 | Spec에서 제외했으나 피드백에서 필수 기능으로 요청. Raw Query 의존성 감소 |
| FR-014 유연성 | 엄격한 기본값 유지하되, 마이그레이션 등 특수 상황 대응을 위해 옵션 제공 |

## Generated Artifacts

- [research.md](./research.md) - Phase 0 연구 결과
- [data-model.md](./data-model.md) - 데이터 모델 정의
- [quickstart.md](./quickstart.md) - 빠른 시작 가이드
- [contracts/](./contracts/) - C++ 인터페이스 정의

## Next Steps

1. `/speckit.tasks` 명령으로 구체적인 Task 분해
2. 각 Phase별 구현 진행
3. 단위 테스트 작성 (TDD)
4. 통합 테스트 및 성능 검증
