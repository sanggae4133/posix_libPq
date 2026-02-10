# 안정성 패치 노트 (2026-02)

이 문서는 2026년 2월에 적용된 안정성/정확성 개선 사항을 요약합니다.

## 변경 사항

### 1) 커넥션 풀 수명 및 동시성 안정성

- `PooledConnection`이 더 이상 raw `ConnectionPool*`를 보관하지 않고, 공유 상태를 참조합니다.
- `ConnectionPool` 객체가 먼저 소멸된 뒤 대여 커넥션이 반환되어도 안전하게 처리됩니다.
- `acquire()`에서 생성 슬롯을 예약하여 동시 접근 시 `maxSize` 제한을 보장합니다.
- 유휴 커넥션 검증 쿼리는 풀 뮤텍스를 오래 잡지 않도록 조정했습니다.

### 2) NULL 과 빈 문자열 처리 분리

- `Connection::execute(sql, const std::vector<std::string>&)`는 이제 빈 문자열을 그대로 빈 문자열로 전달합니다.
- 명시적 NULL 전달용 오버로드 추가:
  `Connection::execute(sql, const std::vector<std::optional<std::string>>&)`
- ORM SQL 빌더(`insertParams`, `updateParams`) 반환 타입을 `std::vector<std::optional<std::string>>`로 변경했습니다.

### 3) 트랜잭션 상태 일관성

- `Connection::commit()`/`rollback()`에서 SQL 성공 이후에만 `inTransaction_` 상태를 false로 바꿉니다.

### 4) Entity 메타데이터 초기화 스레드 안전성

- 비원자 static bool 대신 `std::call_once`로 메타데이터 초기화를 수행합니다.

### 5) Repository 개선

- PK 변환이 더 이상 `std::to_string` 숫자 가정에 묶이지 않습니다.
- `Repository<Entity, PK>`에서 문자열 계열 PK(`std::string`, `std::string_view`, `const char*`)를 지원합니다.
- `repo.config()`로 설정 변경 시 내부 mapper 설정이 동기화되도록 수정했습니다.

## 마이그레이션 가이드

1. SQL `NULL` 파라미터가 필요하면 다음 형식을 사용하세요.

```cpp
std::vector<std::optional<std::string>> params = {
    std::string{"Alice"},
    std::nullopt
};
conn.execute("INSERT INTO users(name, nickname) VALUES($1, $2)", params);
```

2. `std::vector<std::string>`를 전달할 경우 `""`는 `NULL`이 아니라 빈 문자열로 저장됩니다.

3. `SqlBuilder<Entity>::insertParams`/`updateParams`를 직접 쓰는 코드는 원소 타입이 `std::optional<std::string>`로 바뀐 점을 반영해야 합니다.

## 테스트 반영

- `tests/unit/test_repository.cpp` 추가: 문자열 계열 PK 템플릿 동작 커버.
- `tests/unit/test_mapper.cpp` 갱신: optional 파라미터 벡터/빈 문자열 보존 검증.
- 기존 단위 테스트와 함께 전체 테스트 통과를 확인했습니다.
