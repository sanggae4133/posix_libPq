#pragma once

/**
 * @file PqHandle.hpp
 * @brief RAII wrappers for libpq handles (PGconn, PGresult)
 * 
 * Provides zero-cost abstraction over raw libpq pointers with automatic
 * resource management following the Rule of Zero principle.
 */

#include <memory>
#include <libpq-fe.h>

namespace pq {
namespace core {

/**
 * @brief Custom deleter for PGconn handles
 */
struct PgConnDeleter {
    void operator()(PGconn* conn) const noexcept {
        if (conn) {
            PQfinish(conn);
        }
    }
};

/**
 * @brief Custom deleter for PGresult handles
 */
struct PgResultDeleter {
    void operator()(PGresult* result) const noexcept {
        if (result) {
            PQclear(result);
        }
    }
};

/**
 * @brief RAII wrapper for PGconn* with unique ownership
 * 
 * Automatically calls PQfinish() when going out of scope.
 * Move-only semantics ensure single ownership.
 */
using PgConnPtr = std::unique_ptr<PGconn, PgConnDeleter>;

/**
 * @brief RAII wrapper for PGresult* with unique ownership
 * 
 * Automatically calls PQclear() when going out of scope.
 * Move-only semantics ensure single ownership.
 */
using PgResultPtr = std::unique_ptr<PGresult, PgResultDeleter>;

/**
 * @brief Factory function to create a managed PGconn pointer
 * @param conninfo Connection string for PostgreSQL
 * @return Managed PGconn pointer (may contain nullptr on failure)
 */
[[nodiscard]] inline PgConnPtr makePgConn(const char* conninfo) noexcept {
    return PgConnPtr(PQconnectdb(conninfo));
}

/**
 * @brief Factory function to wrap an existing PGresult pointer
 * @param result Raw PGresult pointer (ownership transferred)
 * @return Managed PGresult pointer
 */
[[nodiscard]] inline PgResultPtr makePgResult(PGresult* result) noexcept {
    return PgResultPtr(result);
}

/**
 * @brief Check if a PGconn handle represents a successful connection
 * @param conn The connection handle to check
 * @return true if connection is established and OK
 */
[[nodiscard]] inline bool isConnected(const PgConnPtr& conn) noexcept {
    return conn && PQstatus(conn.get()) == CONNECTION_OK;
}

/**
 * @brief Check if a PGresult represents a successful query
 * @param result The result handle to check
 * @return true if result indicates success (PGRES_COMMAND_OK or PGRES_TUPLES_OK)
 */
[[nodiscard]] inline bool isSuccess(const PgResultPtr& result) noexcept {
    if (!result) return false;
    const auto status = PQresultStatus(result.get());
    return status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK;
}

} // namespace core
} // namespace pq
