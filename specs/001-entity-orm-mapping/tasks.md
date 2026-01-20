# Tasks: Entity ORM Mapping

**Input**: Design documents from `/specs/001-entity-orm-mapping/`  
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/ ✅, quickstart.md ✅

**Tests**: Included (TDD approach per Constitution VII, SC-007: 80% coverage target)

**Organization**: Tasks are grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3, US4, US5)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Project Infrastructure)

**Purpose**: Project initialization, CMake configuration, and basic structure

- [x] T001 Create project directory structure per plan.md (include/pq/core/, include/pq/orm/, src/core/, tests/unit/, tests/integration/, examples/)
- [x] T002 Initialize CMakeLists.txt with C++17 standard, libpq dependency, and Google Test integration at CMakeLists.txt
- [x] T003 [P] Create cmake/pqConfig.cmake.in for library installation
- [x] T004 [P] Create tests/CMakeLists.txt with Google Test configuration
- [x] T005 [P] Create README.md with build instructions and library overview

**Checkpoint**: ✅ Project compiles with empty library target

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [x] T006 [P] Implement PgConnDeleter struct for PGconn cleanup in include/pq/core/PqHandle.hpp
- [x] T007 [P] Implement PgResultDeleter struct for PGresult cleanup in include/pq/core/PqHandle.hpp
- [x] T008 [P] Implement PgConnPtr and PgResultPtr type aliases with custom deleters in include/pq/core/PqHandle.hpp
- [x] T009 [P] Implement makePgConn() and makePgResult() factory functions in include/pq/core/PqHandle.hpp
- [x] T010 [P] Implement isConnected() and isSuccess() helper functions in include/pq/core/PqHandle.hpp
- [x] T011 [P] Implement DbError struct with message, sqlState, errorCode in include/pq/core/Result.hpp
- [x] T012 [P] Implement Result<T, E> class with std::variant storage in include/pq/core/Result.hpp
- [x] T013 [P] Implement Result<void, E> specialization in include/pq/core/Result.hpp
- [x] T014 [P] Implement DbResult<T> type alias in include/pq/core/Result.hpp
- [x] T015 [P] Implement PgTypeTraits<int> specialization (OID 23) in include/pq/core/Types.hpp
- [x] T016 [P] Implement PgTypeTraits<std::string> specialization (OID 25) in include/pq/core/Types.hpp
- [x] T017 [P] Implement PgTypeTraits<bool> specialization (OID 16) in include/pq/core/Types.hpp
- [x] T018 [P] Implement PgTypeTraits<int64_t> specialization (OID 20) in include/pq/core/Types.hpp
- [x] T019 [P] Implement PgTypeTraits<double> specialization (OID 701) in include/pq/core/Types.hpp
- [x] T020 [P] Implement PgTypeTraits<std::optional<T>> specialization in include/pq/core/Types.hpp
- [x] T021 [P] Implement NullTerminatedString helper class in include/pq/core/Types.hpp
- [x] T022 [P] Implement PostgreSQL OID constants namespace in include/pq/core/Types.hpp
- [x] T023 Unit test for Result<T, E> success/error states in tests/unit/test_result.cpp
- [x] T024 Unit test for PgTypeTraits toString/fromString in tests/unit/test_types.cpp

**Checkpoint**: ✅ Foundation ready - user story implementation can now begin

---

## Phase 3: User Story 1 - Entity 정의 및 테이블 매핑 (Priority: P1) 🎯 MVP

**Goal**: 개발자가 C++ 클래스를 PostgreSQL 테이블에 매핑하는 Entity로 정의할 수 있다

**Independent Test**: 클래스에 PQ_ENTITY, PQ_COLUMN 매크로를 적용하고 메타데이터가 올바르게 생성되는지 검증

### Tests for User Story 1 ✅

> **NOTE: Tests written after implementation in this case**

- [x] T025 [P] [US1] Unit test for ColumnFlags enum bitwise operations in tests/unit/test_entity.cpp
- [x] T026 [P] [US1] Unit test for ColumnInfo metadata extraction in tests/unit/test_entity.cpp
- [x] T027 [P] [US1] Unit test for EntityMetadata table name and columns in tests/unit/test_entity.cpp
- [x] T028 [P] [US1] Unit test for PQ_ENTITY macro table registration in tests/unit/test_entity.cpp
- [x] T029 [P] [US1] Unit test for PQ_COLUMN macro with PQ_PRIMARY_KEY flag in tests/unit/test_entity.cpp
- [x] T030 [P] [US1] Unit test for PQ_REGISTER_ENTITY metadata access in tests/unit/test_entity.cpp

### Implementation for User Story 1

- [x] T031 [P] [US1] Implement ColumnFlags enum with PrimaryKey, AutoIncrement, NotNull, Unique in include/pq/orm/Entity.hpp
- [x] T032 [P] [US1] Implement ColumnInfo struct with fieldName, columnName, pgType, flags in include/pq/orm/Entity.hpp
- [x] T033 [US1] Implement ColumnDescriptor<Entity> template with member pointer in include/pq/orm/Entity.hpp
- [x] T034 [US1] Implement EntityMetadata<Entity> template class in include/pq/orm/Entity.hpp
- [x] T035 [US1] Implement EntityMeta<T> trait struct with tableName and metadata() in include/pq/orm/Entity.hpp
- [x] T036 [US1] Implement PQ_ENTITY(ClassName, TableName) macro in include/pq/orm/Entity.hpp
- [x] T037 [US1] Implement PQ_COLUMN(Member, ColumnName, Flags) macro in include/pq/orm/Entity.hpp
- [x] T038 [US1] Implement PQ_ENTITY_END() macro in include/pq/orm/Entity.hpp
- [x] T039 [US1] Implement PQ_REGISTER_ENTITY(ClassName) macro for EntityMeta specialization in include/pq/orm/Entity.hpp
- [x] T040 [P] [US1] Implement Row class with get<T>(), isNull(), columnName() in include/pq/core/QueryResult.hpp
- [x] T041 [P] [US1] Implement RowIterator for range-based for loop support in include/pq/core/QueryResult.hpp
- [x] T042 [US1] Implement QueryResult class with rowCount(), columnCount(), iteration in include/pq/core/QueryResult.hpp
- [x] T043 [US1] Implement MapperConfig struct with strictColumnMapping, ignoreExtraColumns in include/pq/orm/Mapper.hpp
- [x] T044 [US1] Implement EntityMapper<Entity> class with mapRow(), mapAll(), mapOne() in include/pq/orm/Mapper.hpp
- [x] T045 [US1] Implement strict mapping validation (FR-014) in EntityMapper in include/pq/orm/Mapper.hpp
- [x] T046 [US1] Implement NULL to non-optional error handling in EntityMapper in include/pq/orm/Mapper.hpp

**Checkpoint**: ✅ COMPLETE - Entity 매크로로 클래스 정의 후 메타데이터 접근 가능. 쿼리 결과를 Entity로 매핑 가능.

---

## Phase 4: User Story 2 - Repository를 통한 CUD 작업 (Priority: P2)

**Goal**: Repository 패턴을 사용하여 Entity에 대한 save, saveAll, findAll, update, remove, removeAll, findById 작업 수행

**Independent Test**: Repository로 Entity CRUD 작업 후 데이터베이스 반영 확인

### Tests for User Story 2 ⚠️

- [ ] T047 [P] [US2] Unit test for ConnectionConfig toConnectionString() in tests/unit/test_connection.cpp
- [ ] T048 [P] [US2] Unit test for SqlBuilder INSERT generation in tests/unit/test_repository.cpp
- [ ] T049 [P] [US2] Unit test for SqlBuilder UPDATE generation in tests/unit/test_repository.cpp
- [ ] T050 [P] [US2] Unit test for SqlBuilder DELETE generation in tests/unit/test_repository.cpp
- [ ] T051 [P] [US2] Unit test for SqlBuilder SELECT all generation in tests/unit/test_repository.cpp
- [ ] T052 [P] [US2] Unit test for SqlBuilder SELECT by ID generation in tests/unit/test_repository.cpp

### Implementation for User Story 2

- [x] T053 [P] [US2] Implement ConnectionConfig struct with host, port, database, user, password in include/pq/core/Connection.hpp
- [x] T054 [P] [US2] Implement ConnectionConfig::toConnectionString() in include/pq/core/Connection.hpp
- [x] T055 [US2] Implement Connection class constructor with connection string in include/pq/core/Connection.hpp
- [x] T056 [US2] Implement Connection::connect() and disconnect() in src/core/Connection.cpp
- [x] T057 [US2] Implement Connection::isConnected() and status() in src/core/Connection.cpp
- [x] T058 [US2] Implement Connection::execute(sql) for simple queries in src/core/Connection.cpp
- [x] T059 [US2] Implement Connection::execute(sql, params) for parameterized queries in src/core/Connection.cpp
- [x] T060 [US2] Implement Connection::executeUpdate() returning affected rows in src/core/Connection.cpp
- [x] T061 [US2] Implement Connection::escapeString() and escapeIdentifier() in src/core/Connection.cpp
- [x] T062 [US2] Implement Connection::makeError() for DbError creation in src/core/Connection.cpp
- [x] T063 [P] [US2] Implement SqlBuilder<Entity> template for SQL generation in include/pq/orm/Repository.hpp
- [x] T064 [US2] Implement SqlBuilder::insertSql() for INSERT statement in include/pq/orm/Repository.hpp
- [x] T065 [US2] Implement SqlBuilder::updateSql() for UPDATE statement in include/pq/orm/Repository.hpp
- [x] T066 [US2] Implement SqlBuilder::deleteSql() for DELETE statement in include/pq/orm/Repository.hpp
- [x] T067 [US2] Implement SqlBuilder::selectAllSql() for SELECT * in include/pq/orm/Repository.hpp
- [x] T068 [US2] Implement SqlBuilder::selectByIdSql() for SELECT by PK in include/pq/orm/Repository.hpp
- [x] T069 [US2] Implement Repository<Entity, PK> template class in include/pq/orm/Repository.hpp
- [x] T070 [US2] Implement Repository::save() with INSERT and RETURNING in include/pq/orm/Repository.hpp
- [x] T071 [US2] Implement Repository::saveAll() with batch INSERT in include/pq/orm/Repository.hpp
- [x] T072 [US2] Implement Repository::findById() with prepared statement in include/pq/orm/Repository.hpp
- [x] T073 [US2] Implement Repository::findAll() in include/pq/orm/Repository.hpp
- [x] T074 [US2] Implement Repository::update() with UPDATE by PK in include/pq/orm/Repository.hpp
- [x] T075 [US2] Implement Repository::remove() with DELETE in include/pq/orm/Repository.hpp
- [x] T076 [US2] Implement Repository::removeById() in include/pq/orm/Repository.hpp
- [x] T077 [US2] Implement Repository::removeAll() with batch DELETE in include/pq/orm/Repository.hpp
- [x] T078 [US2] Implement Repository::count() and existsById() in include/pq/orm/Repository.hpp
- [ ] T079 [US2] Integration test for Repository CRUD operations in tests/integration/test_crud.cpp

**Checkpoint**: ⏳ Repository로 Entity 저장, 조회, 수정, 삭제 가능. findById()로 단일 조회 가능. (Integration test 대기)

---

## Phase 5: User Story 3 - Raw SQL 쿼리 실행 (Priority: P3)

**Goal**: 개발자가 직접 SQL 문자열을 작성하여 실행하고 결과를 Entity 또는 원시 데이터로 받음

**Independent Test**: Raw SQL 쿼리 실행 후 결과를 Entity로 매핑하거나 원시 데이터로 접근

### Tests for User Story 3 ⚠️

- [ ] T080 [P] [US3] Unit test for ParamConverter type-safe parameter binding in tests/unit/test_connection.cpp
- [ ] T081 [P] [US3] Integration test for SELECT with WHERE clause in tests/integration/test_raw_query.cpp
- [ ] T082 [P] [US3] Integration test for parameterized query with $1, $2 in tests/integration/test_raw_query.cpp
- [ ] T083 [P] [US3] Integration test for invalid SQL error handling in tests/integration/test_raw_query.cpp

### Implementation for User Story 3

- [x] T084 [P] [US3] Implement ParamConverter<T> template for parameter binding in include/pq/core/Types.hpp
- [x] T085 [US3] Implement Connection::executeParams<Args...>() variadic template in include/pq/core/Connection.hpp
- [x] T086 [US3] Implement Connection::prepare() for prepared statements in src/core/Connection.cpp
- [x] T087 [US3] Implement Connection::executePrepared() in src/core/Connection.cpp
- [x] T088 [US3] Implement Repository::executeQuery() for raw SELECT with Entity mapping in include/pq/orm/Repository.hpp
- [x] T089 [US3] Implement Repository::executeQueryOne() for single result in include/pq/orm/Repository.hpp
- [x] T090 [US3] Implement QueryResult error message extraction with SQLSTATE in include/pq/core/QueryResult.hpp

**Checkpoint**: ⏳ Raw SQL 쿼리로 복잡한 조건 조회 가능. 파라미터 바인딩으로 SQL Injection 방지. (Integration test 대기)

---

## Phase 6: User Story 4 - Transaction 관리 (Priority: P4)

**Goal**: 여러 데이터베이스 작업을 하나의 Transaction으로 묶어 원자적으로 실행

**Independent Test**: Transaction 내 여러 작업 후 commit/rollback 동작 확인, RAII auto-rollback 검증

### Tests for User Story 4 ⚠️

- [ ] T091 [P] [US4] Unit test for Transaction RAII auto-rollback in tests/unit/test_transaction.cpp
- [ ] T092 [P] [US4] Unit test for Transaction commit success in tests/unit/test_transaction.cpp
- [ ] T093 [P] [US4] Unit test for Savepoint create and release in tests/unit/test_transaction.cpp
- [ ] T094 [P] [US4] Integration test for Transaction commit with multiple operations in tests/integration/test_transaction.cpp
- [ ] T095 [P] [US4] Integration test for Transaction rollback on error in tests/integration/test_transaction.cpp

### Implementation for User Story 4

- [x] T096 [US4] Implement Connection::beginTransaction() in src/core/Connection.cpp
- [x] T097 [US4] Implement Connection::commit() in src/core/Connection.cpp
- [x] T098 [US4] Implement Connection::rollback() in src/core/Connection.cpp
- [x] T099 [US4] Implement Connection::inTransaction() state tracking in src/core/Connection.cpp
- [x] T100 [US4] Implement Transaction class constructor with BEGIN in include/pq/core/Transaction.hpp
- [x] T101 [US4] Implement Transaction destructor with auto-rollback in src/core/Transaction.cpp
- [x] T102 [US4] Implement Transaction move semantics (Rule of Five) in src/core/Transaction.cpp
- [x] T103 [US4] Implement Transaction::commit() in src/core/Transaction.cpp
- [x] T104 [US4] Implement Transaction::rollback() in src/core/Transaction.cpp
- [x] T105 [US4] Implement Savepoint class with SAVEPOINT/RELEASE in include/pq/core/Transaction.hpp
- [x] T106 [US4] Implement Savepoint::rollbackTo() in src/core/Transaction.cpp

**Checkpoint**: ⏳ Transaction RAII로 자동 롤백. commit()으로 영구 저장. Savepoint로 부분 롤백. (Unit/Integration test 대기)

---

## Phase 7: User Story 5 - Connection Pool 관리 (Priority: P5)

**Goal**: Connection Pool로 연결 재사용, 최대 연결 수 제한, 자동 반환

**Independent Test**: Pool에서 연결 획득/반환, 최대 연결 수 도달 시 대기/타임아웃 확인

### Tests for User Story 5 ⚠️

- [ ] T107 [P] [US5] Unit test for ConnectionPool configuration in tests/unit/test_pool.cpp
- [ ] T108 [P] [US5] Unit test for PooledConnection RAII return in tests/unit/test_pool.cpp
- [ ] T109 [P] [US5] Integration test for Pool acquire/release cycle in tests/integration/test_pool.cpp
- [ ] T110 [P] [US5] Integration test for Pool max connections limit in tests/integration/test_pool.cpp
- [ ] T111 [P] [US5] Integration test for Pool timeout on exhausted in tests/integration/test_pool.cpp

### Implementation for User Story 5

- [x] T112 [P] [US5] Implement PoolConfig struct with maxSize, timeout, validation in include/pq/core/ConnectionPool.hpp
- [x] T113 [US5] Implement ConnectionPool class with vector<PgConnPtr> storage in include/pq/core/ConnectionPool.hpp
- [x] T114 [US5] Implement PooledConnection RAII wrapper in include/pq/core/ConnectionPool.hpp
- [x] T115 [US5] Implement ConnectionPool::acquire() with mutex and condition_variable in src/core/ConnectionPool.cpp
- [x] T116 [US5] Implement ConnectionPool::release() for connection return in src/core/ConnectionPool.cpp
- [x] T117 [US5] Implement ConnectionPool timeout logic with std::chrono in src/core/ConnectionPool.cpp
- [x] T118 [US5] Implement ConnectionPool idle connection validation in src/core/ConnectionPool.cpp
- [x] T119 [US5] Implement ConnectionPool destructor for cleanup in src/core/ConnectionPool.cpp

**Checkpoint**: ⏳ Connection Pool로 연결 효율적 재사용. 최대 연결 수 제한. RAII 자동 반환. (Unit/Integration test 대기)

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [x] T120 [P] Create convenience header include/pq/pq.hpp including all core and orm headers
- [x] T121 [P] Create complete usage example in examples/usage_example.cpp
- [x] T122 [P] Update README.md with API documentation and examples
- [ ] T123 Run quickstart.md validation - verify all code examples compile and run
- [ ] T124 Implement additional unit tests to achieve 80% code coverage
- [ ] T125 Performance benchmark: verify libpq overhead < 5% (SC-002)
- [ ] T126 Memory leak test: 100만 회 연결 cycle (SC-005)
- [ ] T127 Final code review and cleanup

---

## Dependencies & Execution Order

### Phase Dependencies

```
Phase 1 (Setup)
     │
     ▼
Phase 2 (Foundational) ──── BLOCKS ALL USER STORIES ────
     │
     ├──────────────────┬──────────────────┬──────────────────┬──────────────────┐
     ▼                  ▼                  ▼                  ▼                  ▼
Phase 3 (US1)      Phase 4 (US2)     Phase 5 (US3)     Phase 6 (US4)     Phase 7 (US5)
   P1 🎯 MVP          P2                 P3                 P4                 P5
     │                  │                  │                  │                  │
     └──────────────────┴──────────────────┴──────────────────┴──────────────────┘
                                          │
                                          ▼
                                  Phase 8 (Polish)
```

### User Story Dependencies

| Story | Dependencies | Can Start After |
|-------|--------------|-----------------|
| **US1 (P1)** | Phase 2 완료 | Foundational 완료 |
| **US2 (P2)** | Phase 2 완료, US1 부분 필요 (Entity, Mapper) | US1 T045 완료 |
| **US3 (P3)** | Phase 2 완료, US2 Connection 필요 | US2 T062 완료 |
| **US4 (P4)** | Phase 2 완료, US2 Connection 필요 | US2 T062 완료 |
| **US5 (P5)** | Phase 2 완료, US2 Connection 필요 | US2 T062 완료 |

### Within Each User Story

1. Tests MUST be written and FAIL before implementation
2. Infrastructure/Types before higher-level abstractions
3. Core classes before convenience methods
4. Unit tests before integration tests

### Parallel Opportunities

**Phase 2 (Foundational)**: T006-T024 all [P] - 19 parallel tasks
**Phase 3 (US1)**: T025-T030 tests [P], T031-T032 enums [P], T040-T041 [P]
**Phase 4 (US2)**: T047-T052 tests [P], T053-T054 config [P], T063 SqlBuilder [P]
**Phase 5 (US3)**: T080-T083 tests [P], T084 ParamConverter [P]
**Phase 6 (US4)**: T091-T095 tests [P]
**Phase 7 (US5)**: T107-T111 tests [P], T112 config [P]
**Phase 8 (Polish)**: T120-T122 [P]

---

## Parallel Example: Foundational Phase

```bash
# All foundational tasks can run in parallel:
T006: "Implement PgConnDeleter struct in include/pq/core/PqHandle.hpp"
T007: "Implement PgResultDeleter struct in include/pq/core/PqHandle.hpp"
T011: "Implement DbError struct in include/pq/core/Result.hpp"
T012: "Implement Result<T, E> class in include/pq/core/Result.hpp"
T015: "Implement PgTypeTraits<int> in include/pq/core/Types.hpp"
T016: "Implement PgTypeTraits<std::string> in include/pq/core/Types.hpp"
# ... all [P] tasks
```

## Parallel Example: User Story 1

```bash
# Launch all tests for US1 together:
T025: "Unit test for ColumnFlags in tests/unit/test_entity.cpp"
T026: "Unit test for ColumnInfo in tests/unit/test_entity.cpp"
T027: "Unit test for EntityMetadata in tests/unit/test_entity.cpp"
T028: "Unit test for PQ_ENTITY macro in tests/unit/test_entity.cpp"
T029: "Unit test for PQ_COLUMN macro in tests/unit/test_entity.cpp"
T030: "Unit test for PQ_REGISTER_ENTITY in tests/unit/test_entity.cpp"

# After tests, launch parallel implementation:
T031: "Implement ColumnFlags enum in include/pq/orm/Entity.hpp"
T032: "Implement ColumnInfo struct in include/pq/orm/Entity.hpp"
T040: "Implement Row class in include/pq/core/QueryResult.hpp"
T041: "Implement RowIterator in include/pq/core/QueryResult.hpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test Entity 정의 and 매핑 independently
5. Demo: 클래스에 매크로 적용 → 메타데이터 접근 가능

### Incremental Delivery

| Increment | Stories | Deliverable |
|-----------|---------|-------------|
| **MVP** | Setup + Foundational + US1 | Entity 정의 및 매핑 |
| **+CUD** | + US2 | Repository CRUD 작업 |
| **+Raw** | + US3 | Raw SQL 쿼리 실행 |
| **+Tx** | + US4 | Transaction 관리 |
| **+Pool** | + US5 | Connection Pool (성능 최적화) |

### Recommended Order (Single Developer)

```
1. Setup (T001-T005) ─────────────────────▶ 프로젝트 빌드 가능
2. Foundational (T006-T024) ──────────────▶ Core 인프라 완료
3. US1: Entity (T025-T046) ───────────────▶ 🎯 MVP 완료
4. US2: Repository (T047-T079) ───────────▶ CRUD 작업 가능
5. US3: Raw SQL (T080-T090) ──────────────▶ 복잡한 쿼리 지원
6. US4: Transaction (T091-T106) ──────────▶ 데이터 무결성
7. US5: Pool (T107-T119) ─────────────────▶ 성능 최적화
8. Polish (T120-T127) ────────────────────▶ 릴리스 준비
```

---

## Summary

| Phase | Task Count | Completed | Status |
|-------|------------|-----------|--------|
| Setup | 5 | 5 | ✅ 100% |
| Foundational | 19 | 19 | ✅ 100% |
| US1 (P1) | 22 | 22 | ✅ 100% |
| US2 (P2) | 33 | 32 | ⏳ 97% (integration test 대기) |
| US3 (P3) | 11 | 7 | ⏳ 64% (tests 대기) |
| US4 (P4) | 16 | 11 | ⏳ 69% (tests 대기) |
| US5 (P5) | 13 | 8 | ⏳ 62% (tests 대기) |
| Polish | 8 | 3 | ⏳ 38% |
| **Total** | **127** | **107** | **84% 완료** |

**MVP Scope**: Phase 1 + Phase 2 + Phase 3 (US1) = ✅ **COMPLETE**

**남은 작업**: 주로 테스트 코드 작성 (Unit/Integration tests)

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing (TDD)
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Avoid: vague tasks, same file conflicts, cross-story dependencies
- Constitution compliance verified at each phase checkpoint
