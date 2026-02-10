# PosixLibPq

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

RAII 리소스 관리, 컴파일 타임 타입 안전성, Zero-Cost 추상화를 제공하는 현대적인 C++17 PostgreSQL ORM 라이브러리입니다.

[English](README.md) | [문서](docs/ko/README.md)

## 특징

- **RAII 우선 설계** - 모든 리소스(Connection, Result, Transaction)가 자동으로 관리됩니다
- **타입 안전성** - `std::optional`을 활용한 NULL 처리와 컴파일 타임 타입 검사
- **Zero-Cost 추상화** - libpq 직접 사용 대비 최소한의 오버헤드
- **JPA 스타일 API** - 매크로 기반 선언적 Entity 매핑과 Repository 패턴
- **커넥션 풀** - 고성능 애플리케이션을 위한 스레드 안전 커넥션 풀
- **현대적 C++** - `std::optional`, `std::string_view`, 구조화된 바인딩 등 C++17 기능 활용

## 요구사항

| 요구사항 | 버전 |
|---------|------|
| C++ 표준 | C++17 |
| 컴파일러 | GCC 8+, Clang 7+, MSVC 2019+ |
| PostgreSQL libpq | 15.x+ |
| CMake | 3.16+ |
| Google Test | (선택사항, 테스트용) |

## 빠른 시작

### 설치

**CMake FetchContent 사용:**

```cmake
include(FetchContent)
FetchContent_Declare(
    posixlibpq
    GIT_REPOSITORY https://github.com/your-repo/posixLibPq.git
    GIT_TAG main
)
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(posixlibpq)

target_link_libraries(your_app PRIVATE pq::pq)
```

**서브디렉토리로 사용:**

```cmake
add_subdirectory(external/posixLibPq)
target_link_libraries(your_app PRIVATE pq::pq)
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
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### CRUD 작업

```cpp
// 연결
pq::Connection conn("host=localhost dbname=mydb user=postgres");

// Repository 생성
pq::Repository<User, int> userRepo(conn);

// 생성 (Create)
User user;
user.name = "John";
user.email = "john@example.com";
auto saved = userRepo.save(user);
if (saved) {
    std::cout << "저장된 ID: " << saved->id << std::endl;
}

// 조회 (Read)
auto found = userRepo.findById(1);
if (found && *found) {
    std::cout << "검색됨: " << (*found)->name << std::endl;
}

// 수정 (Update)
if (saved) {
    saved->name = "Jane";
    userRepo.update(*saved);
}

// 삭제 (Delete)
userRepo.removeById(1);
```

### 트랜잭션

```cpp
{
    pq::Transaction tx(conn);
    if (!tx) {
        // 에러 처리
        return;
    }
    
    userRepo.save(user1);
    userRepo.save(user2);
    
    tx.commit();  // 명시적 커밋
}  // commit() 호출 없이 스코프 종료 시 자동 롤백
```

### 커넥션 풀

```cpp
pq::PoolConfig config;
config.connectionString = "host=localhost dbname=mydb";
config.minSize = 2;
config.maxSize = 10;

pq::ConnectionPool pool(config);

{
    auto conn = pool.acquire();
    if (conn) {
        conn->execute("SELECT * FROM users");
    }
}  // 커넥션이 풀로 자동 반환됨
```

## 소스에서 빌드

```bash
# 클론
git clone https://github.com/your-repo/posixLibPq.git
cd posixLibPq

# 빌드
mkdir build && cd build
cmake ..
cmake --build .

# 테스트 실행
ctest --output-on-failure

# 또는 빌드 후 자동 테스트 실행
cmake .. -DPQ_RUN_TESTS=ON
cmake --build .
```

### CMake 옵션

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `PQ_BUILD_EXAMPLES` | ON | 예제 프로그램 빌드 |
| `PQ_BUILD_TESTS` | ON | 단위 테스트 빌드 |
| `PQ_RUN_TESTS` | OFF | 빌드 후 자동 테스트 실행 |
| `PQ_INSTALL` | ON | `cmake --install`용 설치 타겟 생성 |
| `PQ_LIBPQ_INCLUDE_DIR` | (자동) | `libpq-fe.h` 디렉토리 수동 경로 |
| `PQ_LIBPQ_LIBRARY` | (자동) | libpq 라이브러리 수동 경로 |

### PostgreSQL 수동 경로 지정

CMake가 libpq를 자동으로 찾지 못할 때:

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/path/to/include \
  -DPQ_LIBPQ_LIBRARY=/path/to/libpq.so
```

### macOS Homebrew libpq 사용 시

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/opt/homebrew/opt/libpq/include \
  -DPQ_LIBPQ_LIBRARY=/opt/homebrew/opt/libpq/lib/libpq.dylib
```

## 프로젝트 구조

```
posixLibPq/
├── include/pq/
│   ├── core/                 # 핵심 컴포넌트
│   │   ├── PqHandle.hpp      # RAII 래퍼
│   │   ├── Result.hpp        # Result<T,E> 에러 핸들링
│   │   ├── Types.hpp         # 타입 트레이트 시스템
│   │   ├── Connection.hpp    # 데이터베이스 연결
│   │   ├── QueryResult.hpp   # 쿼리 결과
│   │   ├── Transaction.hpp   # 트랜잭션 관리
│   │   └── ConnectionPool.hpp# 커넥션 풀링
│   ├── orm/                  # ORM 레이어
│   │   ├── Entity.hpp        # Entity 매크로
│   │   ├── Mapper.hpp        # 결과-Entity 매핑
│   │   └── Repository.hpp    # Repository 패턴
│   └── pq.hpp               # 편의 헤더
├── src/core/                # 구현 파일
├── cmake/                   # CMake 패키지 설정
│   └── pqConfig.cmake.in    # find_package(pq) 지원 템플릿
├── docs/                    # 문서
│   └── ko/                  # 한글 문서
├── examples/                # 예제 프로그램
├── tests/                   # 단위 테스트
└── CMakeLists.txt
```

### cmake/ 디렉토리

`cmake/pqConfig.cmake.in` 파일은 CMake 패키지 설정 템플릿입니다. `cmake --install`을 실행하면 `pqConfig.cmake`가 생성되어 다른 프로젝트에서 다음과 같이 사용할 수 있습니다:

```cmake
find_package(pq REQUIRED)
target_link_libraries(your_app PRIVATE pq::pq)
```

## 문서

- [시작하기](docs/ko/getting-started.md)
- [Entity 매핑](docs/ko/entity-mapping.md)
- [Repository 패턴](docs/ko/repository-pattern.md)
- [트랜잭션](docs/ko/transactions.md)
- [커넥션 풀](docs/ko/connection-pool.md)
- [에러 핸들링](docs/ko/error-handling.md)
- [커스텀 쿼리](docs/ko/custom-queries.md)
- [API 레퍼런스](docs/ko/api-reference.md)
- [타입 시스템](docs/ko/type-system.md)
- [안정성 패치 노트 (2026-02)](docs/ko/stability-fixes-2026-02.md)

## 설계 원칙

### RAII (Resource Acquisition Is Initialization)

모든 libpq 리소스는 RAII로 관리됩니다:

```cpp
{
    pq::Connection conn("...");  // PQconnectdb
    // 커넥션 사용...
}  // 자동으로 PQfinish 호출

{
    pq::Transaction tx(conn);  // BEGIN
    // 작업 수행...
}  // 커밋되지 않았다면 자동 ROLLBACK
```

### Result 타입 에러 핸들링

예외 대신 `DbResult<T>`를 반환합니다:

```cpp
auto result = userRepo.findById(1);
if (result) {
    // 성공
    auto& user = *result;
} else {
    // 에러
    std::cerr << result.error().message << std::endl;
}
```

### 타입 안전한 NULL 처리

NULL 가능 컬럼에는 `std::optional`을 사용합니다:

```cpp
struct User {
    int id;                           // NOT NULL
    std::optional<std::string> bio;   // NULL 허용
};

// NULL 컬럼을 non-optional에 매핑하면 MappingException 발생
```

## 기여하기

이슈와 풀 리퀘스트를 환영합니다!

## 라이선스

MIT License - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.
