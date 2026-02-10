/**
 * @file ConnectionPool.cpp
 * @brief Implementation of connection pool
 */

#include "pq/core/ConnectionPool.hpp"

namespace pq {
namespace core {

// PooledConnection implementation

PooledConnection::PooledConnection(std::shared_ptr<ConnectionPoolState> state,
                                   std::unique_ptr<Connection> conn)
    : state_(std::move(state))
    , conn_(std::move(conn)) {}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : state_(std::move(other.state_))
    , conn_(std::move(other.conn_)) {
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {
        release();
        state_ = std::move(other.state_);
        conn_ = std::move(other.conn_);
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
    if (!conn_) {
        state_.reset();
        return;
    }

    if (state_) {
        std::lock_guard<std::mutex> lock(state_->mutex);

        if (state_->activeCount > 0) {
            --state_->activeCount;
        }

        if (!state_->shutdown && conn_->isConnected()) {
            state_->idle.push_back(std::move(conn_));
        } else {
            conn_.reset();
        }

        state_->cv.notify_one();
    }

    conn_.reset();
    state_.reset();
}

// ConnectionPool implementation

ConnectionPool::ConnectionPool(const PoolConfig& config)
    : config_(config)
    , state_(std::make_shared<ConnectionPoolState>()) {
    // Pre-create minimum connections
    for (size_t i = 0; i < config_.minSize; ++i) {
        auto result = createConnection();
        if (result) {
            state_->idle.push_back(std::move(*result));
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
    std::unique_lock<std::mutex> lock(state_->mutex);
    
    if (state_->shutdown) {
        return DbResult<PooledConnection>::error(DbError{"Pool is shutdown"});
    }
    
    auto deadline = std::chrono::steady_clock::now() + timeout;
    
    while (true) {
        // Try to get an idle connection
        if (!state_->idle.empty()) {
            auto conn = std::move(state_->idle.back());
            state_->idle.pop_back();
            ++state_->activeCount;  // Reserve slot before releasing lock

            lock.unlock();
            const bool isValid = !config_.validateOnAcquire || validateConnection(*conn);
            lock.lock();

            if (state_->shutdown) {
                if (state_->activeCount > 0) {
                    --state_->activeCount;
                }
                return DbResult<PooledConnection>::error(DbError{"Pool is shutdown"});
            }

            if (!isValid) {
                if (state_->activeCount > 0) {
                    --state_->activeCount;
                }
                continue;
            }

            lock.unlock();
            return PooledConnection(state_, std::move(conn));
        }
        
        // Reserve a create slot while holding the lock to enforce maxSize.
        if (state_->activeCount + state_->idle.size() + state_->pendingCreates < config_.maxSize) {
            ++state_->pendingCreates;
            lock.unlock();
            auto result = createConnection();
            lock.lock();

            if (state_->pendingCreates > 0) {
                --state_->pendingCreates;
            }
            
            if (result) {
                ++state_->activeCount;
                lock.unlock();
                return PooledConnection(state_, std::move(*result));
            }
            
            state_->cv.notify_one();
            return DbResult<PooledConnection>::error(std::move(result).error());
        }
        
        // Wait for a connection to be returned
        if (state_->cv.wait_until(lock, deadline) == std::cv_status::timeout) {
            return DbResult<PooledConnection>::error(
                DbError{"Timeout waiting for connection from pool"});
        }
        
        if (state_->shutdown) {
            return DbResult<PooledConnection>::error(DbError{"Pool is shutdown"});
        }
    }
}

size_t ConnectionPool::idleCount() const noexcept {
    std::lock_guard<std::mutex> lock(state_->mutex);
    return state_->idle.size();
}

size_t ConnectionPool::activeCount() const noexcept {
    std::lock_guard<std::mutex> lock(state_->mutex);
    return state_->activeCount;
}

size_t ConnectionPool::totalCount() const noexcept {
    std::lock_guard<std::mutex> lock(state_->mutex);
    return state_->activeCount + state_->idle.size() + state_->pendingCreates;
}

void ConnectionPool::drain() {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->idle.clear();
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->shutdown = true;
    state_->idle.clear();
    state_->cv.notify_all();
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
