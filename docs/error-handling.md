# Error Handling

PosixLibPq uses a `Result<T, E>` type for error handling, providing a type-safe alternative to exceptions that is similar to Rust's `Result` or C++23's `std::expected`.

## The Result Type

```cpp
template<typename T, typename E = DbError>
class Result;

// Convenient alias
template<typename T>
using DbResult = Result<T, DbError>;
```

Every database operation returns a `DbResult<T>`:

```cpp
DbResult<QueryResult> execute(std::string_view sql);
DbResult<Entity> save(const Entity& entity);
DbResult<std::optional<Entity>> findById(const PK& id);
DbResult<void> commit();
```

## Basic Usage

### Checking Success

```cpp
auto result = conn.execute("SELECT * FROM users");

// Method 1: Boolean conversion
if (result) {
    // Success
    QueryResult& qr = *result;
}

// Method 2: hasValue() / hasError()
if (result.hasValue()) {
    // Success
}
if (result.hasError()) {
    // Failure
}
```

### Accessing Values

```cpp
auto result = conn.execute("SELECT * FROM users");

if (result) {
    // Dereference to get value
    QueryResult& value = *result;
    
    // Or use value()
    QueryResult& value2 = result.value();
    
    // Arrow operator for member access
    int rows = result->rowCount();
}
```

### Accessing Errors

```cpp
auto result = conn.execute("INVALID SQL");

if (!result) {
    DbError& err = result.error();
    
    std::cerr << "Message: " << err.message << std::endl;
    std::cerr << "SQLSTATE: " << err.sqlState << std::endl;
    std::cerr << "Code: " << err.errorCode << std::endl;
}
```

## DbError Structure

```cpp
struct DbError {
    std::string message;    // Human-readable error message
    std::string sqlState;   // PostgreSQL SQLSTATE code (5 chars)
    int errorCode{0};       // Additional error code
    
    const char* what() const noexcept;  // Returns message.c_str()
};
```

### Common SQLSTATE Codes

| SQLSTATE | Description |
|----------|-------------|
| `23505` | Unique violation |
| `23503` | Foreign key violation |
| `23502` | Not null violation |
| `42P01` | Undefined table |
| `42703` | Undefined column |
| `08001` | Unable to connect |
| `08006` | Connection failure |
| `25P02` | Transaction aborted |

## Value-Or Pattern

Use `valueOr()` to provide a default value:

```cpp
// Returns value if success, default if error
auto users = userRepo.findAll().valueOr(std::vector<User>{});

int count = countResult.valueOr(0);
std::string name = nameResult.valueOr("Unknown");
```

## Map/Transform

Transform success values while preserving errors:

```cpp
auto result = userRepo.findById(1);

// Map transforms the success value
auto nameResult = result.map([](const std::optional<User>& opt) {
    return opt ? opt->name : "Not found";
});

if (nameResult) {
    std::cout << "Name: " << *nameResult << std::endl;
}

// Chain maps
auto lengthResult = result
    .map([](const auto& opt) { return opt ? opt->name : ""; })
    .map([](const std::string& s) { return s.length(); });
```

## Creating Results

### Success Results

```cpp
// Implicit construction
DbResult<int> success = 42;

// Factory method
auto result = DbResult<int>::ok(42);

// Void result
auto voidResult = DbResult<void>::ok();
```

### Error Results

```cpp
// Factory method with DbError
auto error = DbResult<int>::error(DbError{"Something went wrong", "00000", 1});

// Short form with just message
auto error2 = DbResult<int>::error(DbError{"Failed"});

// In-place construction
Result<int, DbError> error3(inPlaceError, "Error message", "42P01", 0);
```

## Pattern: Early Return

```cpp
DbResult<User> createUserWithProfile(const std::string& name) {
    // Each step can fail
    auto userResult = userRepo.save(User{0, name});
    if (!userResult) {
        return DbResult<User>::error(userResult.error());
    }
    
    auto profileResult = profileRepo.save(Profile{0, userResult->id});
    if (!profileResult) {
        return DbResult<User>::error(profileResult.error());
    }
    
    return *userResult;
}
```

## Pattern: Error Propagation

```cpp
DbResult<void> complexOperation() {
    // Execute multiple steps, propagate first error
    
    auto result1 = step1();
    if (!result1) return DbResult<void>::error(result1.error());
    
    auto result2 = step2(*result1);
    if (!result2) return DbResult<void>::error(result2.error());
    
    auto result3 = step3(*result2);
    if (!result3) return DbResult<void>::error(result3.error());
    
    return DbResult<void>::ok();
}
```

## Pattern: Collect Errors

```cpp
std::vector<DbError> errors;

for (const auto& user : users) {
    auto result = userRepo.save(user);
    if (!result) {
        errors.push_back(result.error());
    }
}

if (!errors.empty()) {
    // Handle collected errors
}
```

## Void Results

For operations that don't return a value:

```cpp
DbResult<void> commit();
DbResult<void> connect(std::string_view connStr);

auto result = tx.commit();
if (result) {
    // Success (no value to access)
    std::cout << "Committed!" << std::endl;
} else {
    std::cerr << "Failed: " << result.error().message << std::endl;
}
```

## Exception Integration

While PosixLibPq uses `Result` types, you can convert to exceptions if needed:

```cpp
// Throw on error
auto result = userRepo.findById(id);
if (!result) {
    throw std::runtime_error(result.error().message);
}

// Or create a helper
template<typename T>
T unwrap(DbResult<T> result) {
    if (!result) {
        throw std::runtime_error(result.error().message);
    }
    return std::move(*result);
}

// Usage
auto user = unwrap(userRepo.findById(id));
```

## Best Practices

### 1. Always Check Results

```cpp
// Bad: Ignoring result
conn.execute("INSERT INTO users (name) VALUES ('test')");

// Good: Check result
auto result = conn.execute("INSERT INTO users (name) VALUES ('test')");
if (!result) {
    log_error(result.error().message);
}
```

### 2. Handle Both Success and Error Cases

```cpp
auto result = userRepo.findById(id);
if (result) {
    if (*result) {
        // Found
        processUser(**result);
    } else {
        // Not found (but not an error)
        return NotFound();
    }
} else {
    // Database error
    return ServerError(result.error().message);
}
```

### 3. Log Errors with Context

```cpp
auto result = userRepo.save(user);
if (!result) {
    const auto& err = result.error();
    logger.error("Failed to save user '{}': {} (SQLSTATE: {})",
                 user.name, err.message, err.sqlState);
}
```

### 4. Use Appropriate Error Messages

```cpp
// Provide context in error messages
if (!result) {
    // Add context before returning
    return DbResult<Order>::error(DbError{
        "Failed to create order for user " + std::to_string(userId) + ": " + result.error().message,
        result.error().sqlState,
        result.error().errorCode
    });
}
```

### 5. Don't Use value() Without Checking

```cpp
// Dangerous: Throws if error
auto value = result.value();  // May throw!

// Safe: Check first
if (result) {
    auto value = result.value();  // Safe
}

// Or use valueOr
auto value = result.valueOr(defaultValue);
```

## API Reference

### Result<T, E> Methods

| Method | Return | Description |
|--------|--------|-------------|
| `hasValue()` | `bool` | True if contains value |
| `hasError()` | `bool` | True if contains error |
| `operator bool()` | `bool` | Same as `hasValue()` |
| `value()` | `T&` | Get value (throws if error) |
| `error()` | `E&` | Get error (throws if value) |
| `valueOr(default)` | `T` | Get value or default |
| `operator*()` | `T&` | Same as `value()` |
| `operator->()` | `T*` | Pointer to value |
| `map(f)` | `Result<U, E>` | Transform value |

### Static Factory Methods

| Method | Description |
|--------|-------------|
| `Result::ok(value)` | Create success result |
| `Result::error(err)` | Create error result |

## Next Steps

- [Repository Pattern](repository-pattern.md) - Repository error handling
- [Transactions](transactions.md) - Transaction error handling
