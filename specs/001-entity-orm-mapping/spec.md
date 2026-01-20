# Feature Specification: Entity ORM Mapping

**Feature Branch**: `001-entity-orm-mapping`  
**Created**: 2026-01-20  
**Status**: Draft  
**Input**: User description: "ORM 기능을 구현해서 class entity를 database table에 매핑해야 해. raw query를 스트링으로 받는 기능 필요."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Entity 정의 및 테이블 매핑 (Priority: P1)

개발자가 C++ 클래스를 PostgreSQL 데이터베이스 테이블에 매핑하기 위해 Entity로 정의한다. 클래스의 멤버 변수가 테이블의 컬럼에 대응되며, 매핑 정보를 선언적으로 지정할 수 있다.

**Why this priority**: ORM의 가장 기본적인 기능으로, 이 기능 없이는 다른 모든 기능이 작동하지 않는다. Entity 정의는 전체 ORM 시스템의 기반이다.

**Independent Test**: 클래스를 Entity로 정의하고, 매핑 정보가 올바르게 생성되는지 확인. 테이블 이름, 컬럼 이름, 타입 매핑이 정확한지 검증.

**Acceptance Scenarios**:

1. **Given** 개발자가 User 클래스를 정의했을 때, **When** Entity 매핑 매크로/어노테이션을 적용하면, **Then** 클래스가 "users" 테이블에 매핑되어야 한다
2. **Given** Entity 클래스에 id, name, email 멤버가 있을 때, **When** 매핑이 완료되면, **Then** 각 멤버가 해당 컬럼(id, name, email)에 매핑되어야 한다
3. **Given** 컬럼 타입을 명시적으로 지정했을 때, **When** 매핑이 완료되면, **Then** 지정된 PostgreSQL 타입으로 변환되어야 한다
4. **Given** Primary Key를 지정했을 때, **When** Entity가 생성되면, **Then** 해당 필드가 식별자로 인식되어야 한다

---

### User Story 2 - Repository를 통한 CUD 작업 (Priority: P2)

개발자가 Repository 패턴을 사용하여 Entity에 대한 기본 CUD(Create, Update, Delete) 작업과 전체 조회(findAll)를 수행한다. 특정 조건 조회(findById, findByName 등)는 Raw Query를 통해 처리한다.

**Why this priority**: Entity 정의 다음으로 가장 자주 사용되는 기능. 개발자가 실제로 데이터를 다루기 위해 반드시 필요하다.

**Independent Test**: Repository를 통해 Entity를 저장, 수정, 삭제하고 결과가 데이터베이스에 반영되는지 확인.

**Acceptance Scenarios**:

1. **Given** User Entity 객체가 있을 때, **When** repository.save(user)를 호출하면, **Then** 데이터베이스에 새 레코드가 삽입되어야 한다
2. **Given** 여러 User Entity 객체가 있을 때, **When** repository.saveAll(users)를 호출하면, **Then** 모든 레코드가 일괄 삽입되어야 한다
3. **Given** 수정된 User Entity가 있을 때, **When** repository.update(user)를 호출하면, **Then** 데이터베이스의 해당 레코드가 업데이트되어야 한다
4. **Given** 데이터베이스에 User가 있을 때, **When** repository.remove(user)를 호출하면, **Then** 해당 레코드가 삭제되어야 한다
5. **Given** 여러 User Entity가 있을 때, **When** repository.removeAll(users)를 호출하면, **Then** 모든 해당 레코드가 일괄 삭제되어야 한다
6. **Given** 여러 User가 데이터베이스에 있을 때, **When** repository.findAll()을 호출하면, **Then** 모든 User 목록이 반환되어야 한다
7. **Given** 특정 조건으로 User를 조회하고 싶을 때, **When** Raw Query를 통해 "SELECT * FROM users WHERE id = $1"을 실행하면, **Then** 조건에 맞는 User가 반환되어야 한다

---

### User Story 3 - Raw SQL 쿼리 실행 (Priority: P3)

개발자가 복잡한 쿼리나 ORM이 지원하지 않는 쿼리를 위해 직접 SQL 문자열을 작성하여 실행한다. 쿼리 결과는 Entity로 매핑하거나 원시 데이터로 받을 수 있다.

**Why this priority**: ORM의 추상화로 처리하기 어려운 복잡한 쿼리, 성능 최적화가 필요한 쿼리, 또는 데이터베이스 특화 기능을 사용해야 할 때 필수적이다.

**Independent Test**: Raw SQL 쿼리를 실행하고 결과를 Entity 또는 원시 데이터로 받아올 수 있는지 확인.

**Acceptance Scenarios**:

1. **Given** SELECT 쿼리 문자열이 있을 때, **When** connection.executeQuery("SELECT * FROM users")를 호출하면, **Then** 쿼리 결과가 반환되어야 한다
2. **Given** 파라미터가 포함된 쿼리가 있을 때, **When** connection.executeQuery("SELECT * FROM users WHERE id = $1", {1})를 호출하면, **Then** 파라미터가 바인딩된 쿼리가 실행되어야 한다
3. **Given** INSERT/UPDATE/DELETE 쿼리가 있을 때, **When** connection.executeUpdate(query)를 호출하면, **Then** 영향받은 행 수가 반환되어야 한다
4. **Given** 쿼리 결과가 있을 때, **When** 결과를 User Entity로 매핑 요청하면, **Then** User 객체 목록으로 변환되어야 한다
5. **Given** 잘못된 SQL 문법이 있을 때, **When** executeQuery를 호출하면, **Then** 적절한 오류 정보와 함께 실패해야 한다

---

### User Story 4 - Transaction 관리 (Priority: P4)

개발자가 여러 데이터베이스 작업을 하나의 Transaction으로 묶어 원자적으로 실행한다. 모든 작업이 성공하면 커밋하고, 하나라도 실패하면 전체를 롤백한다.

**Why this priority**: 데이터 무결성을 보장하기 위해 필수적인 기능이나, 기본 CRUD와 Raw Query가 먼저 작동해야 의미가 있다.

**Independent Test**: Transaction 내에서 여러 작업 수행 후 커밋/롤백이 정상 작동하는지 확인.

**Acceptance Scenarios**:

1. **Given** Transaction이 시작되었을 때, **When** 여러 save() 작업 후 commit()하면, **Then** 모든 변경사항이 데이터베이스에 영구 저장되어야 한다
2. **Given** Transaction이 시작되었을 때, **When** 작업 중 rollback()하면, **Then** 모든 변경사항이 취소되어야 한다
3. **Given** Transaction 내에서 오류가 발생했을 때, **When** 예외가 throw되면, **Then** 자동으로 롤백되어야 한다 (RAII)
4. **Given** Transaction 객체가 있을 때, **When** 스코프를 벗어나면, **Then** 커밋되지 않은 Transaction은 자동 롤백되어야 한다

---

### User Story 5 - Connection Pool 관리 (Priority: P5)

시스템이 데이터베이스 연결을 효율적으로 관리하기 위해 Connection Pool을 제공한다. 연결 생성/해제 오버헤드를 최소화하고 동시 접근을 지원한다.

**Why this priority**: 성능 최적화 기능으로, 기본 기능이 모두 작동한 후에 추가하는 것이 적절하다.

**Independent Test**: Pool에서 연결 획득/반환이 정상 작동하고, 최대 연결 수 제한이 지켜지는지 확인.

**Acceptance Scenarios**:

1. **Given** Connection Pool이 설정되었을 때, **When** getConnection()을 호출하면, **Then** 사용 가능한 연결이 반환되어야 한다
2. **Given** 연결을 사용 완료했을 때, **When** 연결 객체가 스코프를 벗어나면, **Then** 자동으로 Pool에 반환되어야 한다 (RAII)
3. **Given** 최대 연결 수에 도달했을 때, **When** 새 연결을 요청하면, **Then** 연결이 반환될 때까지 대기하거나 타임아웃되어야 한다
4. **Given** Pool이 설정되었을 때, **When** 유휴 연결이 일정 시간 지나면, **Then** 연결 유효성을 검사해야 한다

---

### Edge Cases

- Entity 클래스에 매핑되지 않은 컬럼이 쿼리 결과에 있을 때 어떻게 처리하는가?
  - **처리 방식**: 매핑되지 않은 컬럼이 결과에 있으면 예외를 발생시킨다 (엄격한 매핑 정책)
- NULL 값이 있는 컬럼을 non-optional 멤버에 매핑할 때 어떻게 처리하는가?
  - **처리 방식**: std::optional이 아닌 타입에 NULL이 매핑되면 예외를 발생시킨다
- 연결이 끊어진 상태에서 쿼리를 실행하면 어떻게 되는가?
  - **처리 방식**: 연결 상태를 확인하고, 재연결을 시도하거나 명확한 오류를 반환한다
- 매우 큰 결과 세트를 처리할 때 메모리 관리는 어떻게 하는가?
  - **처리 방식**: 커서 기반 iteration을 지원하여 전체 결과를 메모리에 올리지 않도록 한다 (페이징은 iteration으로 대체)
- 동시에 여러 스레드에서 같은 Entity를 수정하면 어떻게 되는가?
  - **처리 방식**: 각 스레드는 독립적인 Connection을 사용하며, 동시성 제어는 데이터베이스 트랜잭션 격리 수준에 의존한다

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: 시스템은 C++ 클래스를 PostgreSQL 테이블에 매핑하는 Entity 정의 메커니즘을 제공해야 한다
- **FR-002**: 시스템은 매핑된 Entity에 대해 save(), saveAll(), findAll(), update(), remove(), removeAll() 기본 CUD 및 전체 조회 작업을 지원해야 한다 (특정 조건 조회는 Raw Query 사용)
- **FR-003**: 시스템은 Raw SQL 쿼리 문자열을 직접 실행할 수 있어야 한다
- **FR-004**: 시스템은 파라미터 바인딩을 통해 SQL Injection을 방지해야 한다 (Prepared Statement)
- **FR-005**: 시스템은 쿼리 결과를 지정된 Entity 타입으로 자동 매핑할 수 있어야 한다
- **FR-006**: 시스템은 Transaction의 시작, 커밋, 롤백을 지원해야 한다
- **FR-007**: 시스템은 RAII 패턴으로 리소스(Connection, Transaction, Result)를 자동 관리해야 한다
- **FR-008**: 시스템은 Connection Pool을 통해 연결을 효율적으로 재사용해야 한다
- **FR-009**: 시스템은 NULL 가능한 컬럼을 std::optional로 표현해야 한다
- **FR-010**: 시스템은 Primary Key 지정 및 자동 생성(auto-increment/serial)을 지원해야 한다
- **FR-011**: 시스템은 PostgreSQL의 기본 데이터 타입(integer, bigint, varchar, text, boolean, timestamp 등)을 C++ 타입에 매핑해야 한다
- **FR-012**: 시스템은 쿼리 실행 오류 시 상세한 오류 정보를 제공해야 한다
- **FR-013**: 시스템은 커서 기반 결과 순회를 지원하여 대용량 결과를 메모리 효율적으로 처리해야 한다
- **FR-014**: 시스템은 쿼리 결과에 Entity에 매핑되지 않은 컬럼이 포함된 경우 예외를 발생시켜야 한다 (엄격한 매핑 정책)

### Key Entities

- **Entity**: 데이터베이스 테이블에 매핑되는 C++ 클래스의 기본 개념. 테이블 이름, Primary Key, 컬럼 매핑 정보를 포함한다
- **Repository**: 특정 Entity 타입에 대한 CUD 작업(save, saveAll, update, remove, removeAll)과 전체 조회(findAll)를 수행하는 데이터 접근 객체. 조건부 조회는 Raw Query를 통해 처리한다
- **Connection**: PostgreSQL 데이터베이스와의 단일 연결. RAII로 관리되며, 쿼리 실행의 기본 단위이다
- **ConnectionPool**: 여러 Connection을 관리하고 재사용하는 풀. 연결 획득/반환, 최대 연결 수 관리를 담당한다
- **Transaction**: 하나 이상의 데이터베이스 작업을 원자적으로 묶는 단위. RAII로 관리되며 스코프 종료 시 자동 롤백된다
- **QueryResult**: 쿼리 실행 결과를 담는 객체. 행 단위 순회, Entity 매핑, 원시 데이터 접근을 지원한다
- **ColumnMapping**: Entity 멤버와 테이블 컬럼 간의 매핑 정보. 컬럼명, 타입, nullable 여부 등을 포함한다

### Assumptions

- PostgreSQL libpq 버전 150007 이상이 설치되어 있다
- C++17 컴파일러(GCC 8+ 또는 Clang 7+)를 사용한다
- 단일 데이터베이스 연결만 지원한다 (멀티 데이터베이스는 추후 확장)
- Entity 클래스는 기본 생성자를 가진다
- 복합 Primary Key는 초기 버전에서 지원하지 않는다 (단일 PK만 지원)
- 관계 매핑(1:1, 1:N, N:M)은 초기 버전에서 지원하지 않는다

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 개발자가 Entity 클래스 정의부터 첫 CRUD 작업 수행까지 30분 이내에 완료할 수 있다
- **SC-002**: Raw SQL 쿼리 실행이 libpq 직접 사용 대비 5% 이내의 오버헤드만 발생한다
- **SC-003**: Connection Pool 사용 시 연결 획득이 새 연결 생성 대비 90% 이상 빠르다
- **SC-004**: 1만 건의 Entity 조회 작업을 10초 이내에 완료할 수 있다
- **SC-005**: 리소스 누수 없이 100만 회의 연결 획득/반환 사이클을 완료할 수 있다 (메모리 안정성)
- **SC-006**: 모든 데이터베이스 오류 상황에서 사용자가 원인을 파악할 수 있는 오류 메시지를 제공한다
- **SC-007**: 80% 이상의 코드 커버리지를 가진 테스트 스위트가 존재한다
