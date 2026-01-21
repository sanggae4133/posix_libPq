# 시작하기

이 가이드는 PosixLibPq를 프로젝트에서 사용하는 방법을 안내합니다.

## 요구사항

| 요구사항 | 버전 |
|---------|------|
| C++ 표준 | C++17 이상 |
| 컴파일러 | GCC 8+, Clang 7+, 또는 MSVC 2019+ |
| PostgreSQL libpq | 15.x+ (PQlibVersion >= 150007) |
| CMake | 3.16+ |
| Google Test | (선택사항, 테스트용) |

## 설치

### 방법 1: CMake FetchContent 사용 (권장)

`CMakeLists.txt`에 추가:

```cmake
include(FetchContent)
FetchContent_Declare(
    posixlibpq
    GIT_REPOSITORY https://github.com/your-repo/posixLibPq.git
    GIT_TAG main
)

# 라이브러리만 빌드하려면 예제와 테스트 비활성화
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(posixlibpq)

# 타겟에 링크
target_link_libraries(your_app PRIVATE pq::pq)
```

### 방법 2: 서브디렉토리로 사용

저장소를 프로젝트에 클론하거나 복사:

```bash
git submodule add https://github.com/your-repo/posixLibPq.git external/posixLibPq
```

그리고 `CMakeLists.txt`에서:

```cmake
set(PQ_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(external/posixLibPq)

target_link_libraries(your_app PRIVATE pq::pq)
```

### 방법 3: 시스템 설치

```bash
# 저장소 클론
git clone https://github.com/your-repo/posixLibPq.git
cd posixLibPq

# 빌드
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# 설치 (sudo 필요할 수 있음)
cmake --install .
```

프로젝트에서 사용:

```cmake
find_package(pq REQUIRED)
target_link_libraries(your_app PRIVATE pq::pq)
```

## 소스에서 빌드

### 사전 준비

PostgreSQL 개발 라이브러리 설치:

```bash
# Ubuntu/Debian
sudo apt-get install libpq-dev

# RHEL/CentOS
sudo yum install postgresql-devel

# macOS (Homebrew)
brew install libpq
```

### 빌드 명령어

```bash
mkdir build && cd build

# 기본 빌드
cmake ..
cmake --build .

# 테스트와 함께
cmake .. -DPQ_BUILD_TESTS=ON
cmake --build .
ctest --output-on-failure

# 빌드 후 자동 테스트 실행
cmake .. -DPQ_RUN_TESTS=ON
cmake --build .

# 릴리스 빌드
cmake .. -DCMAKE_BUILD_TYPE=Release
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
| `PQ_LIBPQ_LIBRARY` | (자동) | libpq 라이브러리 파일 수동 경로 |

#### 옵션 상세 설명

- **`PQ_INSTALL`**: `ON`이면 `cmake --install .` 명령으로 헤더, 라이브러리, CMake 설정 파일을 시스템에 설치할 수 있는 타겟을 생성합니다. 다른 프로젝트의 서브디렉토리로 사용할 때는 `OFF`로 설정하세요.

- **`PQ_LIBPQ_INCLUDE_DIR` / `PQ_LIBPQ_LIBRARY`**: 자동 탐지가 실패할 때 PostgreSQL 경로를 수동으로 지정합니다. 두 옵션을 함께 설정해야 합니다.

### PostgreSQL 수동 경로 설정

CMake가 libpq를 자동으로 찾지 못할 때 경로를 직접 지정할 수 있습니다:

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/path/to/include \
  -DPQ_LIBPQ_LIBRARY=/path/to/libpq.so
```

### macOS에서 Homebrew libpq 사용 시

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/opt/homebrew/opt/libpq/include \
  -DPQ_LIBPQ_LIBRARY=/opt/homebrew/opt/libpq/lib/libpq.dylib
```

### Linux 일반적인 경로

```bash
cmake .. \
  -DPQ_LIBPQ_INCLUDE_DIR=/usr/include/postgresql \
  -DPQ_LIBPQ_LIBRARY=/usr/lib/x86_64-linux-gnu/libpq.so
```

## 빠른 시작

### 1. 헤더 포함

```cpp
#include <pq/pq.hpp>
```

### 2. Entity 정의

```cpp
struct User {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(User)
```

### 3. 데이터베이스 연결

```cpp
// 연결 문자열 사용
pq::Connection conn("host=localhost dbname=mydb user=postgres password=secret");

// 또는 ConnectionConfig 사용
pq::ConnectionConfig config;
config.host = "localhost";
config.port = 5432;
config.database = "mydb";
config.user = "postgres";
config.password = "secret";

pq::Connection conn(config);
```

### 4. CRUD 작업 수행

```cpp
pq::Repository<User, int> userRepo(conn);

// 생성 (Create)
User user;
user.name = "John Doe";
user.email = "john@example.com";
auto savedResult = userRepo.save(user);
if (savedResult) {
    std::cout << "저장된 사용자 ID: " << savedResult->id << std::endl;
}

// 조회 (Read)
auto findResult = userRepo.findById(1);
if (findResult && *findResult) {
    User& found = **findResult;
    std::cout << "검색됨: " << found.name << std::endl;
}

// 수정 (Update)
if (savedResult) {
    savedResult->name = "Jane Doe";
    userRepo.update(*savedResult);
}

// 삭제 (Delete)
userRepo.removeById(1);
```

## 다음 단계

- [Entity 매핑](entity-mapping.md) - 복잡한 Entity 매핑 방법
- [Repository 패턴](repository-pattern.md) - 전체 Repository API
- [트랜잭션](transactions.md) - 트랜잭션 관리
- [에러 핸들링](error-handling.md) - 올바른 에러 처리
