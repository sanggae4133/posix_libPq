#pragma once

/**
 * @file Connection.hpp
 * @brief PostgreSQL connection management with RAII
 * 
 * Provides a safe, efficient interface for database connections
 * with support for prepared statements and parameterized queries.
 */

#include "PqHandle.hpp"
#include "Result.hpp"
#include "QueryResult.hpp"
#include "Types.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <initializer_list>
#include <memory>
#include <optional>

namespace pq {
namespace core {

// Forward declarations
class Transaction;

/**
 * @brief Configuration options for a database connection
 */
struct ConnectionConfig {
    std::string host = "localhost";
    int port = 5432;
    std::string database;
    std::string user;
    std::string password;
    std::string options;
    int connectTimeoutSec = 10;
    
    /**
     * @brief Build a connection string from config
     */
    [[nodiscard]] std::string toConnectionString() const;
    
    /**
     * @brief Create config from a connection string
     */
    [[nodiscard]] static ConnectionConfig fromConnectionString(std::string_view connStr);
};

/**
 * @brief RAII wrapper for PostgreSQL database connection
 * 
 * Manages a single connection to PostgreSQL with automatic cleanup.
 * Supports parameterized queries and prepared statements for security.
 */
class Connection {
    PgConnPtr conn_;
    bool inTransaction_{false};
    
    friend class Transaction;
    
public:
    /**
     * @brief Construct an unconnected Connection
     */
    Connection() = default;
    
    /**
     * @brief Construct and connect using a connection string
     * @param connectionString PostgreSQL connection string
     */
    explicit Connection(std::string_view connectionString);
    
    /**
     * @brief Construct and connect using config
     * @param config Connection configuration
     */
    explicit Connection(const ConnectionConfig& config);
    
    // Move-only semantics (RAII)
    Connection(Connection&&) = default;
    Connection& operator=(Connection&&) = default;
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    
    ~Connection() = default;
    
    /**
     * @brief Connect to database
     * @param connectionString PostgreSQL connection string
     * @return Result indicating success or error
     */
    DbResult<void> connect(std::string_view connectionString);
    
    /**
     * @brief Connect using config
     */
    DbResult<void> connect(const ConnectionConfig& config);
    
    /**
     * @brief Disconnect from database
     */
    void disconnect() noexcept;
    
    /**
     * @brief Check if connected
     */
    [[nodiscard]] bool isConnected() const noexcept;
    
    /**
     * @brief Get connection status
     */
    [[nodiscard]] ConnStatusType status() const noexcept;
    
    /**
     * @brief Get last error message
     */
    [[nodiscard]] const char* lastError() const noexcept;
    
    /**
     * @brief Get PostgreSQL server version
     * @return Version number (e.g., 150007 for 15.0.7)
     */
    [[nodiscard]] int serverVersion() const noexcept;
    
    /**
     * @brief Execute a simple query (no parameters)
     * @param sql SQL query string
     * @return Query result or error
     */
    DbResult<QueryResult> execute(std::string_view sql);
    
    /**
     * @brief Execute a parameterized query
     * @param sql SQL query with $1, $2, ... placeholders
     * @param params Parameter values
     * @return Query result or error
     */
    DbResult<QueryResult> execute(std::string_view sql, 
                                   std::initializer_list<std::string> params);
    
    /**
     * @brief Execute a parameterized query with vector parameters
     */
    DbResult<QueryResult> execute(std::string_view sql,
                                   const std::vector<std::string>& params);

    /**
     * @brief Execute a parameterized query with nullable parameters
     */
    DbResult<QueryResult> execute(std::string_view sql,
                                   const std::vector<std::optional<std::string>>& params);
    
    /**
     * @brief Execute a parameterized query with typed parameters
     * @tparam Args Parameter types
     * @param sql SQL query with $1, $2, ... placeholders
     * @param args Parameter values
     */
    template<typename... Args>
    DbResult<QueryResult> executeParams(std::string_view sql, Args&&... args);
    
    /**
     * @brief Execute a query and return affected row count
     * @param sql SQL query (INSERT/UPDATE/DELETE)
     * @return Number of affected rows or error
     */
    DbResult<int> executeUpdate(std::string_view sql);
    
    /**
     * @brief Execute an update with parameters
     */
    DbResult<int> executeUpdate(std::string_view sql,
                                 std::initializer_list<std::string> params);
    
    /**
     * @brief Prepare a statement for repeated execution
     * @param name Statement name (unique per connection)
     * @param sql SQL with $1, $2, ... placeholders
     * @return Result indicating success or error
     */
    DbResult<void> prepare(std::string_view name, std::string_view sql);
    
    /**
     * @brief Execute a prepared statement
     * @param name Statement name
     * @param params Parameter values
     */
    DbResult<QueryResult> executePrepared(std::string_view name,
                                           std::initializer_list<std::string> params);
    
    /**
     * @brief Begin a transaction
     * @return Result indicating success or error
     */
    DbResult<void> beginTransaction();
    
    /**
     * @brief Commit current transaction
     */
    DbResult<void> commit();
    
    /**
     * @brief Rollback current transaction
     */
    DbResult<void> rollback();
    
    /**
     * @brief Check if currently in a transaction
     */
    [[nodiscard]] bool inTransaction() const noexcept {
        return inTransaction_;
    }
    
    /**
     * @brief Escape a string value for use in SQL
     * @param value String to escape
     * @return Escaped string (without quotes)
     */
    [[nodiscard]] std::string escapeString(std::string_view value) const;
    
    /**
     * @brief Escape an identifier (table/column name)
     * @param identifier Identifier to escape
     * @return Escaped identifier (with quotes)
     */
    [[nodiscard]] std::string escapeIdentifier(std::string_view identifier) const;
    
    /**
     * @brief Access the underlying PGconn pointer (for advanced use)
     */
    [[nodiscard]] PGconn* raw() const noexcept {
        return conn_.get();
    }
    
private:
    /**
     * @brief Create DbError from current connection state
     */
    [[nodiscard]] DbError makeError(const char* context = nullptr) const;
    
    /**
     * @brief Create DbError from a query result
     */
    [[nodiscard]] DbError makeError(const QueryResult& result, 
                                     const char* context = nullptr) const;
};

// Template implementation
template<typename... Args>
DbResult<QueryResult> Connection::executeParams(std::string_view sql, Args&&... args) {
    if (!isConnected()) {
        return DbResult<QueryResult>::error(DbError{"Not connected"});
    }
    
    // Convert all parameters to strings and store them
    std::vector<std::string> paramStrings;
    std::vector<bool> paramNulls;
    paramStrings.reserve(sizeof...(Args));
    paramNulls.reserve(sizeof...(Args));
    
    // Use fold expression to convert each argument
    (void)std::initializer_list<int>{(
        [&](auto&& arg) {
            using ArgType = std::decay_t<decltype(arg)>;
            ParamConverter<ArgType> conv(std::forward<decltype(arg)>(arg));
            paramStrings.push_back(std::move(conv.value));
            paramNulls.push_back(conv.isNull);
        }(std::forward<Args>(args)), 0
    )...};
    
    // Build parameter arrays for PQexecParams
    std::vector<const char*> paramValues;
    paramValues.reserve(sizeof...(Args));
    
    for (size_t i = 0; i < paramStrings.size(); ++i) {
        paramValues.push_back(paramNulls[i] ? nullptr : paramStrings[i].c_str());
    }
    
    NullTerminatedString sqlStr(sql);
    
    PgResultPtr result(PQexecParams(
        conn_.get(),
        sqlStr.c_str(),
        static_cast<int>(paramValues.size()),
        nullptr,  // Let PostgreSQL infer types
        paramValues.data(),
        nullptr,  // Text format
        nullptr,  // Text format
        0         // Text format result
    ));
    
    QueryResult qr(std::move(result));
    
    if (!qr.isSuccess()) {
        return DbResult<QueryResult>::error(makeError(qr, "executeParams"));
    }
    
    return qr;
}

} // namespace core
} // namespace pq
