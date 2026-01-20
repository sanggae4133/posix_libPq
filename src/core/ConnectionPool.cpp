/**
 * @file ConnectionPool.cpp
 * @brief Implementation of connection pool
 */

#include "pq/core/ConnectionPool.hpp"

namespace pq {
namespace core {

// PooledConnection implementation

PooledConnection::PooledConnection(ConnectionPool* pool, std::unique_ptr<Connection> conn)
    : pool_(pool)
    , conn_(std::move(conn)) {}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : pool_(other.pool_)
    , conn_(std::move(other.conn_)) {
    other.pool_ = nullptr;
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {
        release();
        pool_ = other.pool_;
        conn_ = std::move(other.conn_);
        other.pool_ = nullptr;
    }
    return *this;
}

PooledConnection::~PooledConnection() {
    release();
}

bool PooledConnection::isValid() const noexcept {
    return conn_ && conn_->isConnected();
}

void PooledConnection::release() {
    if (pool_ && conn_) {
        pool_->release(std::move(conn_));
        pool_ = nullptr;
    }
}

// ConnectionPool implementation

ConnectionPool::ConnectionPool(const PoolConfig& config)
    : config_(config) {
    // Pre-create minimum connections
    for (size_t i = 0; i < config_.minSize; ++i) {
        auto result = createConnection();
        if (result) {
            idle_.push_back(std::move(*result));
        }
    }
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

DbResult<PooledConnection> ConnectionPool::acquire() {
    return acquire(config_.acquireTimeout);
}

DbResult<PooledConnection> ConnectionPool::acquire(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (shutdown_) {
        return DbResult<PooledConnection>::error(DbError{"Pool is shutdown"});
    }
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (true) {
        // Try to get an idle connection
        if (!idle_.empty()) {
            auto conn = std::move(idle_.back());
            idle_.pop_back();
            
            // Validate if configured
            if (config_.validateOnAcquire && !validateConnection(*conn)) {
                // Connection is bad, try again
                continue;
            }
            
            ++activeCount_;
            return PooledConnection(this, std::move(conn));
        }
        
        // Try to create a new connection if under limit
        if (activeCount_ + idle_.size() < config_.maxSize) {
            lock.unlock();
            auto result = createConnection();
            lock.lock();
            
            if (result) {
                ++activeCount_;
                return PooledConnection(this, std::move(*result));
            }
            
            return DbResult<PooledConnection>::error(std::move(result).error());
        }
        
        // Wait for a connection to be returned
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            return DbResult<PooledConnection>::error(
                DbError{"Timeout waiting for connection from pool"});
        }
        
        if (shutdown_) {
            return DbResult<PooledConnection>::error(DbError{"Pool is shutdown"});
        }
    }
}

size_t ConnectionPool::idleCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return idle_.size();
}

size_t ConnectionPool::activeCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeCount_;
}

size_t ConnectionPool::totalCount() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeCount_ + idle_.size();
}

void ConnectionPool::drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    idle_.clear();
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    idle_.clear();
    cv_.notify_all();
}

void ConnectionPool::release(std::unique_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (activeCount_ > 0) {
        --activeCount_;
    }
    
    if (!shutdown_ && conn && conn->isConnected()) {
        idle_.push_back(std::move(conn));
    }
    
    cv_.notify_one();
}

DbResult<std::unique_ptr<Connection>> ConnectionPool::createConnection() {
    auto conn = std::make_unique<Connection>();
    auto result = conn->connect(config_.connectionString);
    
    if (!result) {
        return DbResult<std::unique_ptr<Connection>>::error(std::move(result).error());
    }
    
    return std::move(conn);
}

bool ConnectionPool::validateConnection(Connection& conn) {
    if (!conn.isConnected()) {
        return false;
    }
    
    // Execute a simple query to verify connection is alive
    auto result = conn.execute("SELECT 1");
    return result.hasValue();
}

} // namespace core
} // namespace pq
