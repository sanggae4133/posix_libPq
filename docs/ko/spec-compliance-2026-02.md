# 스펙 준수 현황 (2026-02-10)

이 문서는 `specs/002-orm-enhancements/spec.md` 대비 구현 준수 상태를 기록합니다.

## 요약

- 기준 스펙: `specs/002-orm-enhancements/spec.md` (Draft)
- 점검 일자: 2026-02-10
- 현재 반영 상태: 복합 PK + 스키마 검증 + 확장 타입 지원 구현 완료

## 기능 요구사항(FR) 매트릭스

| 요구사항 | 상태 | 비고 |
|----------|------|------|
| FR-001 | 완료 | 하나의 Entity에 여러 `PQ_PRIMARY_KEY` 컬럼 지원 |
| FR-002 | 완료 | `EntityMetadata`가 PK 인덱스 목록 저장 |
| FR-003 | 완료 | `SqlBuilder`가 PK 전체를 `AND` 조건으로 생성 |
| FR-004 | 완료 | variadic `findById(args...)`에 tuple PK 개수/타입 정적 검증 + Entity PK 형태 런타임 검증 |
| FR-005 | 완료 | 단일 PK 하위 호환 유지 |
| FR-006 | 완료 | 복합 PK에서 scalar `PK` 타입 사용 시 명확한 런타임 에러 |
| FR-007 | 완료 | `SchemaValidator::validate<Entity>(conn)` 수동 검증 API |
| FR-007a | 완료 | `MapperConfig::autoValidateSchema` + `schemaValidationMode` 첫 호출 자동 검증 |
| FR-008 | 완료 | `information_schema.tables/columns` 기반 검증 조회 |
| FR-009 | 완료 | 테이블/컬럼 존재, 타입 호환성, nullable 제약 검증 |
| FR-010 | 완료 | `ValidationResult`에 모든 이슈(errors/warnings) 집계 |
| FR-011 | 완료 | Entity/테이블/컬럼, expected/actual 정보를 포함한 이슈 메시지 |
| FR-012 | 완료 | strict/lenient 검증 모드 지원 |
| FR-013 | 완료 | `PQ_COLUMN_VARCHAR` + `maxLength` 메타데이터 및 길이 검증 |
| FR-014 | 완료 | `timestamp` → `std::chrono::system_clock::time_point` |
| FR-015 | 완료 | `timestamptz` → `TimestampTz` (타임존 오프셋 처리) |
| FR-016 | 완료 | `date` → `Date` |
| FR-017 | 완료 | `time` → `Time` |
| FR-018 | 완료 | `numeric` → 문자열 기반 `Numeric` |
| FR-019 | 완료 | `uuid` → 문자열 기반 `Uuid` |
| FR-020 | 완료 | `jsonb` → 문자열 기반 `Jsonb` |
| FR-021 | 완료 | 모든 신규 타입에 대해 `PgTypeTraits<std::optional<T>>` NULL 처리 |

## 참고 사항

- 스키마 미지정 테이블명은 `current_schemas(true)` 기준으로 해석하여 search_path 환경 호환성을 높였습니다.
- strict 모드에서도 DB extra 컬럼은 경고(warning)로 분류합니다(수용 시나리오 기준).

## 테스트 커버리지

- 단위(Unit): 복합 PK repository 경로, 스키마 검증(미연결), 확장 타입 변환, optional NULL 처리
- 시나리오(Scenario): 복합 PK API, 스키마 검증 API, 확장 타입 트레이트 시나리오 검증
- 통합(Integration): 단일/복합 PK CRUD, strict/lenient 스키마 검증 매트릭스, repository 자동 검증, 확장 타입 DB 라운드트립 (`PQ_TEST_CONN_STR`)
- 성능(Perf): SQL builder 회귀 가드 + 확장 타입 변환 회귀 가드 (`PQ_ENABLE_PERF_TESTS=1`)
