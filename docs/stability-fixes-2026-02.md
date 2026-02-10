# Stability Fixes (2026-02)

This document summarizes stability and correctness fixes applied in February 2026.

## What Changed

### 1) Connection pool lifetime and concurrency safety

- `PooledConnection` now holds shared pool state rather than a raw `ConnectionPool*`.
- Releasing a leased connection after `ConnectionPool` object destruction is now safe.
- `acquire()` now reserves create slots to enforce `maxSize` under concurrency.
- Idle connection validation no longer runs while the pool mutex is held.

## 2) NULL vs empty-string parameter handling

- `Connection::execute(sql, const std::vector<std::string>&)` now preserves empty strings.
- New overload: `Connection::execute(sql, const std::vector<std::optional<std::string>>&)` for explicit NULL parameters.
- ORM SQL builders (`insertParams`, `updateParams`) now return `std::vector<std::optional<std::string>>`.

### 3) Transaction state consistency

- `Connection::commit()` and `Connection::rollback()` now update `inTransaction_` only after SQL succeeds.

### 4) Thread-safe entity metadata initialization

- Entity metadata registration uses `std::call_once` instead of a non-atomic static boolean.

### 5) Repository improvements

- Primary key conversion no longer assumes numeric IDs via `std::to_string`.
- `Repository<Entity, PK>` now supports string-like PK types (`std::string`, `std::string_view`, `const char*`) and trait-based types.
- Runtime updates through `repo.config()` are now synchronized to the internal mapper.

## Migration Notes

1. If you want SQL `NULL` parameters, use:

```cpp
std::vector<std::optional<std::string>> params = {
    std::string{"Alice"},
    std::nullopt
};
conn.execute("INSERT INTO users(name, nickname) VALUES($1, $2)", params);
```

2. If you pass `std::vector<std::string>`, empty string (`""`) is now sent as empty string, not `NULL`.

3. If you use `SqlBuilder<Entity>::insertParams` or `updateParams` directly, adapt your code to `std::optional<std::string>` elements.

## Added/Updated Tests

- `tests/unit/test_repository.cpp` added for PK template coverage with string-like types.
- `tests/unit/test_mapper.cpp` updated for optional parameter vectors and empty-string preservation.
- Existing unit tests continue to pass.
