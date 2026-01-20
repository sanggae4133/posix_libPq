/**
 * @file Transaction.cpp
 * @brief Implementation of RAII transaction management
 */

#include "pq/core/Transaction.hpp"

namespace pq {
namespace core {

// Transaction implementation

Transaction::Transaction(Connection& conn)
    : conn_(&conn)
    , committed_(false)
    , valid_(false) {
    auto result = conn_->beginTransaction();
    valid_ = result.hasValue();
}

Transaction::Transaction(Transaction&& other) noexcept
    : conn_(other.conn_)
    , committed_(other.committed_)
    , valid_(other.valid_) {
    other.conn_ = nullptr;
    other.valid_ = false;
}

Transaction& Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        // Rollback current transaction if needed
        if (valid_ && !committed_ && conn_) {
            conn_->rollback();
        }
        
        conn_ = other.conn_;
        committed_ = other.committed_;
        valid_ = other.valid_;
        
        other.conn_ = nullptr;
        other.valid_ = false;
    }
    return *this;
}

Transaction::~Transaction() {
    if (valid_ && !committed_ && conn_) {
        // Auto-rollback on destruction if not committed
        conn_->rollback();
    }
}

DbResult<void> Transaction::commit() {
    if (!valid_) {
        return DbResult<void>::error(DbError{"Transaction not valid"});
    }
    if (committed_) {
        return DbResult<void>::error(DbError{"Transaction already committed"});
    }
    
    auto result = conn_->commit();
    if (result) {
        committed_ = true;
        valid_ = false;
    }
    return result;
}

DbResult<void> Transaction::rollback() {
    if (!valid_) {
        return DbResult<void>::error(DbError{"Transaction not valid"});
    }
    if (committed_) {
        return DbResult<void>::error(DbError{"Transaction already committed"});
    }
    
    auto result = conn_->rollback();
    valid_ = false;
    return result;
}

// Savepoint implementation

Savepoint::Savepoint(Connection& conn, std::string_view name)
    : conn_(&conn)
    , name_(name)
    , released_(false)
    , valid_(false) {
    if (!conn_->inTransaction()) {
        return;
    }
    
    std::string sql = "SAVEPOINT " + conn_->escapeIdentifier(name);
    auto result = conn_->execute(sql);
    valid_ = result.hasValue();
}

Savepoint::Savepoint(Savepoint&& other) noexcept
    : conn_(other.conn_)
    , name_(std::move(other.name_))
    , released_(other.released_)
    , valid_(other.valid_) {
    other.conn_ = nullptr;
    other.valid_ = false;
}

Savepoint& Savepoint::operator=(Savepoint&& other) noexcept {
    if (this != &other) {
        // Rollback to this savepoint if needed
        if (valid_ && !released_ && conn_) {
            rollbackTo();
        }
        
        conn_ = other.conn_;
        name_ = std::move(other.name_);
        released_ = other.released_;
        valid_ = other.valid_;
        
        other.conn_ = nullptr;
        other.valid_ = false;
    }
    return *this;
}

Savepoint::~Savepoint() {
    if (valid_ && !released_ && conn_) {
        // Auto-rollback to savepoint on destruction if not released
        rollbackTo();
    }
}

DbResult<void> Savepoint::release() {
    if (!valid_) {
        return DbResult<void>::error(DbError{"Savepoint not valid"});
    }
    if (released_) {
        return DbResult<void>::error(DbError{"Savepoint already released"});
    }
    
    std::string sql = "RELEASE SAVEPOINT " + conn_->escapeIdentifier(name_);
    auto result = conn_->execute(sql);
    
    if (result) {
        released_ = true;
        valid_ = false;
    }
    return result.hasValue() ? DbResult<void>::ok() 
                              : DbResult<void>::error(std::move(result).error());
}

DbResult<void> Savepoint::rollbackTo() {
    if (!valid_) {
        return DbResult<void>::error(DbError{"Savepoint not valid"});
    }
    if (released_) {
        return DbResult<void>::error(DbError{"Savepoint already released"});
    }
    
    std::string sql = "ROLLBACK TO SAVEPOINT " + conn_->escapeIdentifier(name_);
    auto result = conn_->execute(sql);
    
    // Note: After rollback, savepoint still exists
    return result.hasValue() ? DbResult<void>::ok() 
                              : DbResult<void>::error(std::move(result).error());
}

} // namespace core
} // namespace pq
