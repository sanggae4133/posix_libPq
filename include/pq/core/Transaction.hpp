#pragma once

/**
 * @file Transaction.hpp
 * @brief RAII-based transaction management
 * 
 * Provides automatic rollback on scope exit unless explicitly committed,
 * ensuring data integrity even when exceptions occur.
 */

#include "Connection.hpp"
#include "Result.hpp"

namespace pq {
namespace core {

/**
 * @brief RAII wrapper for database transactions
 * 
 * Automatically rolls back if not committed when going out of scope.
 * This ensures data integrity even when exceptions occur.
 * 
 * Usage:
 * @code
 * {
 *     Transaction tx(connection);
 *     if (!tx) { // handle begin error }
 *     
 *     // ... perform operations ...
 *     
 *     tx.commit();  // Explicit commit
 * }  // Auto-rollback if commit() not called
 * @endcode
 */
class Transaction {
    Connection* conn_;
    bool committed_{false};
    bool valid_{false};
    
public:
    /**
     * @brief Begin a transaction on the given connection
     * @param conn Connection to use (must outlive this Transaction)
     */
    explicit Transaction(Connection& conn);
    
    // Move-only semantics
    Transaction(Transaction&& other) noexcept;
    Transaction& operator=(Transaction&& other) noexcept;
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    
    /**
     * @brief Destructor - auto-rollback if not committed
     */
    ~Transaction();
    
    /**
     * @brief Check if transaction is valid (BEGIN succeeded)
     */
    [[nodiscard]] bool isValid() const noexcept {
        return valid_;
    }
    
    /**
     * @brief Check if transaction is valid
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return valid_;
    }
    
    /**
     * @brief Commit the transaction
     * @return Result indicating success or error
     */
    DbResult<void> commit();
    
    /**
     * @brief Rollback the transaction
     * @return Result indicating success or error
     */
    DbResult<void> rollback();
    
    /**
     * @brief Check if transaction has been committed
     */
    [[nodiscard]] bool isCommitted() const noexcept {
        return committed_;
    }
    
    /**
     * @brief Get the underlying connection
     */
    [[nodiscard]] Connection& connection() noexcept {
        return *conn_;
    }
    
    [[nodiscard]] const Connection& connection() const noexcept {
        return *conn_;
    }
};

/**
 * @brief Create a savepoint within a transaction
 * 
 * RAII wrapper that releases or rolls back to the savepoint automatically.
 */
class Savepoint {
    Connection* conn_;
    std::string name_;
    bool released_{false};
    bool valid_{false};
    
public:
    /**
     * @brief Create a savepoint
     * @param conn Connection with active transaction
     * @param name Savepoint name
     */
    Savepoint(Connection& conn, std::string_view name);
    
    // Move-only semantics
    Savepoint(Savepoint&& other) noexcept;
    Savepoint& operator=(Savepoint&& other) noexcept;
    Savepoint(const Savepoint&) = delete;
    Savepoint& operator=(const Savepoint&) = delete;
    
    ~Savepoint();
    
    /**
     * @brief Release the savepoint (make changes permanent within transaction)
     */
    DbResult<void> release();
    
    /**
     * @brief Rollback to this savepoint
     */
    DbResult<void> rollbackTo();
    
    [[nodiscard]] bool isValid() const noexcept {
        return valid_;
    }
    
    [[nodiscard]] explicit operator bool() const noexcept {
        return valid_;
    }
};

} // namespace core
} // namespace pq
