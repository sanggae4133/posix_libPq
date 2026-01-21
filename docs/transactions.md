# Transactions

PosixLibPq provides RAII-based transaction management that ensures data integrity even when exceptions occur.

## Basic Usage

```cpp
#include <pq/pq.hpp>

pq::Connection conn("host=localhost dbname=mydb");

{
    pq::Transaction tx(conn);
    
    if (!tx) {
        // BEGIN failed
        std::cerr << "Failed to start transaction" << std::endl;
        return;
    }
    
    // Perform operations...
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Alice"});
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Bob"});
    
    // Explicit commit
    auto result = tx.commit();
    if (!result) {
        std::cerr << "Commit failed: " << result.error().message << std::endl;
    }
}
// If commit() was not called, automatic rollback occurs here
```

## RAII Semantics

The `Transaction` class follows RAII principles:

- **Constructor**: Executes `BEGIN`
- **Destructor**: If not committed, executes `ROLLBACK`
- **commit()**: Executes `COMMIT`
- **rollback()**: Executes `ROLLBACK` (optional, destructor does this automatically)

```cpp
void transferMoney(Connection& conn, int fromId, int toId, double amount) {
    pq::Transaction tx(conn);
    if (!tx) return;
    
    // Deduct from source account
    conn.execute(
        "UPDATE accounts SET balance = balance - $1 WHERE id = $2",
        {std::to_string(amount), std::to_string(fromId)}
    );
    
    // If this throws or fails, tx will rollback automatically
    conn.execute(
        "UPDATE accounts SET balance = balance + $1 WHERE id = $2",
        {std::to_string(amount), std::to_string(toId)}
    );
    
    tx.commit();  // Only commits if both operations succeeded
}
```

## Transaction API

### Constructor

```cpp
explicit Transaction(Connection& conn);
```

Begins a transaction. Check `isValid()` or `operator bool()` to verify success.

### Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `commit()` | `DbResult<void>` | Commit the transaction |
| `rollback()` | `DbResult<void>` | Rollback the transaction |
| `isValid()` | `bool` | Check if BEGIN succeeded |
| `isCommitted()` | `bool` | Check if already committed |
| `connection()` | `Connection&` | Get the underlying connection |

### Checking Transaction State

```cpp
pq::Transaction tx(conn);

// Method 1: bool operator
if (tx) {
    // Transaction started successfully
}

// Method 2: isValid()
if (tx.isValid()) {
    // Transaction started successfully
}

// Check if committed
if (tx.isCommitted()) {
    // Already committed
}
```

## With Repository

```cpp
pq::Connection conn(...);
pq::Repository<User, int> userRepo(conn);
pq::Repository<Order, int> orderRepo(conn);

{
    pq::Transaction tx(conn);
    if (!tx) {
        throw std::runtime_error("Failed to start transaction");
    }
    
    // Create user
    User user;
    user.name = "John";
    auto savedUser = userRepo.save(user);
    if (!savedUser) {
        // Automatic rollback on scope exit
        throw std::runtime_error(savedUser.error().message);
    }
    
    // Create order for user
    Order order;
    order.userId = savedUser->id;
    order.total = 99.99;
    auto savedOrder = orderRepo.save(order);
    if (!savedOrder) {
        // Automatic rollback on scope exit
        throw std::runtime_error(savedOrder.error().message);
    }
    
    // Both succeeded, commit
    tx.commit();
}
```

## Savepoints

For nested transaction-like behavior, use `Savepoint`:

```cpp
pq::Connection conn(...);
pq::Transaction tx(conn);
if (!tx) return;

conn.execute("INSERT INTO users (name) VALUES ($1)", {"Alice"});

{
    pq::Savepoint sp(conn, "sp1");
    if (!sp) {
        // Savepoint creation failed
        return;
    }
    
    conn.execute("INSERT INTO users (name) VALUES ($1)", {"Bob"});
    
    // Decide whether to keep or rollback this part
    if (someCondition) {
        sp.release();   // Keep the changes
    } else {
        sp.rollbackTo();  // Undo back to savepoint
    }
}
// Destructor calls rollbackTo() if release() wasn't called

tx.commit();  // Commits Alice (and Bob if released)
```

### Savepoint API

| Method | Return Type | Description |
|--------|-------------|-------------|
| `release()` | `DbResult<void>` | Release savepoint (keep changes) |
| `rollbackTo()` | `DbResult<void>` | Rollback to this savepoint |
| `isValid()` | `bool` | Check if SAVEPOINT succeeded |

## Error Handling

```cpp
pq::Transaction tx(conn);
if (!tx) {
    // Handle BEGIN failure
    return;
}

// ... operations ...

auto commitResult = tx.commit();
if (!commitResult) {
    // Commit failed - transaction already rolled back by PostgreSQL
    // or connection lost
    std::cerr << "Commit failed: " << commitResult.error().message << std::endl;
    std::cerr << "SQLSTATE: " << commitResult.error().sqlState << std::endl;
}
```

## Exception Safety

Transactions are exception-safe by design:

```cpp
void riskyOperation(Connection& conn) {
    pq::Transaction tx(conn);
    if (!tx) throw std::runtime_error("Cannot start tx");
    
    conn.execute("INSERT INTO important_table VALUES (...)");
    
    // If this throws an exception...
    doSomethingThatMightThrow();
    
    // ...this line is never reached...
    tx.commit();
}
// ...but the destructor IS called, triggering automatic rollback
```

## Best Practices

### 1. Always Check Transaction Validity

```cpp
pq::Transaction tx(conn);
if (!tx) {
    // Handle error - don't proceed
    return;
}
```

### 2. Keep Transactions Short

```cpp
// Good: Short transaction
{
    pq::Transaction tx(conn);
    repo.save(entity);
    tx.commit();
}
// Transaction released, connection available

// Bad: Long-running transaction
{
    pq::Transaction tx(conn);
    repo.save(entity);
    std::this_thread::sleep_for(std::chrono::minutes(5));  // Don't do this!
    tx.commit();
}
```

### 3. Don't Nest Transactions (Use Savepoints)

```cpp
// Wrong: Nested transactions
{
    pq::Transaction tx1(conn);
    {
        pq::Transaction tx2(conn);  // ERROR: Already in transaction
    }
}

// Correct: Use savepoints
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

### 4. Explicit Commit is Recommended

```cpp
// Explicit commit makes intent clear
pq::Transaction tx(conn);
if (!tx) return;

// ... operations ...

tx.commit();  // Clear intent to save changes
```

## Connection State

After a transaction:

```cpp
pq::Connection conn(...);

{
    pq::Transaction tx(conn);
    // ...
    tx.commit();
}

// Connection is still usable
conn.execute("SELECT 1");

// Can start another transaction
{
    pq::Transaction tx2(conn);
    // ...
}
```

## Next Steps

- [Connection Pool](connection-pool.md) - Pool management with transactions
- [Error Handling](error-handling.md) - Handling transaction errors
