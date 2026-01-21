# Connection Pool

PosixLibPq provides a thread-safe connection pool for efficient database connection management in multi-threaded applications.

## Basic Usage

```cpp
#include <pq/pq.hpp>

// Configure the pool
pq::PoolConfig config;
config.connectionString = "host=localhost dbname=mydb user=postgres";
config.minSize = 2;   // Minimum idle connections
config.maxSize = 10;  // Maximum total connections

// Create the pool
pq::ConnectionPool pool(config);

// Acquire a connection
auto connResult = pool.acquire();
if (connResult) {
    pq::PooledConnection& conn = *connResult;
    
    // Use the connection
    conn->execute("SELECT * FROM users");
    
}  // Connection automatically returned to pool
```

## Pool Configuration

```cpp
struct PoolConfig {
    std::string connectionString;           // Required: connection string
    size_t maxSize = 10;                    // Maximum connections
    size_t minSize = 1;                     // Minimum idle connections
    std::chrono::milliseconds acquireTimeout{5000};   // Timeout for acquire()
    std::chrono::milliseconds idleTimeout{60000};     // Idle validation timeout
    bool validateOnAcquire = true;          // Validate before returning
};
```

| Option | Default | Description |
|--------|---------|-------------|
| `connectionString` | (required) | PostgreSQL connection string |
| `maxSize` | 10 | Maximum number of connections |
| `minSize` | 1 | Minimum idle connections to maintain |
| `acquireTimeout` | 5000ms | How long to wait for a connection |
| `idleTimeout` | 60000ms | How long before validating idle connections |
| `validateOnAcquire` | true | Test connection before returning |

## PooledConnection

`PooledConnection` is an RAII wrapper that automatically returns the connection to the pool when destroyed.

```cpp
auto connResult = pool.acquire();
if (connResult) {
    pq::PooledConnection conn = std::move(*connResult);
    
    // Access underlying connection with -> or *
    conn->execute("SELECT 1");
    (*conn).execute("SELECT 2");
    
    // Check validity
    if (conn.isValid()) {
        // Connection is good
    }
    
    // Manual release (optional - destructor does this)
    conn.release();
}
```

### PooledConnection API

| Method | Description |
|--------|-------------|
| `operator*()` | Access underlying `Connection` |
| `operator->()` | Access underlying `Connection` members |
| `isValid()` | Check if connection is still valid |
| `release()` | Return connection to pool manually |

## Pool Statistics

```cpp
pq::ConnectionPool pool(config);

std::cout << "Idle: " << pool.idleCount() << std::endl;
std::cout << "Active: " << pool.activeCount() << std::endl;
std::cout << "Total: " << pool.totalCount() << std::endl;
std::cout << "Max: " << pool.maxSize() << std::endl;
```

| Method | Description |
|--------|-------------|
| `idleCount()` | Number of idle connections in pool |
| `activeCount()` | Number of connections currently in use |
| `totalCount()` | Total connections (idle + active) |
| `maxSize()` | Maximum configured pool size |

## Pool Management

```cpp
pq::ConnectionPool pool(config);

// Close all idle connections
pool.drain();

// Shutdown pool - all connections closed
pool.shutdown();
// After shutdown, acquire() will fail
```

## Acquire with Custom Timeout

```cpp
// Use default timeout from config
auto conn1 = pool.acquire();

// Use custom timeout
auto conn2 = pool.acquire(std::chrono::milliseconds(1000));

if (!conn2) {
    // Timeout or error
    std::cerr << "Failed to acquire: " << conn2.error().message << std::endl;
}
```

## Thread Safety

`ConnectionPool` is fully thread-safe:

```cpp
pq::ConnectionPool pool(config);

// Can be used from multiple threads
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

## With Repository

```cpp
pq::ConnectionPool pool(config);

void handleRequest() {
    auto connResult = pool.acquire();
    if (!connResult) {
        throw std::runtime_error("No connection available");
    }
    
    // Create repository with pooled connection
    pq::Repository<User, int> userRepo(**connResult);
    
    auto users = userRepo.findAll();
    // ... use users ...
    
}  // Connection returned to pool
```

## With Transactions

```cpp
pq::ConnectionPool pool(config);

void transferMoney(int from, int to, double amount) {
    auto connResult = pool.acquire();
    if (!connResult) {
        throw std::runtime_error("No connection");
    }
    
    pq::PooledConnection conn = std::move(*connResult);
    
    {
        pq::Transaction tx(*conn);
        if (!tx) {
            throw std::runtime_error("Cannot start transaction");
        }
        
        conn->execute("UPDATE accounts SET balance = balance - $1 WHERE id = $2",
                      {std::to_string(amount), std::to_string(from)});
        conn->execute("UPDATE accounts SET balance = balance + $1 WHERE id = $2",
                      {std::to_string(amount), std::to_string(to)});
        
        tx.commit();
    }
    
}  // Connection returned to pool
```

## Error Handling

```cpp
auto connResult = pool.acquire();

if (!connResult) {
    // Could be timeout or connection failure
    const pq::DbError& err = connResult.error();
    
    if (err.message.find("timeout") != std::string::npos) {
        // Pool exhausted, all connections busy
        std::cerr << "Pool exhausted, try again later" << std::endl;
    } else {
        // Connection error
        std::cerr << "Connection error: " << err.message << std::endl;
    }
}
```

## Connection Validation

When `validateOnAcquire` is true (default), the pool validates connections before returning them:

```cpp
pq::PoolConfig config;
config.connectionString = "...";
config.validateOnAcquire = true;   // Test connection health
config.idleTimeout = std::chrono::milliseconds(30000);  // Validate after 30s idle

pq::ConnectionPool pool(config);

// This will get a validated connection
auto conn = pool.acquire();
// If validation fails, pool creates a new connection
```

## Best Practices

### 1. Configure Pool Size Appropriately

```cpp
// Consider:
// - Number of concurrent threads/requests
// - PostgreSQL max_connections setting
// - Available memory

pq::PoolConfig config;
config.minSize = 2;   // Keep 2 ready for quick access
config.maxSize = 20;  // Don't exceed this
```

### 2. Keep Connection Scope Short

```cpp
// Good: Short scope
void handleRequest() {
    auto conn = pool.acquire();
    if (conn) {
        auto result = conn->execute("SELECT * FROM data");
        // Process result...
    }
}  // Connection returned immediately

// Bad: Holding connection too long
void badExample() {
    auto conn = pool.acquire();  // Acquired
    processData();               // Long operation without connection
    doNetworkCall();             // Connection held but unused
    conn->execute("...");        // Finally used
}  // Connection held entire time
```

### 3. Handle Pool Exhaustion

```cpp
auto conn = pool.acquire(std::chrono::milliseconds(100));
if (!conn) {
    // Return 503 Service Unavailable, retry later, etc.
    return HttpResponse::ServiceUnavailable();
}
```

### 4. Shutdown Gracefully

```cpp
int main() {
    pq::ConnectionPool pool(config);
    
    // ... application runs ...
    
    // Clean shutdown
    pool.shutdown();
    
    return 0;
}
```

### 5. Don't Hold PooledConnection Across Async Operations

```cpp
// Bad: Holding across async boundary
auto conn = pool.acquire();
asyncOperation([conn = std::move(conn)]() {
    // Connection held during entire async operation
});

// Better: Acquire when needed
asyncOperation([&pool]() {
    auto conn = pool.acquire();
    // Use and release quickly
});
```

## Monitoring

```cpp
// Simple monitoring
void logPoolStats(const pq::ConnectionPool& pool) {
    std::cout << "Pool stats:"
              << " idle=" << pool.idleCount()
              << " active=" << pool.activeCount()
              << " total=" << pool.totalCount()
              << " max=" << pool.maxSize()
              << std::endl;
}

// Call periodically
logPoolStats(pool);
```

## Next Steps

- [Transactions](transactions.md) - Transactions with pooled connections
- [Error Handling](error-handling.md) - Handling pool errors
