# PosixLibPq

C++17 기반 PostgreSQL ORM 라이브러리 - RAII, Type Safety, Zero-Cost Abstraction

## 특징

- **RAII-First**: 모든 리소스(Connection, Result, Transaction)가 RAII 패턴으로 자동 관리
- **Type Safety**: C++17 타입 시스템을 활용한 컴파일 타임 안전성 (std::optional, 강타입)
- **Zero-Cost Abstraction**: libpq 직접 사용 대비 최소한의 오버헤드
- **JPA-Like API**: 매크로 기반의 선언적 Entity 매핑과 Repository 패턴
- **SOLID 준수**: 확장 가능하고 유지보수하기 쉬운 아키텍처

## 요구사항

- **C++ Standard**: C++17
- **Compiler**: GCC 8+ 또는 Clang 7+
- **Platform**: RHEL 8+ (POSIX compliant)
- **Dependencies**: 
  - PostgreSQL libpq 15.x (PQlibVersion >= 150007)
  - CMake 3.16+
  - Google Test (테스트용, 선택사항)

## 빠른 시작

### 빌드

```bash
mkdir build && cd build
cmake ..
make
```

### Entity 정의

```cpp
#include <pq/pq.hpp>

struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;  // NULL 가능
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name")
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### Repository 사용

```cpp
// 연결
pq::Connection conn("host=localhost dbname=mydb user=postgres");

// Repository 생성
pq::Repository<User, int> userRepo(conn);

// Save
User user;
user.name = "John";
user.email = "john@example.com";
auto saved = userRepo.save(user);
if (saved) {
    std::cout << "Saved with ID: " << saved->id << std::endl;
}

// FindById
auto found = userRepo.findById(1);
if (found && *found) {
    std::cout << "Found: " << (*found)->name << std::endl;
}

// FindAll
auto all = userRepo.findAll();
if (all) {
    for (const auto& u : *all) {
        std::cout << u.name << std::endl;
    }
}

// Update
saved->name = "Jane";
userRepo.update(*saved);

// Remove
userRepo.remove(*saved);
```

### Raw Query (조건부 조회)

```cpp
// findById는 Repository에 포함되어 있지만,
// 다른 조건 조회는 Raw Query 사용
std::vector<std::string> params = {"john@example.com"};
auto result = userRepo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    params
);
```

### Transaction

```cpp
{
    pq::Transaction tx(conn);
    if (!tx) {
        // 트랜잭션 시작 실패
    }
    
    userRepo.save(user1);
    userRepo.save(user2);
    
    tx.commit();  // 명시적 커밋
}  // commit 없이 스코프 벗어나면 자동 롤백
```

## 프로젝트 구조

```
posixLibPq/
├── include/pq/
│   ├── core/           # 핵심 컴포넌트
│   │   ├── PqHandle.hpp    # RAII 래퍼 (PGconn, PGresult)
│   │   ├── Result.hpp      # Result<T,E> 에러 핸들링
│   │   ├── Types.hpp       # TypeTraits 시스템
│   │   ├── Connection.hpp  # DB 연결
│   │   ├── QueryResult.hpp # 쿼리 결과
│   │   └── Transaction.hpp # 트랜잭션 관리
│   ├── orm/            # ORM 레이어
│   │   ├── Entity.hpp      # 매크로 기반 Entity 정의
│   │   ├── Mapper.hpp      # 결과-Entity 매핑
│   │   └── Repository.hpp  # Repository 패턴
│   └── pq.hpp          # 편의 헤더
├── src/
│   └── core/
├── examples/
│   └── usage_example.cpp
├── tests/
└── CMakeLists.txt
```

## 설계 원칙

### RAII (Resource Acquisition Is Initialization)

모든 libpq 리소스는 RAII로 관리됩니다:

```cpp
// PGconn* 자동 관리
pq::core::PgConnPtr conn = pq::core::makePgConn(connStr);
// 스코프 종료 시 자동 PQfinish()

// Transaction 자동 롤백
{
    pq::Transaction tx(conn);
    // ... 작업 ...
}  // commit 없이 종료되면 자동 롤백
```

### 엄격한 매핑 정책 (FR-014)

기본적으로 쿼리 결과에 매핑되지 않은 컬럼이 있으면 예외 발생:

```cpp
// 완화된 설정 (필요시)
pq::MapperConfig config;
config.strictColumnMapping = true;
config.ignoreExtraColumns = true;  // 추가 컬럼 무시

pq::Repository<User, int> repo(conn, config);
```

### Type Safety

`std::optional`을 사용한 NULL 처리:

```cpp
struct User {
    int id;                      // NOT NULL
    std::optional<int> age;      // NULL 가능
};

// NULL 컬럼을 non-optional에 매핑 시도하면 예외 발생
```

## 라이선스

MIT License
