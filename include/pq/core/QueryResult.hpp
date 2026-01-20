#pragma once

/**
 * @file QueryResult.hpp
 * @brief RAII wrapper for query results with row iteration support
 * 
 * Provides a safe, efficient interface for accessing PostgreSQL query results
 * with support for cursor-based iteration to handle large result sets.
 */

#include "PqHandle.hpp"
#include "Result.hpp"
#include "Types.hpp"
#include <optional>
#include <vector>
#include <stdexcept>
#include <cstring>

namespace pq {
namespace core {

/**
 * @brief Represents a single row in a query result
 * 
 * Lightweight view into the underlying PGresult. Does not own the result.
 */
class Row {
    PGresult* result_;
    int rowIndex_;
    int columnCount_;
    
public:
    Row(PGresult* result, int rowIndex)
        : result_(result)
        , rowIndex_(rowIndex)
        , columnCount_(PQnfields(result)) {}
    
    /**
     * @brief Get the number of columns in this row
     */
    [[nodiscard]] int columnCount() const noexcept {
        return columnCount_;
    }
    
    /**
     * @brief Check if a column value is NULL
     * @param columnIndex Zero-based column index
     */
    [[nodiscard]] bool isNull(int columnIndex) const noexcept {
        return PQgetisnull(result_, rowIndex_, columnIndex) == 1;
    }
    
    /**
     * @brief Get raw string value at column index
     * @param columnIndex Zero-based column index
     * @return Pointer to value (may be empty string for NULL)
     */
    [[nodiscard]] const char* getRaw(int columnIndex) const noexcept {
        return PQgetvalue(result_, rowIndex_, columnIndex);
    }
    
    /**
     * @brief Get column name by index
     * @param columnIndex Zero-based column index
     */
    [[nodiscard]] const char* columnName(int columnIndex) const noexcept {
        return PQfname(result_, columnIndex);
    }
    
    /**
     * @brief Get column index by name
     * @param name Column name
     * @return Column index or -1 if not found
     */
    [[nodiscard]] int columnIndex(const char* name) const noexcept {
        return PQfnumber(result_, name);
    }
    
    /**
     * @brief Get typed value at column index
     * @tparam T Target type (must have PgTypeTraits specialization)
     * @param columnIndex Zero-based column index
     * @throws std::runtime_error if NULL and T is not optional
     */
    template<typename T>
    [[nodiscard]] T get(int columnIndex) const {
        if constexpr (isOptionalV<T>) {
            if (isNull(columnIndex)) {
                return std::nullopt;
            }
            using Inner = OptionalInnerT<T>;
            return PgTypeTraits<Inner>::fromString(getRaw(columnIndex));
        } else {
            if (isNull(columnIndex)) {
                throw std::runtime_error(
                    std::string("NULL value in non-optional column: ") + 
                    columnName(columnIndex));
            }
            return PgTypeTraits<T>::fromString(getRaw(columnIndex));
        }
    }
    
    /**
     * @brief Get typed value by column name
     * @tparam T Target type
     * @param name Column name
     */
    template<typename T>
    [[nodiscard]] T get(const char* name) const {
        int idx = columnIndex(name);
        if (idx < 0) {
            throw std::runtime_error(
                std::string("Column not found: ") + name);
        }
        return get<T>(idx);
    }
    
    /**
     * @brief Get typed value by column name (string_view overload)
     */
    template<typename T>
    [[nodiscard]] T get(std::string_view name) const {
        NullTerminatedString nts(name);
        return get<T>(nts.c_str());
    }
};

/**
 * @brief Iterator for traversing query result rows
 */
class RowIterator {
    PGresult* result_;
    int currentRow_;
    
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Row;
    using difference_type = int;
    using pointer = Row*;
    using reference = Row;
    
    RowIterator(PGresult* result, int row)
        : result_(result), currentRow_(row) {}
    
    Row operator*() const {
        return Row(result_, currentRow_);
    }
    
    RowIterator& operator++() {
        ++currentRow_;
        return *this;
    }
    
    RowIterator operator++(int) {
        RowIterator tmp = *this;
        ++currentRow_;
        return tmp;
    }
    
    bool operator==(const RowIterator& other) const noexcept {
        return result_ == other.result_ && currentRow_ == other.currentRow_;
    }
    
    bool operator!=(const RowIterator& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief RAII wrapper for PostgreSQL query results
 * 
 * Provides safe access to query results with automatic cleanup.
 * Supports range-based for loops and cursor-based iteration.
 */
class QueryResult {
    PgResultPtr result_;
    int rowCount_;
    int columnCount_;
    
public:
    /**
     * @brief Construct from a PGresult pointer (takes ownership)
     */
    explicit QueryResult(PgResultPtr result)
        : result_(std::move(result))
        , rowCount_(result_ ? PQntuples(result_.get()) : 0)
        , columnCount_(result_ ? PQnfields(result_.get()) : 0) {}
    
    // Move-only semantics
    QueryResult(QueryResult&&) = default;
    QueryResult& operator=(QueryResult&&) = default;
    QueryResult(const QueryResult&) = delete;
    QueryResult& operator=(const QueryResult&) = delete;
    
    /**
     * @brief Check if the result is valid
     */
    [[nodiscard]] bool isValid() const noexcept {
        return result_ != nullptr;
    }
    
    /**
     * @brief Check if the query was successful
     */
    [[nodiscard]] bool isSuccess() const noexcept {
        return pq::core::isSuccess(result_);
    }
    
    /**
     * @brief Get the result status
     */
    [[nodiscard]] ExecStatusType status() const noexcept {
        return result_ ? PQresultStatus(result_.get()) : PGRES_FATAL_ERROR;
    }
    
    /**
     * @brief Get error message if query failed
     */
    [[nodiscard]] const char* errorMessage() const noexcept {
        return result_ ? PQresultErrorMessage(result_.get()) : "No result";
    }
    
    /**
     * @brief Get SQLSTATE code for errors
     */
    [[nodiscard]] const char* sqlState() const noexcept {
        if (!result_) return "";
        const char* state = PQresultErrorField(result_.get(), PG_DIAG_SQLSTATE);
        return state ? state : "";
    }
    
    /**
     * @brief Get the number of rows in the result
     */
    [[nodiscard]] int rowCount() const noexcept {
        return rowCount_;
    }
    
    /**
     * @brief Get the number of columns in the result
     */
    [[nodiscard]] int columnCount() const noexcept {
        return columnCount_;
    }
    
    /**
     * @brief Check if the result is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return rowCount_ == 0;
    }
    
    /**
     * @brief Get the number of rows affected by INSERT/UPDATE/DELETE
     */
    [[nodiscard]] int affectedRows() const noexcept {
        if (!result_) return 0;
        const char* affected = PQcmdTuples(result_.get());
        return affected && *affected ? std::atoi(affected) : 0;
    }
    
    /**
     * @brief Get column name by index
     */
    [[nodiscard]] const char* columnName(int index) const noexcept {
        return result_ ? PQfname(result_.get(), index) : "";
    }
    
    /**
     * @brief Get column index by name
     * @return Index or -1 if not found
     */
    [[nodiscard]] int columnIndex(const char* name) const noexcept {
        return result_ ? PQfnumber(result_.get(), name) : -1;
    }
    
    /**
     * @brief Get column OID
     */
    [[nodiscard]] Oid columnType(int index) const noexcept {
        return result_ ? PQftype(result_.get(), index) : 0;
    }
    
    /**
     * @brief Get all column names
     */
    [[nodiscard]] std::vector<std::string> columnNames() const {
        std::vector<std::string> names;
        names.reserve(columnCount_);
        for (int i = 0; i < columnCount_; ++i) {
            names.emplace_back(PQfname(result_.get(), i));
        }
        return names;
    }
    
    /**
     * @brief Access a specific row
     * @param index Zero-based row index
     */
    [[nodiscard]] Row row(int index) const {
        if (index < 0 || index >= rowCount_) {
            throw std::out_of_range("Row index out of range");
        }
        return Row(result_.get(), index);
    }
    
    /**
     * @brief Access row using array operator
     */
    [[nodiscard]] Row operator[](int index) const {
        return row(index);
    }
    
    // Range-based for loop support
    [[nodiscard]] RowIterator begin() const {
        return RowIterator(result_.get(), 0);
    }
    
    [[nodiscard]] RowIterator end() const {
        return RowIterator(result_.get(), rowCount_);
    }
    
    /**
     * @brief Get the first row, if any
     */
    [[nodiscard]] std::optional<Row> first() const {
        if (rowCount_ > 0) {
            return Row(result_.get(), 0);
        }
        return std::nullopt;
    }
    
    /**
     * @brief Access the underlying PGresult pointer (for advanced use)
     */
    [[nodiscard]] PGresult* raw() const noexcept {
        return result_.get();
    }
};

} // namespace core
} // namespace pq
