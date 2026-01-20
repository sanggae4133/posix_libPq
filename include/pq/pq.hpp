#pragma once

/**
 * @file pq.hpp
 * @brief Main convenience header for PosixLibPq
 * 
 * Include this single header to access all library functionality.
 * 
 * @code
 * #include <pq/pq.hpp>
 * @endcode
 */

// Core components
#include "core/PqHandle.hpp"
#include "core/Result.hpp"
#include "core/Types.hpp"
#include "core/QueryResult.hpp"
#include "core/Connection.hpp"
#include "core/Transaction.hpp"

// ORM components
#include "orm/Entity.hpp"
#include "orm/Mapper.hpp"
#include "orm/Repository.hpp"

/**
 * @namespace pq
 * @brief Root namespace for PosixLibPq library
 */
namespace pq {

// Bring commonly used types into pq namespace for convenience
using core::Connection;
using core::ConnectionConfig;
using core::QueryResult;
using core::Row;
using core::Transaction;
using core::Savepoint;

using orm::Repository;
using orm::MapperConfig;
using orm::ColumnFlags;

} // namespace pq
