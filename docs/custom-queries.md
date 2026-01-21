# Custom Queries

While the Repository pattern handles standard CRUD operations, you'll often need custom queries for complex business logic.

## Using Repository's executeQuery

```cpp
pq::Repository<User, int> userRepo(conn);

// Execute custom query, map results to entities
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
// Returns vector of entities (0 or more)
DbResult<std::vector<User>> executeQuery(
    std::string_view sql,
    const std::vector<std::string>& params = {}
);

// Returns optional entity (0 or 1)
DbResult<std::optional<User>> executeQueryOne(
    std::string_view sql,
    const std::vector<std::string>& params = {}
);
```

### Example: Single Result

```cpp
auto result = userRepo.executeQueryOne(
    "SELECT * FROM users WHERE email = $1",
    {"john@example.com"}
);

if (result) {
    if (*result) {
        User& user = **result;
        std::cout << "Found: " << user.name << std::endl;
    } else {
        std::cout << "User not found" << std::endl;
    }
}
```

## Direct Connection Queries

For queries that don't map to entities, use `Connection::execute` directly:

```cpp
pq::Connection conn(...);

// Simple query
auto result = conn.execute("SELECT COUNT(*) FROM users");
if (result) {
    int count = result->row(0).get<int>(0);
    std::cout << "Count: " << count << std::endl;
}

// Parameterized query
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

## Parameter Binding

Parameters use `$1`, `$2`, etc. placeholders:

```cpp
// String parameters
conn.execute(
    "SELECT * FROM users WHERE name = $1 AND city = $2",
    {"John", "New York"}
);

// Convert numeric parameters to strings
conn.execute(
    "SELECT * FROM users WHERE age > $1 AND age < $2",
    {std::to_string(18), std::to_string(65)}
);

// NULL parameters - use empty string
conn.execute(
    "INSERT INTO users (name, email) VALUES ($1, $2)",
    {"John", ""}  // email will be NULL
);
```

## Typed Parameter Binding

Use `executeParams` for type-safe parameter binding:

```cpp
// Parameters are automatically converted based on type
auto result = conn.executeParams(
    "SELECT * FROM users WHERE id = $1 AND active = $2",
    42,        // int -> "$1"
    true       // bool -> "t"
);

// With optional (NULL handling)
std::optional<std::string> maybeEmail = std::nullopt;
conn.executeParams(
    "INSERT INTO users (name, email) VALUES ($1, $2)",
    std::string("John"),
    maybeEmail  // Becomes NULL
);
```

## Working with QueryResult

```cpp
auto result = conn.execute("SELECT id, name, email, age FROM users");

if (result) {
    QueryResult& qr = *result;
    
    // Metadata
    std::cout << "Rows: " << qr.rowCount() << std::endl;
    std::cout << "Columns: " << qr.columnCount() << std::endl;
    
    // Column names
    for (const auto& name : qr.columnNames()) {
        std::cout << "Column: " << name << std::endl;
    }
    
    // Iterate rows
    for (int i = 0; i < qr.rowCount(); ++i) {
        Row row = qr.row(i);
        
        // By index
        int id = row.get<int>(0);
        std::string name = row.get<std::string>(1);
        
        // By column name
        auto email = row.get<std::optional<std::string>>("email");
        auto age = row.get<std::optional<int>>("age");
        
        // Check NULL
        if (row.isNull(2)) {
            std::cout << "Email is NULL" << std::endl;
        }
    }
    
    // Range-based for loop
    for (const auto& row : qr) {
        std::cout << row.get<std::string>("name") << std::endl;
    }
    
    // First row (if exists)
    if (auto first = qr.first()) {
        std::cout << "First: " << first->get<std::string>("name") << std::endl;
    }
}
```

## Aggregate Queries

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

## JOIN Queries

For JOIN queries, results may not map directly to a single entity:

```cpp
// Using direct query
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
        std::cout << "User: " << row.get<std::string>("user_name")
                  << " Order: " << row.get<int>("order_id")
                  << " Total: " << row.get<double>("total")
                  << std::endl;
    }
}
```

## INSERT with RETURNING

```cpp
auto result = conn.execute(
    "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id, created_at",
    {"John", "john@example.com"}
);

if (result && !result->empty()) {
    int id = result->row(0).get<int>("id");
    std::string createdAt = result->row(0).get<std::string>("created_at");
    std::cout << "Created user " << id << " at " << createdAt << std::endl;
}
```

## UPDATE/DELETE with Affected Rows

```cpp
// Using executeUpdate for affected row count
auto affected = conn.executeUpdate(
    "UPDATE users SET active = $1 WHERE last_login < $2",
    {"f", "2024-01-01"}
);

if (affected) {
    std::cout << "Deactivated " << *affected << " users" << std::endl;
}

// Or check affectedRows() from QueryResult
auto result = conn.execute("DELETE FROM sessions WHERE expired = true");
if (result) {
    std::cout << "Deleted " << result->affectedRows() << " sessions" << std::endl;
}
```

## Prepared Statements

For frequently executed queries, use prepared statements:

```cpp
// Prepare once
auto prepResult = conn.prepare("find_user_by_email", 
    "SELECT * FROM users WHERE email = $1");
if (!prepResult) {
    // Handle error
}

// Execute many times
auto result1 = conn.executePrepared("find_user_by_email", {"alice@example.com"});
auto result2 = conn.executePrepared("find_user_by_email", {"bob@example.com"});
```

## Transaction with Custom Queries

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

## Error Handling

```cpp
auto result = conn.execute("SELECT * FROM nonexistent_table");

if (!result) {
    const DbError& err = result.error();
    
    // Check SQLSTATE for specific errors
    if (err.sqlState == "42P01") {
        std::cerr << "Table does not exist" << std::endl;
    } else {
        std::cerr << "Query failed: " << err.message << std::endl;
    }
}
```

## Best Practices

### 1. Use Parameterized Queries (Always!)

```cpp
// NEVER do this - SQL injection risk!
conn.execute("SELECT * FROM users WHERE name = '" + userInput + "'");

// Always use parameters
conn.execute("SELECT * FROM users WHERE name = $1", {userInput});
```

### 2. Check for Empty Results

```cpp
auto result = conn.execute("SELECT * FROM users WHERE id = $1", {id});
if (result) {
    if (result->empty()) {
        // Handle not found
    } else {
        // Process result
    }
}
```

### 3. Use Appropriate Types

```cpp
// Match C++ types to PostgreSQL types
row.get<int32_t>("integer_column");
row.get<int64_t>("bigint_column");
row.get<double>("double_column");
row.get<std::string>("text_column");
row.get<bool>("boolean_column");
row.get<std::optional<std::string>>("nullable_column");
```

### 4. Consider Query Performance

```cpp
// Add LIMIT for potentially large results
conn.execute("SELECT * FROM logs ORDER BY timestamp DESC LIMIT 100");

// Use indexes (ensure proper indexing on queried columns)
conn.execute("SELECT * FROM users WHERE email = $1", {email});  // email should be indexed
```

## Next Steps

- [Error Handling](error-handling.md) - Query error handling
- [Transactions](transactions.md) - Transactions with custom queries
