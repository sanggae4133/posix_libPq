# Feature Specification: ORM Enhancements (Composite PK, Schema Validation, Extended Types)

**Feature Branch**: `002-orm-enhancements`  
**Created**: 2026-01-22  
**Status**: Draft  
**Input**: User description: "복합 pk 기능이랑 스키마 검증 기능(빡세게) 그리고 지원 타입 추가하자"

## Implementation Tracking

- Latest implementation status snapshot (2026-02-10): `docs/spec-compliance-2026-02.md`

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Composite Primary Key Support (Priority: P1)

개발자가 복합 기본 키(composite primary key)를 가진 테이블을 Entity로 정의하고, `findById`, `update`, `remove` 등의 Repository 메서드를 사용하여 CRUD 작업을 수행할 수 있다.

**Why this priority**: 복합 PK는 many-to-many 관계 테이블이나 복잡한 도메인 모델에서 필수적이다. 현재 단일 PK만 지원하여 사용 범위가 제한적임.

**Independent Test**: `order_items(order_id, product_id)` 같은 복합 PK 테이블에 대해 Entity 정의 후 CRUD 작업이 정상 동작하는지 테스트.

**Acceptance Scenarios**:

1. **Given** Entity에 두 개 이상의 컬럼이 `PQ_PRIMARY_KEY`로 지정됨, **When** `findById(orderId, productId)` 호출, **Then** 해당 복합 키에 맞는 레코드가 반환됨
2. **Given** 복합 PK Entity 인스턴스가 존재, **When** `update(entity)` 호출, **Then** WHERE 절에 모든 PK 컬럼이 포함되어 업데이트됨
3. **Given** 복합 PK Entity 정의, **When** `SqlBuilder::selectByIdSql()` 호출, **Then** `WHERE pk1 = $1 AND pk2 = $2` 형태의 SQL 생성
4. **Given** 복합 PK가 없는 Entity, **When** 기존처럼 단일 PK 사용, **Then** 기존 동작과 동일하게 작동 (하위 호환성)

---

### User Story 2 - Strict Schema Validation (Priority: P1)

개발자가 애플리케이션 시작 시 Entity 정의와 실제 데이터베이스 스키마 간의 불일치를 감지하고, 명확한 에러 메시지를 받아 문제를 빠르게 해결할 수 있다.

**Why this priority**: 스키마 불일치로 인한 런타임 에러는 디버깅이 어렵고 프로덕션에서 장애를 유발할 수 있음. 조기 검증으로 개발 생산성 향상.

**Independent Test**: 의도적으로 Entity와 DB 스키마를 불일치시킨 후 검증 실행하여 에러가 정확히 보고되는지 확인.

**Acceptance Scenarios**:

1. **Given** Entity 컬럼 타입이 DB 컬럼 타입과 다름 (예: Entity는 int64, DB는 varchar), **When** 스키마 검증 실행, **Then** 타입 불일치 에러와 해당 컬럼 정보가 보고됨
2. **Given** Entity에 정의된 컬럼이 DB 테이블에 존재하지 않음, **When** 스키마 검증 실행, **Then** 누락된 컬럼 에러가 보고됨
3. **Given** DB 컬럼이 NOT NULL인데 Entity에서 nullable로 정의됨, **When** 스키마 검증 실행, **Then** nullable 불일치 경고/에러가 보고됨
4. **Given** Entity 테이블 이름이 DB에 존재하지 않음, **When** 스키마 검증 실행, **Then** 테이블 미존재 에러가 보고됨
5. **Given** DB에 extra 컬럼이 있지만 Entity에 정의되지 않음, **When** strict 모드로 검증, **Then** 매핑되지 않은 컬럼 경고가 보고됨
6. **Given** varchar(n) 길이 제한이 있는 컬럼, **When** 스키마 검증 실행, **Then** 길이 제한 정보가 메타데이터에 포함됨

---

### User Story 3 - Extended PostgreSQL Type Support (Priority: P2)

개발자가 `numeric`, `timestamp`, `timestamptz`, `date`, `time`, `uuid`, `jsonb` 등 추가 PostgreSQL 타입을 C++ Entity에서 사용할 수 있다.

**Why this priority**: 실무에서 자주 사용되는 타입들이며, 현재는 `std::string`으로 우회해야 하는 불편함이 있음.

**Independent Test**: 각 새로운 타입에 대해 INSERT/SELECT/UPDATE 라운드트립 테스트 수행.

**Acceptance Scenarios**:

1. **Given** Entity에 `timestamp` 타입 필드, **When** DB에서 조회, **Then** C++ `std::chrono::system_clock::time_point`로 변환됨
2. **Given** Entity에 `timestamptz` 타입 필드, **When** DB에서 조회, **Then** 타임존 정보가 포함된 시간으로 변환됨
3. **Given** Entity에 `numeric(10,2)` 타입 필드, **When** DB에서 조회, **Then** 정밀도 손실 없이 `std::string` 또는 전용 Decimal 타입으로 변환됨
4. **Given** Entity에 `uuid` 타입 필드, **When** INSERT 후 SELECT, **Then** UUID 문자열이 정확히 보존됨
5. **Given** Entity에 `date` 타입 필드, **When** DB에서 조회, **Then** 날짜 전용 타입으로 변환됨
6. **Given** Entity에 `jsonb` 타입 필드, **When** DB에서 조회, **Then** JSON 문자열로 변환되어 파싱 가능

---

### Edge Cases

- 복합 PK에서 하나의 컬럼만 값이 NULL인 경우 어떻게 처리되는가?
- 스키마 검증 시 DB 연결이 끊어진 경우 적절한 에러 메시지가 반환되는가?
- timestamp 타입에서 극단적인 날짜 (1970년 이전, 9999년 이후) 처리
- numeric 타입의 매우 큰 숫자나 매우 작은 소수점 처리
- UUID가 잘못된 포맷인 경우 에러 처리
- jsonb에 매우 큰 JSON 객체가 저장된 경우 성능

## Requirements *(mandatory)*

### Functional Requirements

#### Composite Primary Key

- **FR-001**: System MUST support multiple columns marked as `PQ_PRIMARY_KEY` in a single Entity
- **FR-002**: System MUST store composite PK indices as a list in `EntityMetadata`
- **FR-003**: `SqlBuilder` MUST generate WHERE clauses with AND for all PK columns
- **FR-004**: `Repository::findById` MUST use variadic template `findById(Args&&... args)` with compile-time validation via `static_assert` for PK count and type matching
- **FR-005**: System MUST maintain backward compatibility with single PK entities
- **FR-006**: System MUST throw clear error if composite PK entity tries to use single-key methods incorrectly

#### Schema Validation

- **FR-007**: System MUST provide `SchemaValidator` class with explicit `validate<Entity>(conn)` method for manual invocation
- **FR-007a**: System MUST provide optional auto-validation flag in Repository/Connection config to enable validation on first use
- **FR-008**: System MUST query PostgreSQL `information_schema` to retrieve actual column metadata
- **FR-009**: System MUST validate: table existence, column existence, type compatibility, nullable constraints
- **FR-010**: System MUST report all validation errors at once (not fail on first error)
- **FR-011**: System MUST provide clear error messages including: Entity name, column name, expected vs actual values
- **FR-012**: System MUST support validation modes: strict (error on any mismatch), lenient (warn only)
- **FR-013**: System MUST validate varchar/char length constraints and report in metadata

#### Extended Type Support

- **FR-014**: System MUST add `PgTypeTraits` specialization for `timestamp` → `std::chrono::system_clock::time_point`
- **FR-015**: System MUST add `PgTypeTraits` specialization for `timestamptz` with timezone handling
- **FR-016**: System MUST add `PgTypeTraits` specialization for `date` → appropriate date type
- **FR-017**: System MUST add `PgTypeTraits` specialization for `time` → appropriate time type
- **FR-018**: System MUST add `PgTypeTraits` specialization for `numeric` → `std::string` (precision preservation)
- **FR-019**: System MUST add `PgTypeTraits` specialization for `uuid` → `std::string`
- **FR-020**: System MUST add `PgTypeTraits` specialization for `jsonb` → `std::string`
- **FR-021**: All new type conversions MUST handle NULL values correctly via `std::optional`

### Key Entities

- **EntityMetadata**: 기존 단일 `primaryKeyIndex_`를 `std::vector<std::size_t> primaryKeyIndices_`로 확장
- **SchemaValidator**: DB 연결을 받아 Entity와 스키마 비교 수행, `ValidationResult` 반환
- **ValidationResult**: 검증 결과 (isValid, errors, warnings 포함)
- **ValidationError**: 에러 타입 (TableNotFound, ColumnNotFound, TypeMismatch, NullableMismatch 등)

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 복합 PK를 가진 Entity에서 CRUD 작업이 100% 정상 동작
- **SC-002**: 단일 PK Entity의 기존 테스트가 모두 통과 (하위 호환성 100%)
- **SC-003**: 스키마 검증 시 의도적 불일치 10개 케이스 모두 정확히 감지
- **SC-004**: 스키마 검증 에러 메시지에 Entity명, 컬럼명, 기대값, 실제값이 모두 포함
- **SC-005**: 새로운 7개 타입 (timestamp, timestamptz, date, time, numeric, uuid, jsonb) 모두 INSERT/SELECT 라운드트립 통과
- **SC-006**: timestamp/timestamptz 변환 시 밀리초 정밀도 보존
- **SC-007**: numeric 타입에서 소수점 15자리 이상 정밀도 보존
- **SC-008**: 모든 새 기능에 대한 단위 테스트 커버리지 90% 이상

## Clarifications

### Session 2026-01-22

- Q: 스키마 검증은 어떤 시점에 실행되어야 하나요? → A: 둘 다 - 수동 호출 API 제공 + 옵션으로 자동 검증 활성화 가능
- Q: 복합 PK `findById` API 설계는? → A: 가변 인자 템플릿 + 컴파일 타임 검증 (static_assert로 PK 개수/타입 불일치 감지)

## Assumptions

- C++17 이상 환경 (기존과 동일)
- PostgreSQL 10 이상 버전의 `information_schema` 사용 가능
- 타임존 처리는 서버 로컬 타임존 기준 (UTC 강제 변환 옵션 추후 추가 가능)
- numeric 타입은 `std::string`으로 처리하여 정밀도 보존 (별도 Decimal 라이브러리 의존성 없음)
- jsonb 파싱은 사용자 책임 (라이브러리는 문자열로만 제공)
