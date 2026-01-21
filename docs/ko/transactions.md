# 트랜잭션

PosixLibPq는 예외가 발생하더라도 데이터 무결성을 보장하는 RAII 기반 트랜잭션 관리를 제공합니다.

## 기본 사용법

```cpp
#include <pq/pq.hpp>

pq::Connection conn("host=localhost dbname=mydb");

{
    pq::Transaction tx(conn);
    
    if (!tx) {
        // BEGIN 실패
        std::cerr << "트랜잭션 시작 실패" << std::endl;
        return;
    }
    
    // 작업 수행...
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Alice"});
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Bob"});
    
    // 명시적 커밋
    auto result = tx.commit();
    if (!result) {
        std::cerr << "커밋 실패: " << result.error().message << std::endl;
    }
}
// commit()이 호출되지 않았다면 여기서 자동 롤백
```

## RAII 시맨틱스

`Transaction` 클래스는 RAII 원칙을 따릅니다:

- **생성자**: `BEGIN` 실행
- **소멸자**: 커밋되지 않았다면 `ROLLBACK` 실행
- **commit()**: `COMMIT` 실행
- **rollback()**: `ROLLBACK` 실행 (선택사항, 소멸자가 자동으로 수행)

```cpp
void transferMoney(Connection& conn, int fromId, int toId, double amount) {
    pq::Transaction tx(conn);
    if (!tx) return;
    
    // 출금 계좌에서 차감
    conn.execute(
        "UPDATE accounts SET balance = balance - $1 WHERE id = $2",
        {std::to_string(amount), std::to_string(fromId)}
    );
    
    // 이 부분에서 예외가 발생하거나 실패하면 tx가 자동으로 롤백
    conn.execute(
        "UPDATE accounts SET balance = balance + $1 WHERE id = $2",
        {std::to_string(amount), std::to_string(toId)}
    );
    
    tx.commit();  // 두 작업 모두 성공해야 커밋
}
```

## Transaction API

### 생성자

```cpp
explicit Transaction(Connection& conn);
```

트랜잭션을 시작합니다. `isValid()` 또는 `operator bool()`로 성공 여부를 확인합니다.

### 메서드

| 메서드 | 반환 타입 | 설명 |
|--------|-----------|------|
| `commit()` | `DbResult<void>` | 트랜잭션 커밋 |
| `rollback()` | `DbResult<void>` | 트랜잭션 롤백 |
| `isValid()` | `bool` | BEGIN 성공 여부 확인 |
| `isCommitted()` | `bool` | 이미 커밋되었는지 확인 |
| `connection()` | `Connection&` | 기본 연결 가져오기 |

### 트랜잭션 상태 확인

```cpp
pq::Transaction tx(conn);

// 방법 1: bool 연산자
if (tx) {
    // 트랜잭션이 성공적으로 시작됨
}

// 방법 2: isValid()
if (tx.isValid()) {
    // 트랜잭션이 성공적으로 시작됨
}

// 커밋 여부 확인
if (tx.isCommitted()) {
    // 이미 커밋됨
}
```

## Repository와 함께 사용

```cpp
pq::Connection conn(...);
pq::Repository<User, int> userRepo(conn);
pq::Repository<Order, int> orderRepo(conn);

{
    pq::Transaction tx(conn);
    if (!tx) {
        throw std::runtime_error("트랜잭션 시작 실패");
    }
    
    // 사용자 생성
    User user;
    user.name = "John";
    auto savedUser = userRepo.save(user);
    if (!savedUser) {
        // 스코프 종료 시 자동 롤백
        throw std::runtime_error(savedUser.error().message);
    }
    
    // 사용자에 대한 주문 생성
    Order order;
    order.userId = savedUser->id;
    order.total = 99.99;
    auto savedOrder = orderRepo.save(order);
    if (!savedOrder) {
        // 스코프 종료 시 자동 롤백
        throw std::runtime_error(savedOrder.error().message);
    }
    
    // 둘 다 성공, 커밋
    tx.commit();
}
```

## Savepoint

중첩 트랜잭션과 유사한 동작을 위해 `Savepoint` 사용:

```cpp
pq::Connection conn(...);
pq::Transaction tx(conn);
if (!tx) return;

conn.execute("INSERT INTO users (name) VALUES ($1)", {"Alice"});

{
    pq::Savepoint sp(conn, "sp1");
    if (!sp) {
        // Savepoint 생성 실패
        return;
    }
    
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Bob"});
    
    // 이 부분을 유지할지 롤백할지 결정
    if (someCondition) {
        sp.release();   // 변경사항 유지
    } else {
        sp.rollbackTo();  // Savepoint로 롤백
    }
}
// release()가 호출되지 않았다면 소멸자가 rollbackTo() 호출

tx.commit();  // Alice 커밋 (release되었다면 Bob도 포함)
```

### Savepoint API

| 메서드 | 반환 타입 | 설명 |
|--------|-----------|------|
| `release()` | `DbResult<void>` | Savepoint 해제 (변경사항 유지) |
| `rollbackTo()` | `DbResult<void>` | 이 Savepoint로 롤백 |
| `isValid()` | `bool` | SAVEPOINT 성공 여부 확인 |

## 에러 핸들링

```cpp
pq::Transaction tx(conn);
if (!tx) {
    // BEGIN 실패 처리
    return;
}

// ... 작업 ...

auto commitResult = tx.commit();
if (!commitResult) {
    // 커밋 실패 - PostgreSQL이 이미 롤백했거나 연결 끊김
    std::cerr << "커밋 실패: " << commitResult.error().message << std::endl;
    std::cerr << "SQLSTATE: " << commitResult.error().sqlState << std::endl;
}
```

## 예외 안전성

트랜잭션은 설계상 예외에 안전합니다:

```cpp
void riskyOperation(Connection& conn) {
    pq::Transaction tx(conn);
    if (!tx) throw std::runtime_error("tx 시작 불가");
    
    conn.execute("INSERT INTO important_table VALUES (...)");
    
    // 여기서 예외가 발생하면...
    doSomethingThatMightThrow();
    
    // ...이 줄은 실행되지 않지만...
    tx.commit();
}
// ...소멸자는 호출되어 자동 롤백 수행
```

## 모범 사례

### 1. 항상 트랜잭션 유효성 확인

```cpp
pq::Transaction tx(conn);
if (!tx) {
    // 에러 처리 - 진행하지 않음
    return;
}
```

### 2. 트랜잭션은 짧게 유지

```cpp
// 좋음: 짧은 트랜잭션
{
    pq::Transaction tx(conn);
    repo.save(entity);
    tx.commit();
}
// 트랜잭션 해제, 연결 가용

// 나쁨: 장시간 트랜잭션
{
    pq::Transaction tx(conn);
    repo.save(entity);
    std::this_thread::sleep_for(std::chrono::minutes(5));  // 하지 마세요!
    tx.commit();
}
```

### 3. 트랜잭션 중첩 금지 (Savepoint 사용)

```cpp
// 잘못됨: 중첩 트랜잭션
{
    pq::Transaction tx1(conn);
    {
        pq::Transaction tx2(conn);  // 에러: 이미 트랜잭션 중
    }
}

// 올바름: Savepoint 사용
{
    pq::Transaction tx(conn);
    {
        pq::Savepoint sp(conn, "inner");
        // ...
        sp.release();
    }
    tx.commit();
}
```

### 4. 명시적 커밋 권장

```cpp
// 명시적 커밋으로 의도를 명확히
pq::Transaction tx(conn);
if (!tx) return;

// ... 작업 ...

tx.commit();  // 변경사항을 저장하겠다는 명확한 의도
```

## 연결 상태

트랜잭션 후:

```cpp
pq::Connection conn(...);

{
    pq::Transaction tx(conn);
    // ...
    tx.commit();
}

// 연결은 여전히 사용 가능
conn.execute("SELECT 1");

// 다른 트랜잭션 시작 가능
{
    pq::Transaction tx2(conn);
    // ...
}
```

## 다음 단계

- [커넥션 풀](connection-pool.md) - 트랜잭션과 풀 관리
- [에러 핸들링](error-handling.md) - 트랜잭션 에러 처리
