<!--
=============================================================================
SYNC IMPACT REPORT
=============================================================================
Version Change: N/A → 1.0.0 (Initial)
Modified Principles: N/A (New constitution)

Added Sections:
  - Core Principles (7 principles)
    - I. RAII-First
    - II. SOLID Compliance
    - III. Type Safety First
    - IV. Zero-Cost Abstraction
    - V. JPA-Like Ergonomics
    - VI. POSIX/Platform Compatibility
    - VII. Test-Driven Development
  - Technical Constraints
  - Development Workflow
  - Governance

Removed Sections: N/A

Templates Status:
  - .specify/templates/plan-template.md ✅ Compatible (no changes needed)
  - .specify/templates/spec-template.md ✅ Compatible (no changes needed)
  - .specify/templates/tasks-template.md ✅ Compatible (no changes needed)
  - .specify/templates/commands/*.md - Directory does not exist (N/A)

Deferred Items: None
=============================================================================
-->

# PosixLibPq Constitution

C++17 기반 PostgreSQL ORM 라이브러리 프로젝트의 핵심 원칙과 거버넌스 규정

## Core Principles

### I. RAII-First (Resource Acquisition Is Initialization)

모든 리소스는 RAII 패턴을 통해 관리되어야 한다.

- 모든 PostgreSQL 연결(PGconn), 결과(PGresult), 준비된 문장은 RAII 래퍼로 감싸야 한다
- 소유권이 있는 원시 포인터(raw pointer) 사용을 금지한다; `std::unique_ptr`, `std::shared_ptr` 사용 필수
- 생성자에서 리소스 획득, 소멸자에서 리소스 해제를 보장해야 한다
- 예외 발생 시에도 리소스 누수가 없어야 한다 (exception-safe)
- 복사/이동 의미론을 명확히 정의해야 한다 (Rule of Five 또는 Rule of Zero)

**근거**: libpq의 C API는 수동 메모리 관리를 요구하며, RAII를 통해 메모리 누수와 dangling pointer를 방지한다.

### II. SOLID Compliance

SOLID 원칙을 엄격히 준수해야 한다.

- **Single Responsibility**: 각 클래스는 하나의 책임만 가진다 (예: Connection, Query, Result 분리)
- **Open/Closed**: 확장에는 열려있고, 수정에는 닫혀있어야 한다 (인터페이스 기반 설계)
- **Liskov Substitution**: 파생 클래스는 기반 클래스를 완전히 대체할 수 있어야 한다
- **Interface Segregation**: 클라이언트가 사용하지 않는 인터페이스에 의존하지 않도록 인터페이스를 분리한다
- **Dependency Inversion**: 고수준 모듈은 저수준 모듈에 의존하지 않고, 추상화에 의존한다

**근거**: 유지보수성과 테스트 용이성을 극대화하며, 라이브러리 확장 시 기존 코드 수정을 최소화한다.

### III. Type Safety First

C++17의 타입 안전 기능을 최대한 활용해야 한다.

- `std::optional<T>`을 사용하여 NULL 가능한 컬럼 값을 표현한다
- `std::variant<T...>`를 사용하여 다형적 반환 타입을 표현한다
- 문자열 대신 강타입(strong types)을 사용한다 (예: `ColumnName`, `TableName` 등의 타입 래퍼)
- 컴파일 타임 검증이 가능한 곳에서는 `constexpr`과 `static_assert`를 활용한다
- SQL 인젝션 방지를 위해 prepared statement를 강제한다

**근거**: 런타임 에러를 컴파일 타임에 잡아내어 안전성을 높이고, SQL 인젝션 공격을 원천 차단한다.

### IV. Zero-Cost Abstraction

성능 병목을 최소화하면서 깔끔한 추상화를 제공해야 한다.

- Connection Pool을 구현하여 연결 오버헤드를 최소화한다
- Prepared Statement 캐싱을 통해 쿼리 파싱 오버헤드를 줄인다
- 불필요한 문자열 복사를 피하기 위해 `std::string_view`를 적극 활용한다
- move semantics를 활용하여 복사 비용을 최소화한다
- 핫 패스(hot path)에서는 예외 대신 `std::expected` 또는 에러 코드를 고려한다

**근거**: JPA와 같은 ORM 추상화는 성능 비용이 발생할 수 있으나, C++에서는 zero-cost abstraction이 가능해야 한다.

### V. JPA-Like Ergonomics

Java JPA와 유사한 직관적인 ORM API를 제공해야 한다.

- Entity 정의를 위한 매크로 또는 템플릿 메타프로그래밍을 제공한다
- CRUD 작업을 위한 Repository 패턴을 구현한다
- 체이닝 가능한 Query Builder를 제공한다
- Transaction 관리를 위한 명확한 API를 제공한다
- 관계(1:1, 1:N, N:M) 매핑을 지원한다

**근거**: 사용자가 익숙한 JPA 패턴을 C++에서 사용할 수 있게 하여 학습 곡선을 낮춘다.

### VI. POSIX/Platform Compatibility

RedHat POSIX 환경에서의 완전한 호환성을 보장해야 한다.

- POSIX API만 사용하고, 플랫폼 특화 확장은 피한다
- GCC/Clang에서 모두 컴파일 가능해야 한다
- libpq 15.x (버전 150007) 이상을 지원한다
- C++17 표준을 엄격히 준수한다
- CMake를 빌드 시스템으로 사용하여 이식성을 보장한다

**근거**: RedHat Enterprise Linux 환경에서 안정적으로 작동해야 하며, 다양한 POSIX 호환 시스템으로 이식 가능해야 한다.

### VII. Test-Driven Development

테스트 주도 개발을 통해 품질을 보장해야 한다.

- 모든 public API는 단위 테스트가 있어야 한다
- 통합 테스트는 실제 PostgreSQL 인스턴스와 함께 수행한다
- 코드 커버리지 80% 이상을 목표로 한다
- Google Test (gtest)를 테스트 프레임워크로 사용한다
- Mock 객체를 통해 libpq 의존성을 격리하여 테스트한다

**근거**: ORM 라이브러리는 데이터 무결성에 직접적인 영향을 미치므로, 철저한 테스트가 필수적이다.

## Technical Constraints

### Environment & Dependencies

| 항목 | 요구사항 |
|------|----------|
| **C++ Standard** | C++17 (ISO/IEC 14882:2017) |
| **Target Platform** | RedHat Enterprise Linux 8+ (POSIX compliant) |
| **Compiler** | GCC 8+ 또는 Clang 7+ |
| **PostgreSQL** | libpq 15.x (PQlibVersion() >= 150007) |
| **Build System** | CMake 3.16+ |
| **Test Framework** | Google Test 1.11+ |

### Code Style & Standards

- 헤더 파일: `.hpp` 확장자, 소스 파일: `.cpp` 확장자
- 네임스페이스: `pq::` 접두사 사용 (예: `pq::Connection`, `pq::Entity`)
- 명명 규칙: 클래스는 PascalCase, 함수/변수는 camelCase, 상수는 UPPER_SNAKE_CASE
- 모든 public API는 Doxygen 주석 필수
- `#pragma once`를 헤더 가드로 사용

### Directory Structure

```
posixLibPq/
├── include/pq/          # Public headers
│   ├── core/            # Core abstractions (Connection, Result, etc.)
│   ├── orm/             # ORM layer (Entity, Repository, etc.)
│   └── util/            # Utilities (StringView, TypeTraits, etc.)
├── src/                 # Implementation files
│   ├── core/
│   ├── orm/
│   └── util/
├── tests/
│   ├── unit/            # Unit tests
│   ├── integration/     # Integration tests (require DB)
│   └── mock/            # Mock implementations
├── examples/            # Usage examples
├── docs/                # Documentation
└── cmake/               # CMake modules
```

## Development Workflow

### Code Review Requirements

- 모든 변경사항은 Pull Request를 통해 제출한다
- 최소 1명의 리뷰어 승인 필요
- CI 파이프라인 통과 필수 (빌드, 테스트, 정적 분석)
- RAII 및 SOLID 원칙 준수 여부 검토

### Quality Gates

1. **컴파일 게이트**: GCC와 Clang 모두에서 경고 없이 컴파일
2. **테스트 게이트**: 모든 단위 테스트 통과
3. **커버리지 게이트**: 신규 코드 80% 이상 커버리지
4. **정적 분석 게이트**: clang-tidy, cppcheck 경고 없음
5. **메모리 게이트**: Valgrind/AddressSanitizer 오류 없음

### Versioning Policy

Semantic Versioning (MAJOR.MINOR.PATCH)을 따른다:

- **MAJOR**: API 호환성을 깨는 변경
- **MINOR**: 하위 호환되는 기능 추가
- **PATCH**: 하위 호환되는 버그 수정

## Governance

### Amendment Procedure

1. 변경 제안서 작성 및 제출
2. 프로젝트 메인테이너 검토
3. 30일간 의견 수렴 기간
4. 과반수 동의 시 승인
5. 마이그레이션 계획 수립 후 적용

### Compliance Review

- 모든 PR은 Constitution 원칙 준수 여부를 검증해야 한다
- 복잡도 증가는 명시적 정당화가 필요하다
- RAII 위반, 원시 포인터 소유권, 타입 안전성 위반은 거부 사유가 된다

### Guidance Reference

런타임 개발 가이드는 `docs/development-guide.md`를 참조한다.

**Version**: 1.0.0 | **Ratified**: 2026-01-20 | **Last Amended**: 2026-01-20
