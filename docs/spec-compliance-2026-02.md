# Specification Compliance Status (2026-02-10)

This document tracks implementation compliance against `specs/002-orm-enhancements/spec.md`.

## Summary

- Target spec: `specs/002-orm-enhancements/spec.md` (Draft)
- Checked date: 2026-02-10
- Current delivery status: Composite PK + schema validation + extended type support implemented

## Functional Requirement Matrix

| Requirement | Status | Notes |
|-------------|--------|-------|
| FR-001 | Complete | Multiple `PQ_PRIMARY_KEY` columns supported |
| FR-002 | Complete | `EntityMetadata` stores PK indices list |
| FR-003 | Complete | `SqlBuilder` WHERE clauses include all PK columns with `AND` |
| FR-004 | Complete | Variadic `findById(args...)` with compile-time PK arity/type checks for tuple PK and runtime entity-PK shape validation |
| FR-005 | Complete | Single PK behavior remains backward compatible |
| FR-006 | Complete | Clear runtime error for composite PK misuse with scalar `PK` type |
| FR-007 | Complete | `SchemaValidator::validate<Entity>(conn)` manual validation API |
| FR-007a | Complete | `MapperConfig::autoValidateSchema` + `schemaValidationMode` first-use repository validation |
| FR-008 | Complete | Validation queries use `information_schema.tables/columns` |
| FR-009 | Complete | Validates table/column existence, type compatibility, nullable constraints |
| FR-010 | Complete | Reports all discovered issues in `ValidationResult` (errors + warnings) |
| FR-011 | Complete | Validation issues include entity/table/column and expected vs actual fields |
| FR-012 | Complete | Strict vs lenient validation mode support |
| FR-013 | Complete | `PQ_COLUMN_VARCHAR` + `maxLength` metadata and length validation |
| FR-014 | Complete | `timestamp` mapped to `std::chrono::system_clock::time_point` |
| FR-015 | Complete | `timestamptz` mapped via `TimestampTz` with timezone offset handling |
| FR-016 | Complete | `date` mapped via `Date` |
| FR-017 | Complete | `time` mapped via `Time` |
| FR-018 | Complete | `numeric` mapped via string-backed `Numeric` |
| FR-019 | Complete | `uuid` mapped via string-backed `Uuid` |
| FR-020 | Complete | `jsonb` mapped via string-backed `Jsonb` |
| FR-021 | Complete | Optional wrappers for all new types via `PgTypeTraits<std::optional<T>>` |

## Notes

- Unqualified table names in schema validation resolve against `current_schemas(true)` for better search_path compatibility.
- `SchemaValidator` strict mode keeps extra DB columns as warnings (per acceptance scenario wording).

## Test Coverage

- Unit: composite PK repository paths, schema validator disconnected behavior, extended type trait conversions, optional NULL handling
- Scenario: composite PK API behavior, schema validation API behavior, extended type trait scenario checks
- Integration: CRUD for single/composite PK, strict/lenient schema validation matrix, repository auto-validation, extended type DB roundtrip (`PQ_TEST_CONN_STR`)
- Performance: SQL builder regression guards + extended type conversion regression guard (`PQ_ENABLE_PERF_TESTS=1`)
