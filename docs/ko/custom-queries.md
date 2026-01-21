# 커스텀 쿼리

Repository 패턴이 표준 CRUD 작업을 처리하지만, 복잡한 비즈니스 로직에는 커스텀 쿼리가 필요한 경우가 많습니다.

## Repository의 executeQuery 사용

```cpp
pq::Repository<User, int> userRepo(conn);

// 커스텀 쿼리 실행, 결과를 Entity로 매핑
auto result = userRepo.executeQuery(
    "SELECT * FROM users WHERE email LIKE $1 ORDER BY name",
    {"%@example.com"}
);

if (result) {
    for (const auto& user : *result) {
        std::cout << user.name << std::endl;
    }
}
```

## executeQuery vs executeQueryOne

```cpp
// 0개 이상의 Entity 벡터 반환
DbResult<std::vector<User>> executeQuery(
    std::string_view sql,
    const std::vector<std::string>& params = {}
);

// 0개 또는 1개의 optional Entity 반환
DbResult<std::optional<User>> executeQueryOne(
    std::string_view sql,
    const std::vector<std::string>& params = {}
);
```

### 예시: 단일 결과

```cpp
auto result = userRepo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    {"john@example.com"}
);

if (result) {
    if (*result) {
        User& user = **result;
        std::cout << "찾음: " << user.name << std::endl;
    } else {
        std::cout << "사용자를 찾을 수 없음" << std::endl;
    }
}
```

## 직접 Connection 쿼리

Entity에 매핑되지 않는 쿼리의 경우 `Connection::execute`를 직접 사용:

```cpp
pq::Connection conn(...);

// 단순 쿼리
auto result = conn.execute("SELECT COUNT(*) FROM users");
if (result) {
    int count = result->row(0).get<int>(0);
    std::cout << "개수: " << count << std::endl;
}

// 파라미터화된 쿼리
auto result = conn.execute(
    "SELECT name, email FROM users WHERE id > $1 AND active = $2",
    {"100", "t"}
);

if (result) {
    for (const auto& row : *result) {
        std::cout << row.get<std::string>(0) << ": " 
                  << row.get<std::optional<std::string>>(1).value_or("N/A") 
                  << std::endl;
    }
}
```

## 파라미터 바인딩

파라미터는 `$1`, `$2` 등의 플레이스홀더 사용:

```cpp
// 문자열 파라미터
conn.execute(
    "SELECT * FROM users WHERE name = $1 AND city = $2",
    {"John", "New York"}
);

// 숫자 파라미터를 문자열로 변환
conn.execute(
    "SELECT * FROM users WHERE age > $1 AND age < $2",
    {std::to_string(18), std::to_string(65)}
);

// NULL 파라미터 - 빈 문자열 사용
conn.execute(
    "INSERT INTO users (name, email) VALUES ($1, $2)",
    {"John", ""}  // email은 NULL이 됨
);
```

## 타입 안전 파라미터 바인딩

타입 안전 파라미터 바인딩에는 `executeParams` 사용:

```cpp
// 타입에 따라 자동으로 변환
auto result = conn.executeParams(
    "SELECT * FROM users WHERE id = $1 AND active = $2",
    42,        // int -> "$1"
    true       // bool -> "t"
);

// optional (NULL 처리)
std::optional<std::string> maybeEmail = std::nullopt;
conn.executeParams(
    "INSERT INTO users (name, email) VALUES ($1, $2)",
    std::string("John"),
    maybeEmail  // NULL이 됨
);
```

## QueryResult 다루기

```cpp
auto result = conn.execute("SELECT id, name, email, age FROM users");

if (result) {
    QueryResult& qr = *result;
    
    // 메타데이터
    std::cout << "행 수: " << qr.rowCount() << std::endl;
    std::cout << "컬럼 수: " << qr.columnCount() << std::endl;
    
    // 컬럼 이름
    for (const auto& name : qr.columnNames()) {
        std::cout << "컬럼: " << name << std::endl;
    }
    
    // 행 순회
    for (int i = 0; i < qr.rowCount(); ++i) {
        Row row = qr.row(i);
        
        // 인덱스로
        int id = row.get<int>(0);
        std::string name = row.get<std::string>(1);
        
        // 컬럼 이름으로
        auto email = row.get<std::optional<std::string>>("email");
        auto age = row.get<std::optional<int>>("age");
        
        // NULL 확인
        if (row.isNull(2)) {
            std::cout << "Email은 NULL" << std::endl;
        }
    }
    
    // 범위 기반 for 루프
    for (const auto& row : qr) {
        std::cout << row.get<std::string>("name") << std::endl;
    }
    
    // 첫 번째 행 (있는 경우)
    if (auto first = qr.first()) {
        std::cout << "첫 번째: " << first->get<std::string>("name") << std::endl;
    }
}
```

## 집계 쿼리

```cpp
// COUNT
auto result = conn.execute("SELECT COUNT(*) FROM users");
if (result && !result->empty()) {
    int64_t count = result->row(0).get<int64_t>(0);
}

// SUM, AVG
auto result = conn.execute("SELECT SUM(price), AVG(price) FROM orders");
if (result && !result->empty()) {
    double sum = result->row(0).get<double>(0);
    double avg = result->row(0).get<double>(1);
}

// GROUP BY
auto result = conn.execute(
    "SELECT category, COUNT(*) as cnt FROM products GROUP BY category"
);
if (result) {
    for (const auto& row : *result) {
        std::cout << row.get<std::string>("category") 
                  << ": " << row.get<int64_t>("cnt") << std::endl;
    }
}
```

## JOIN 쿼리

JOIN 쿼리의 결과는 단일 Entity에 직접 매핑되지 않을 수 있습니다:

```cpp
// 직접 쿼리 사용
auto result = conn.execute(R"(
    SELECT 
        u.id as user_id,
        u.name as user_name,
        o.id as order_id,
        o.total
    FROM users u
    JOIN orders o ON u.id = o.user_id
    WHERE u.id = $1
)", {"1"});

if (result) {
    for (const auto& row : *result) {
        std::cout << "사용자: " << row.get<std::string>("user_name")
                  << " 주문: " << row.get<int>("order_id")
                  << " 총액: " << row.get<double>("total")
                  << std::endl;
    }
}
```

## RETURNING을 사용한 INSERT

```cpp
auto result = conn.execute(
    "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id, created_at",
    {"John", "john@example.com"}
);

if (result && !result->empty()) {
    int id = result->row(0).get<int>("id");
    std::string createdAt = result->row(0).get<std::string>("created_at");
    std::cout << "사용자 " << id << " 생성, 시간: " << createdAt << std::endl;
}
```

## 영향 받은 행 수가 있는 UPDATE/DELETE

```cpp
// executeUpdate로 영향 받은 행 수 얻기
auto affected = conn.executeUpdate(
    "UPDATE users SET active = $1 WHERE last_login < $2",
    {"f", "2024-01-01"}
);

if (affected) {
    std::cout << *affected << "명의 사용자 비활성화됨" << std::endl;
}

// 또는 QueryResult에서 affectedRows() 확인
auto result = conn.execute("DELETE FROM sessions WHERE expired = true");
if (result) {
    std::cout << result->affectedRows() << "개의 세션 삭제됨" << std::endl;
}
```

## Prepared Statement

자주 실행되는 쿼리에는 prepared statement 사용:

```cpp
// 한 번 준비
auto prepResult = conn.prepare("find_user_by_email", 
    "SELECT * FROM users WHERE email = $1");
if (!prepResult) {
    // 에러 처리
}

// 여러 번 실행
auto result1 = conn.executePrepared("find_user_by_email", {"alice@example.com"});
auto result2 = conn.executePrepared("find_user_by_email", {"bob@example.com"});
```

## 커스텀 쿼리와 트랜잭션

```cpp
pq::Transaction tx(conn);
if (!tx) return;

conn.execute("UPDATE accounts SET balance = balance - $1 WHERE id = $2",
             {std::to_string(amount), std::to_string(fromId)});

conn.execute("UPDATE accounts SET balance = balance + $1 WHERE id = $2",
             {std::to_string(amount), std::to_string(toId)});

conn.execute("INSERT INTO transfers (from_id, to_id, amount) VALUES ($1, $2, $3)",
             {std::to_string(fromId), std::to_string(toId), std::to_string(amount)});

tx.commit();
```

## 에러 핸들링

```cpp
auto result = conn.execute("SELECT * FROM nonexistent_table");

if (!result) {
    const DbError& err = result.error();
    
    // SQLSTATE로 특정 에러 확인
    if (err.sqlState == "42P01") {
        std::cerr << "테이블이 존재하지 않음" << std::endl;
    } else {
        std::cerr << "쿼리 실패: " << err.message << std::endl;
    }
}
```

## 모범 사례

### 1. 파라미터화된 쿼리 사용 (항상!)

```cpp
// 절대 이렇게 하지 마세요 - SQL 인젝션 위험!
conn.execute("SELECT * FROM users WHERE name = '" + userInput + "'");

// 항상 파라미터 사용
conn.execute("SELECT * FROM users WHERE name = $1", {userInput});
```

### 2. 빈 결과 확인

```cpp
auto result = conn.execute("SELECT * FROM users WHERE id = $1", {id});
if (result) {
    if (result->empty()) {
        // 찾지 못함 처리
    } else {
        // 결과 처리
    }
}
```

### 3. 적절한 타입 사용

```cpp
// C++ 타입을 PostgreSQL 타입에 맞추기
row.get<int32_t>("integer_column");
row.get<int64_t>("bigint_column");
row.get<double>("double_column");
row.get<std::string>("text_column");
row.get<bool>("boolean_column");
row.get<std::optional<std::string>>("nullable_column");
```

### 4. 쿼리 성능 고려

```cpp
// 대량 결과에는 LIMIT 추가
conn.execute("SELECT * FROM logs ORDER BY timestamp DESC LIMIT 100");

// 인덱스 사용 (쿼리되는 컬럼에 적절한 인덱싱 확인)
conn.execute("SELECT * FROM users WHERE email = $1", {email});  // email에 인덱스 필요
```

## 다음 단계

- [에러 핸들링](error-handling.md) - 쿼리 에러 핸들링
- [트랜잭션](transactions.md) - 커스텀 쿼리와 트랜잭션
