# PosixLibPq 문서

PosixLibPq 문서에 오신 것을 환영합니다. 이 라이브러리는 RAII 시맨틱스, 타입 안전성, Zero-Cost 추상화를 갖춘 현대적인 C++17 PostgreSQL ORM을 제공합니다.

## 문서 목차

### 시작하기
- [**시작하기**](getting-started.md) - 설치, 빌드, 빠른 시작 가이드

### 핵심 개념
- [**Entity 매핑**](entity-mapping.md) - Entity를 PostgreSQL 테이블에 매핑하는 방법
- [**Repository 패턴**](repository-pattern.md) - Repository 패턴을 사용한 CRUD 작업
- [**에러 핸들링**](error-handling.md) - Result 타입과 에러 관리

### 고급 기능
- [**트랜잭션**](transactions.md) - RAII 기반 트랜잭션 관리
- [**커넥션 풀**](connection-pool.md) - 고성능 애플리케이션을 위한 커넥션 풀링
- [**커스텀 쿼리**](custom-queries.md) - 커스텀 SQL 쿼리 실행

### 레퍼런스
- [**API 레퍼런스**](api-reference.md) - 전체 API 문서
- [**타입 시스템**](type-system.md) - C++에서 PostgreSQL로의 타입 매핑
- [**안정성 패치 노트 (2026-02)**](stability-fixes-2026-02.md) - 동시성/정확성 개선 내역
- [**스펙 준수 현황 (2026-02)**](spec-compliance-2026-02.md) - 개선 스펙 대비 구현 상태

## 빠른 예제

```cpp
#include <pq/pq.hpp>

// Entity 정의
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

// 사용하기
pq::Connection conn("host=localhost dbname=mydb user=postgres");
pq::Repository<User, int> userRepo(conn);

User user;
user.name = "John";
auto saved = userRepo.save(user);
```

## 프로젝트 링크

- [GitHub 저장소](https://github.com/your-repo/posixLibPq)
- [이슈 트래커](https://github.com/your-repo/posixLibPq/issues)
- [라이선스 (MIT)](../../LICENSE)
- [영문 문서](../README.md)
