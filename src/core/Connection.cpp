/**
 * @file Connection.cpp
 * @brief Implementation of PostgreSQL connection management
 */

#include "pq/core/Connection.hpp"
#include <sstream>
#include <cstring>

namespace pq {
namespace core {

// ConnectionConfig implementation

std::string ConnectionConfig::toConnectionString() const {
    std::ostringstream ss;
    
    if (!host.empty()) {
        ss << "host=" << host << " ";
    }
    if (port != 0) {
        ss << "port=" << port << " ";
    }
    if (!database.empty()) {
        ss << "dbname=" << database << " ";
    }
    if (!user.empty()) {
        ss << "user=" << user << " ";
    }
    if (!password.empty()) {
        ss << "password=" << password << " ";
    }
    if (!options.empty()) {
        ss << options << " ";
    }
    if (connectTimeoutSec > 0) {
        ss << "connect_timeout=" << connectTimeoutSec;
    }
    
    return ss.str();
}

ConnectionConfig ConnectionConfig::fromConnectionString(std::string_view connStr) {
    ConnectionConfig config;
    // Basic parsing - in production, use PQconninfoParse
    config.options = std::string(connStr);
    return config;
}

// Connection implementation

Connection::Connection(std::string_view connectionString) {
    connect(connectionString);
}

Connection::Connection(const ConnectionConfig& config) {
    connect(config);
}

DbResult<void> Connection::connect(std::string_view connectionString) {
    NullTerminatedString connStr(connectionString);
    conn_ = makePgConn(connStr.c_str());
    
    if (!isConnected()) {
        return DbResult<void>::error(makeError("connect"));
    }
    
    return DbResult<void>::ok();
}

DbResult<void> Connection::connect(const ConnectionConfig& config) {
    return connect(config.toConnectionString());
}

void Connection::disconnect() noexcept {
    conn_.reset();
    inTransaction_ = false;
}

bool Connection::isConnected() const noexcept {
    return pq::core::isConnected(conn_);
}

ConnStatusType Connection::status() const noexcept {
    return conn_ ? PQstatus(conn_.get()) : CONNECTION_BAD;
}

const char* Connection::lastError() const noexcept {
    return conn_ ? PQerrorMessage(conn_.get()) : "Not connected";
}

int Connection::serverVersion() const noexcept {
    return conn_ ? PQserverVersion(conn_.get()) : 0;
}

DbResult<QueryResult> Connection::execute(std::string_view sql) {
    if (!isConnected()) {
        return DbResult<QueryResult>::error(DbError{"Not connected"});
    }
    
    NullTerminatedString sqlStr(sql);
    PgResultPtr result(PQexec(conn_.get(), sqlStr.c_str()));
    QueryResult qr(std::move(result));
    
    if (!qr.isSuccess()) {
        return DbResult<QueryResult>::error(makeError(qr, "execute"));
    }
    
    return qr;
}

DbResult<QueryResult> Connection::execute(std::string_view sql,
                                           std::initializer_list<std::string> params) {
    std::vector<std::string> paramVec(params);
    return execute(sql, paramVec);
}

DbResult<QueryResult> Connection::execute(std::string_view sql,
                                           const std::vector<std::string>& params) {
    if (!isConnected()) {
        return DbResult<QueryResult>::error(DbError{"Not connected"});
    }
    
    std::vector<const char*> paramValues;
    paramValues.reserve(params.size());
    for (const auto& p : params) {
        // Keep empty strings as empty strings. Use optional overload for NULL.
        paramValues.push_back(p.c_str());
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
        return DbResult<QueryResult>::error(makeError(qr, "execute"));
    }
    
    return qr;
}

DbResult<QueryResult> Connection::execute(
        std::string_view sql,
        const std::vector<std::optional<std::string>>& params) {
    if (!isConnected()) {
        return DbResult<QueryResult>::error(DbError{"Not connected"});
    }

    std::vector<const char*> paramValues;
    paramValues.reserve(params.size());
    for (const auto& p : params) {
        paramValues.push_back(p ? p->c_str() : nullptr);
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
        return DbResult<QueryResult>::error(makeError(qr, "execute"));
    }

    return qr;
}

DbResult<int> Connection::executeUpdate(std::string_view sql) {
    auto result = execute(sql);
    if (!result) {
        return DbResult<int>::error(std::move(result).error());
    }
    return result->affectedRows();
}

DbResult<int> Connection::executeUpdate(std::string_view sql,
                                         std::initializer_list<std::string> params) {
    auto result = execute(sql, params);
    if (!result) {
        return DbResult<int>::error(std::move(result).error());
    }
    return result->affectedRows();
}

DbResult<void> Connection::prepare(std::string_view name, std::string_view sql) {
    if (!isConnected()) {
        return DbResult<void>::error(DbError{"Not connected"});
    }
    
    NullTerminatedString nameStr(name);
    NullTerminatedString sqlStr(sql);
    
    PgResultPtr result(PQprepare(
        conn_.get(),
        nameStr.c_str(),
        sqlStr.c_str(),
        0,        // Let PostgreSQL infer parameter types
        nullptr
    ));
    
    QueryResult qr(std::move(result));
    
    if (!qr.isSuccess()) {
        return DbResult<void>::error(makeError(qr, "prepare"));
    }
    
    return DbResult<void>::ok();
}

DbResult<QueryResult> Connection::executePrepared(std::string_view name,
                                                   std::initializer_list<std::string> params) {
    if (!isConnected()) {
        return DbResult<QueryResult>::error(DbError{"Not connected"});
    }
    
    std::vector<const char*> paramValues;
    paramValues.reserve(params.size());
    for (const auto& p : params) {
        paramValues.push_back(p.c_str());
    }
    
    NullTerminatedString nameStr(name);
    
    PgResultPtr result(PQexecPrepared(
        conn_.get(),
        nameStr.c_str(),
        static_cast<int>(paramValues.size()),
        paramValues.data(),
        nullptr,  // Text format
        nullptr,  // Text format
        0         // Text format result
    ));
    
    QueryResult qr(std::move(result));
    
    if (!qr.isSuccess()) {
        return DbResult<QueryResult>::error(makeError(qr, "executePrepared"));
    }
    
    return qr;
}

DbResult<void> Connection::beginTransaction() {
    if (inTransaction_) {
        return DbResult<void>::error(DbError{"Already in transaction"});
    }
    
    auto result = execute("BEGIN");
    if (!result) {
        return DbResult<void>::error(std::move(result).error());
    }
    
    inTransaction_ = true;
    return DbResult<void>::ok();
}

DbResult<void> Connection::commit() {
    if (!inTransaction_) {
        return DbResult<void>::error(DbError{"Not in transaction"});
    }
    
    auto result = execute("COMMIT");
    if (!result) {
        return DbResult<void>::error(std::move(result).error());
    }

    inTransaction_ = false;
    return DbResult<void>::ok();
}

DbResult<void> Connection::rollback() {
    if (!inTransaction_) {
        return DbResult<void>::error(DbError{"Not in transaction"});
    }
    
    auto result = execute("ROLLBACK");
    if (!result) {
        return DbResult<void>::error(std::move(result).error());
    }

    inTransaction_ = false;
    return DbResult<void>::ok();
}

std::string Connection::escapeString(std::string_view value) const {
    if (!isConnected()) {
        return std::string(value);
    }
    
    std::string result;
    result.resize(value.size() * 2 + 1);
    
    int error = 0;
    size_t len = PQescapeStringConn(
        conn_.get(),
        result.data(),
        value.data(),
        value.size(),
        &error
    );
    
    result.resize(len);
    return result;
}

std::string Connection::escapeIdentifier(std::string_view identifier) const {
    if (!isConnected()) {
        return "\"" + std::string(identifier) + "\"";
    }
    
    NullTerminatedString idStr(identifier);
    char* escaped = PQescapeIdentifier(conn_.get(), idStr.c_str(), identifier.size());
    
    if (!escaped) {
        return "\"" + std::string(identifier) + "\"";
    }
    
    std::string result(escaped);
    PQfreemem(escaped);
    return result;
}

DbError Connection::makeError(const char* context) const {
    std::string msg;
    if (context) {
        msg = std::string(context) + ": ";
    }
    msg += lastError();
    
    return DbError{std::move(msg), "", 0};
}

DbError Connection::makeError(const QueryResult& result, const char* context) const {
    std::string msg;
    if (context) {
        msg = std::string(context) + ": ";
    }
    msg += result.errorMessage();
    
    return DbError{std::move(msg), result.sqlState(), 0};
}

} // namespace core
} // namespace pq
