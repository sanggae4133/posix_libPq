#pragma once

/**
 * @file Result.hpp
 * @brief C++17 compatible Result<T, E> type for error handling
 * 
 * Provides a type-safe alternative to exceptions for error handling,
 * similar to std::expected (C++23) but compatible with C++17.
 */

#include <variant>
#include <string>
#include <type_traits>
#include <utility>
#include <stdexcept>

namespace pq {

/**
 * @brief Database error information
 */
struct DbError {
    std::string message;
    std::string sqlState;  // PostgreSQL SQLSTATE code
    int errorCode{0};
    
    DbError() = default;
    
    explicit DbError(std::string msg, std::string state = "", int code = 0)
        : message(std::move(msg))
        , sqlState(std::move(state))
        , errorCode(code) {}
    
    [[nodiscard]] const char* what() const noexcept {
        return message.c_str();
    }
};

/**
 * @brief Tag type for in-place construction of error
 */
struct InPlaceError {};
inline constexpr InPlaceError inPlaceError{};

/**
 * @brief Result type for operations that may fail
 * 
 * @tparam T The success value type
 * @tparam E The error type (defaults to DbError)
 * 
 * Usage:
 * @code
 * Result<User, DbError> findUser(int id) {
 *     if (found) return user;
 *     return Result<User, DbError>::error(DbError{"User not found"});
 * }
 * 
 * auto result = findUser(1);
 * if (result) {
 *     auto& user = result.value();
 * } else {
 *     auto& err = result.error();
 * }
 * @endcode
 */
template<typename T, typename E = DbError>
class Result {
public:
    using ValueType = T;
    using ErrorType = E;
    
private:
    std::variant<T, E> data_;
    
public:
    // Success constructors
    Result(const T& value) : data_(value) {}
    Result(T&& value) : data_(std::move(value)) {}
    
    // Error constructor with tag
    template<typename... Args>
    Result(InPlaceError, Args&&... args)
        : data_(std::in_place_index<1>, std::forward<Args>(args)...) {}
    
    // Factory methods
    [[nodiscard]] static Result ok(T value) {
        return Result(std::move(value));
    }
    
    [[nodiscard]] static Result error(E err) {
        Result r(T{});
        r.data_ = std::move(err);
        return r;
    }
    
    template<typename... Args>
    [[nodiscard]] static Result error(Args&&... args) {
        return Result(inPlaceError, std::forward<Args>(args)...);
    }
    
    // Observers
    [[nodiscard]] bool hasValue() const noexcept {
        return std::holds_alternative<T>(data_);
    }
    
    [[nodiscard]] bool hasError() const noexcept {
        return std::holds_alternative<E>(data_);
    }
    
    [[nodiscard]] explicit operator bool() const noexcept {
        return hasValue();
    }
    
    // Value access
    [[nodiscard]] T& value() & {
        if (!hasValue()) {
            throw std::runtime_error("Result does not contain a value");
        }
        return std::get<T>(data_);
    }
    
    [[nodiscard]] const T& value() const& {
        if (!hasValue()) {
            throw std::runtime_error("Result does not contain a value");
        }
        return std::get<T>(data_);
    }
    
    [[nodiscard]] T&& value() && {
        if (!hasValue()) {
            throw std::runtime_error("Result does not contain a value");
        }
        return std::get<T>(std::move(data_));
    }
    
    // Error access
    [[nodiscard]] E& error() & {
        if (!hasError()) {
            throw std::runtime_error("Result does not contain an error");
        }
        return std::get<E>(data_);
    }
    
    [[nodiscard]] const E& error() const& {
        if (!hasError()) {
            throw std::runtime_error("Result does not contain an error");
        }
        return std::get<E>(data_);
    }
    
    // Value or default
    template<typename U>
    [[nodiscard]] T valueOr(U&& defaultValue) const& {
        if (hasValue()) {
            return std::get<T>(data_);
        }
        return static_cast<T>(std::forward<U>(defaultValue));
    }
    
    template<typename U>
    [[nodiscard]] T valueOr(U&& defaultValue) && {
        if (hasValue()) {
            return std::get<T>(std::move(data_));
        }
        return static_cast<T>(std::forward<U>(defaultValue));
    }
    
    // Monadic operations
    template<typename F>
    auto map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (hasValue()) {
            return Result<U, E>::ok(std::forward<F>(f)(value()));
        }
        return Result<U, E>::error(error());
    }
    
    template<typename F>
    auto map(F&& f) && -> Result<std::invoke_result_t<F, T&&>, E> {
        using U = std::invoke_result_t<F, T&&>;
        if (hasValue()) {
            return Result<U, E>::ok(std::forward<F>(f)(std::move(*this).value()));
        }
        return Result<U, E>::error(std::move(*this).error());
    }
    
    // Pointer-like access
    [[nodiscard]] T* operator->() {
        return &value();
    }
    
    [[nodiscard]] const T* operator->() const {
        return &value();
    }
    
    [[nodiscard]] T& operator*() & {
        return value();
    }
    
    [[nodiscard]] const T& operator*() const& {
        return value();
    }
};

/**
 * @brief Specialization for void success type
 */
template<typename E>
class Result<void, E> {
public:
    using ValueType = void;
    using ErrorType = E;
    
private:
    std::variant<std::monostate, E> data_;
    
public:
    Result() : data_(std::monostate{}) {}
    
    template<typename... Args>
    Result(InPlaceError, Args&&... args)
        : data_(std::in_place_index<1>, std::forward<Args>(args)...) {}
    
    [[nodiscard]] static Result ok() {
        return Result();
    }
    
    [[nodiscard]] static Result error(E err) {
        Result r;
        r.data_ = std::move(err);
        return r;
    }
    
    template<typename... Args>
    [[nodiscard]] static Result error(Args&&... args) {
        return Result(inPlaceError, std::forward<Args>(args)...);
    }
    
    [[nodiscard]] bool hasValue() const noexcept {
        return std::holds_alternative<std::monostate>(data_);
    }
    
    [[nodiscard]] bool hasError() const noexcept {
        return std::holds_alternative<E>(data_);
    }
    
    [[nodiscard]] explicit operator bool() const noexcept {
        return hasValue();
    }
    
    [[nodiscard]] E& error() & {
        return std::get<E>(data_);
    }
    
    [[nodiscard]] const E& error() const& {
        return std::get<E>(data_);
    }
};

// Convenient type alias
template<typename T>
using DbResult = Result<T, DbError>;

} // namespace pq
