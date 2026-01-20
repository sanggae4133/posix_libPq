#pragma once

/**
 * @file ConnectionPool.hpp
 * @brief Connection pool for efficient database connection management
 * 
 * Provides connection pooling with automatic acquisition/release,
 * maximum connection limits, and connection validation.
 */

#include "PqHandle.hpp"
#include "Connection.hpp"
#include "Result.hpp"
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>

namespace pq {
namespace core {

/**
 * @brief Configuration options for connection pool
 */
struct PoolConfig {
    std::string connectionString;
    size_t maxSize = 10;                          // Maximum connections
    size_t minSize = 1;                           // Minimum idle connections
    std::chrono::milliseconds acquireTimeout{5000};  // Timeout for acquire
    std::chrono::milliseconds idleTimeout{60000};    // Idle timeout before validation
    bool validateOnAcquire = true;                // Validate connection before returning
};

// Forward declaration
class ConnectionPool;

/**
 * @brief RAII wrapper for pooled connections
 * 
 * Automatically returns connection to pool when destroyed.
 */
class PooledConnection {
    ConnectionPool* pool_;
    std::unique_ptr<Connection> conn_;
    
    friend class ConnectionPool;
    
    PooledConnection(ConnectionPool* pool, std::unique_ptr<Connection> conn);
    
public:
    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;
    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;
    
    ~PooledConnection();
    
    /**
     * @brief Access the underlying connection
     */
    [[nodiscard]] Connection& operator*() noexcept { return *conn_; }
    [[nodiscard]] const Connection& operator*() const noexcept { return *conn_; }
    [[nodiscard]] Connection* operator->() noexcept { return conn_.get(); }
    [[nodiscard]] const Connection* operator->() const noexcept { return conn_.get(); }
    
    /**
     * @brief Check if connection is valid
     */
    [[nodiscard]] bool isValid() const noexcept;
    
    /**
     * @brief Release connection back to pool manually
     */
    void release();
};

/**
 * @brief Thread-safe connection pool
 * 
 * Usage:
 * @code
 * ConnectionPool pool(config);
 * 
 * {
 *     auto conn = pool.acquire();
 *     if (conn) {
 *         conn->execute("SELECT 1");
 *     }
 * }  // Connection automatically returned to pool
 * @endcode
 */
class ConnectionPool {
    PoolConfig config_;
    
    std::vector<std::unique_ptr<Connection>> idle_;
    size_t activeCount_{0};
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    
    bool shutdown_{false};
    
    friend class PooledConnection;
    
public:
    /**
     * @brief Create a connection pool with the given configuration
     */
    explicit ConnectionPool(const PoolConfig& config);
    
    /**
     * @brief Destructor - closes all connections
     */
    ~ConnectionPool();
    
    // Non-copyable, non-movable
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    ConnectionPool(ConnectionPool&&) = delete;
    ConnectionPool& operator=(ConnectionPool&&) = delete;
    
    /**
     * @brief Acquire a connection from the pool
     * @return Pooled connection or error if timeout/failure
     */
    [[nodiscard]] DbResult<PooledConnection> acquire();
    
    /**
     * @brief Acquire with custom timeout
     */
    [[nodiscard]] DbResult<PooledConnection> acquire(std::chrono::milliseconds timeout);
    
    /**
     * @brief Get current pool statistics
     */
    [[nodiscard]] size_t idleCount() const noexcept;
    [[nodiscard]] size_t activeCount() const noexcept;
    [[nodiscard]] size_t totalCount() const noexcept;
    [[nodiscard]] size_t maxSize() const noexcept { return config_.maxSize; }
    
    /**
     * @brief Close all idle connections
     */
    void drain();
    
    /**
     * @brief Shutdown the pool - all connections will be closed
     */
    void shutdown();
    
private:
    /**
     * @brief Return a connection to the pool
     */
    void release(std::unique_ptr<Connection> conn);
    
    /**
     * @brief Create a new connection
     */
    [[nodiscard]] DbResult<std::unique_ptr<Connection>> createConnection();
    
    /**
     * @brief Validate a connection is still usable
     */
    [[nodiscard]] bool validateConnection(Connection& conn);
};

} // namespace core
} // namespace pq
