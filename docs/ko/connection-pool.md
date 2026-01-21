# 커넥션 풀

PosixLibPq는 멀티스레드 애플리케이션에서 효율적인 데이터베이스 연결 관리를 위한 스레드 안전 커넥션 풀을 제공합니다.

## 기본 사용법

```cpp
#include <pq/pq.hpp>

// 풀 설정
pq::PoolConfig config;
config.connectionString = "host=localhost dbname=mydb user=postgres";
config.minSize = 2;   // 최소 유휴 연결 수
config.maxSize = 10;  // 최대 총 연결 수

// 풀 생성
pq::ConnectionPool pool(config);

// 연결 획득
auto connResult = pool.acquire();
if (connResult) {
    pq::PooledConnection& conn = *connResult;
    
    // 연결 사용
    conn->execute("SELECT * FROM users");
    
}  // 연결이 자동으로 풀로 반환됨
```

## 풀 설정

```cpp
struct PoolConfig {
    std::string connectionString;           // 필수: 연결 문자열
    size_t maxSize = 10;                    // 최대 연결 수
    size_t minSize = 1;                     // 최소 유휴 연결 수
    std::chrono::milliseconds acquireTimeout{5000};   // acquire() 타임아웃
    std::chrono::milliseconds idleTimeout{60000};     // 유휴 검증 타임아웃
    bool validateOnAcquire = true;          // 반환 전 연결 검증
};
```

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `connectionString` | (필수) | PostgreSQL 연결 문자열 |
| `maxSize` | 10 | 최대 연결 수 |
| `minSize` | 1 | 유지할 최소 유휴 연결 수 |
| `acquireTimeout` | 5000ms | 연결 대기 시간 |
| `idleTimeout` | 60000ms | 유휴 연결 검증 전 대기 시간 |
| `validateOnAcquire` | true | 반환 전 연결 테스트 |

## PooledConnection

`PooledConnection`은 소멸 시 자동으로 연결을 풀로 반환하는 RAII 래퍼입니다.

```cpp
auto connResult = pool.acquire();
if (connResult) {
    pq::PooledConnection conn = std::move(*connResult);
    
    // -> 또는 *로 기본 연결 접근
    conn->execute("SELECT 1");
    (*conn).execute("SELECT 2");
    
    // 유효성 확인
    if (conn.isValid()) {
        // 연결 정상
    }
    
    // 수동 반환 (선택사항 - 소멸자가 수행)
    conn.release();
}
```

### PooledConnection API

| 메서드 | 설명 |
|--------|------|
| `operator*()` | 기본 `Connection` 접근 |
| `operator->()` | 기본 `Connection` 멤버 접근 |
| `isValid()` | 연결이 여전히 유효한지 확인 |
| `release()` | 연결을 수동으로 풀로 반환 |

## 풀 통계

```cpp
pq::ConnectionPool pool(config);

std::cout << "유휴: " << pool.idleCount() << std::endl;
std::cout << "사용 중: " << pool.activeCount() << std::endl;
std::cout << "총: " << pool.totalCount() << std::endl;
std::cout << "최대: " << pool.maxSize() << std::endl;
```

| 메서드 | 설명 |
|--------|------|
| `idleCount()` | 풀의 유휴 연결 수 |
| `activeCount()` | 현재 사용 중인 연결 수 |
| `totalCount()` | 총 연결 수 (유휴 + 사용 중) |
| `maxSize()` | 설정된 최대 풀 크기 |

## 풀 관리

```cpp
pq::ConnectionPool pool(config);

// 모든 유휴 연결 닫기
pool.drain();

// 풀 종료 - 모든 연결 닫힘
pool.shutdown();
// 종료 후 acquire()는 실패
```

## 커스텀 타임아웃으로 획득

```cpp
// 설정의 기본 타임아웃 사용
auto conn1 = pool.acquire();

// 커스텀 타임아웃 사용
auto conn2 = pool.acquire(std::chrono::milliseconds(1000));

if (!conn2) {
    // 타임아웃 또는 에러
    std::cerr << "획득 실패: " << conn2.error().message << std::endl;
}
```

## 스레드 안전성

`ConnectionPool`은 완전히 스레드 안전합니다:

```cpp
pq::ConnectionPool pool(config);

// 여러 스레드에서 사용 가능
std::vector<std::thread> threads;
for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&pool, i]() {
        auto conn = pool.acquire();
        if (conn) {
            conn->execute("SELECT $1", {std::to_string(i)});
        }
    });
}

for (auto& t : threads) {
    t.join();
}
```

## Repository와 함께 사용

```cpp
pq::ConnectionPool pool(config);

void handleRequest() {
    auto connResult = pool.acquire();
    if (!connResult) {
        throw std::runtime_error("연결을 사용할 수 없음");
    }
    
    // 풀링된 연결로 Repository 생성
    pq::Repository<User, int> userRepo(**connResult);
    
    auto users = userRepo.findAll();
    // ... users 사용 ...
    
}  // 연결이 풀로 반환됨
```

## 트랜잭션과 함께 사용

```cpp
pq::ConnectionPool pool(config);

void transferMoney(int from, int to, double amount) {
    auto connResult = pool.acquire();
    if (!connResult) {
        throw std::runtime_error("연결 없음");
    }
    
    pq::PooledConnection conn = std::move(*connResult);
    
    {
        pq::Transaction tx(*conn);
        if (!tx) {
            throw std::runtime_error("트랜잭션 시작 불가");
        }
        
        conn->execute("UPDATE accounts SET balance = balance - $1 WHERE id = $2",
                      {std::to_string(amount), std::to_string(from)});
        conn->execute("UPDATE accounts SET balance = balance + $1 WHERE id = $2",
                      {std::to_string(amount), std::to_string(to)});
        
        tx.commit();
    }
    
}  // 연결이 풀로 반환됨
```

## 에러 핸들링

```cpp
auto connResult = pool.acquire();

if (!connResult) {
    // 타임아웃 또는 연결 실패일 수 있음
    const pq::DbError& err = connResult.error();
    
    if (err.message.find("timeout") != std::string::npos) {
        // 풀 소진, 모든 연결 사용 중
        std::cerr << "풀 소진, 나중에 다시 시도" << std::endl;
    } else {
        // 연결 에러
        std::cerr << "연결 에러: " << err.message << std::endl;
    }
}
```

## 연결 검증

`validateOnAcquire`가 true(기본값)이면 풀은 연결을 반환하기 전에 검증합니다:

```cpp
pq::PoolConfig config;
config.connectionString = "...";
config.validateOnAcquire = true;   // 연결 상태 테스트
config.idleTimeout = std::chrono::milliseconds(30000);  // 30초 유휴 후 검증

pq::ConnectionPool pool(config);

// 검증된 연결을 가져옴
auto conn = pool.acquire();
// 검증 실패 시 풀은 새 연결 생성
```

## 모범 사례

### 1. 풀 크기 적절히 설정

```cpp
// 고려 사항:
// - 동시 스레드/요청 수
// - PostgreSQL max_connections 설정
// - 사용 가능한 메모리

pq::PoolConfig config;
config.minSize = 2;   // 빠른 접근을 위해 2개 준비
config.maxSize = 20;  // 이 수를 초과하지 않음
```

### 2. 연결 스코프를 짧게 유지

```cpp
// 좋음: 짧은 스코프
void handleRequest() {
    auto conn = pool.acquire();
    if (conn) {
        auto result = conn->execute("SELECT * FROM data");
        // 결과 처리...
    }
}  // 연결 즉시 반환

// 나쁨: 연결을 오래 보유
void badExample() {
    auto conn = pool.acquire();  // 획득
    processData();               // 연결 없이 긴 작업
    doNetworkCall();             // 연결 보유 중이지만 미사용
    conn->execute("...");        // 이제서야 사용
}  // 전체 시간 동안 연결 보유
```

### 3. 풀 소진 처리

```cpp
auto conn = pool.acquire(std::chrono::milliseconds(100));
if (!conn) {
    // 503 Service Unavailable 반환, 나중에 재시도 등
    return HttpResponse::ServiceUnavailable();
}
```

### 4. 정상적인 종료

```cpp
int main() {
    pq::ConnectionPool pool(config);
    
    // ... 애플리케이션 실행 ...
    
    // 깔끔한 종료
    pool.shutdown();
    
    return 0;
}
```

### 5. 비동기 작업에서 PooledConnection 보유 금지

```cpp
// 나쁨: 비동기 경계를 넘어 보유
auto conn = pool.acquire();
asyncOperation([conn = std::move(conn)]() {
    // 전체 비동기 작업 동안 연결 보유
});

// 좋음: 필요할 때 획득
asyncOperation([&pool]() {
    auto conn = pool.acquire();
    // 빠르게 사용하고 반환
});
```

## 모니터링

```cpp
// 간단한 모니터링
void logPoolStats(const pq::ConnectionPool& pool) {
    std::cout << "풀 상태:"
              << " 유휴=" << pool.idleCount()
              << " 사용중=" << pool.activeCount()
              << " 총=" << pool.totalCount()
              << " 최대=" << pool.maxSize()
              << std::endl;
}

// 주기적으로 호출
logPoolStats(pool);
```

## 다음 단계

- [트랜잭션](transactions.md) - 풀링된 연결과 트랜잭션
- [에러 핸들링](error-handling.md) - 풀 에러 처리
