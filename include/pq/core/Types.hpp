#pragma once

/**
 * @file Types.hpp
 * @brief Type traits system for C++ to PostgreSQL type mapping
 * 
 * Provides compile-time type information for mapping between C++ types
 * and PostgreSQL OIDs, ensuring type safety and preventing SQL injection.
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <chrono>
#include <vector>
#include <type_traits>
#include <libpq-fe.h>

namespace pq {

// PostgreSQL OID constants (from PostgreSQL catalog/pg_type.h)
namespace oid {
    constexpr Oid BOOL      = 16;
    constexpr Oid BYTEA     = 17;
    constexpr Oid CHAR      = 18;
    constexpr Oid INT8      = 20;   // bigint
    constexpr Oid INT2      = 21;   // smallint
    constexpr Oid INT4      = 23;   // integer
    constexpr Oid TEXT      = 25;
    constexpr Oid OID       = 26;
    constexpr Oid FLOAT4    = 700;  // real
    constexpr Oid FLOAT8    = 701;  // double precision
    constexpr Oid VARCHAR   = 1043;
    constexpr Oid DATE      = 1082;
    constexpr Oid TIME      = 1083;
    constexpr Oid TIMESTAMP = 1114;
    constexpr Oid TIMESTAMPTZ = 1184;
    constexpr Oid NUMERIC   = 1700;
    constexpr Oid UUID      = 2950;
    constexpr Oid JSONB     = 3802;
} // namespace oid

/**
 * @brief Primary template for PostgreSQL type traits
 * 
 * Specializations provide:
 * - pgOid: The PostgreSQL OID for the type
 * - pgTypeName: SQL type name as string
 * - isNullable: Whether the type can represent NULL
 * - toString(): Convert C++ value to PostgreSQL text format
 * - fromString(): Parse PostgreSQL text format to C++ value
 */
template<typename T, typename Enable = void>
struct PgTypeTraits;

// Helper to detect optional types
template<typename T>
struct IsOptional : std::false_type {};

template<typename T>
struct IsOptional<std::optional<T>> : std::true_type {};

template<typename T>
inline constexpr bool isOptionalV = IsOptional<T>::value;

// Helper to get inner type of optional
template<typename T>
struct OptionalInner { using type = T; };

template<typename T>
struct OptionalInner<std::optional<T>> { using type = T; };

template<typename T>
using OptionalInnerT = typename OptionalInner<T>::type;

/**
 * @brief Type traits for bool
 */
template<>
struct PgTypeTraits<bool> {
    static constexpr Oid pgOid = oid::BOOL;
    static constexpr const char* pgTypeName = "boolean";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(bool value) {
        return value ? "t" : "f";
    }
    
    [[nodiscard]] static bool fromString(const char* str) {
        return str && (str[0] == 't' || str[0] == 'T' || str[0] == '1');
    }
};

/**
 * @brief Type traits for int16_t (smallint)
 */
template<>
struct PgTypeTraits<int16_t> {
    static constexpr Oid pgOid = oid::INT2;
    static constexpr const char* pgTypeName = "smallint";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(int16_t value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static int16_t fromString(const char* str) {
        return static_cast<int16_t>(std::stoi(str));
    }
};

/**
 * @brief Type traits for int32_t (integer)
 */
template<>
struct PgTypeTraits<int32_t> {
    static constexpr Oid pgOid = oid::INT4;
    static constexpr const char* pgTypeName = "integer";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(int32_t value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static int32_t fromString(const char* str) {
        return std::stoi(str);
    }
};

/**
 * @brief Type traits for int (alias for int32_t on most platforms)
 */
template<>
struct PgTypeTraits<int> {
    static constexpr Oid pgOid = oid::INT4;
    static constexpr const char* pgTypeName = "integer";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(int value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static int fromString(const char* str) {
        return std::stoi(str);
    }
};

/**
 * @brief Type traits for int64_t (bigint)
 */
template<>
struct PgTypeTraits<int64_t> {
    static constexpr Oid pgOid = oid::INT8;
    static constexpr const char* pgTypeName = "bigint";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(int64_t value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static int64_t fromString(const char* str) {
        return std::stoll(str);
    }
};

/**
 * @brief Type traits for float (real)
 */
template<>
struct PgTypeTraits<float> {
    static constexpr Oid pgOid = oid::FLOAT4;
    static constexpr const char* pgTypeName = "real";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(float value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static float fromString(const char* str) {
        return std::stof(str);
    }
};

/**
 * @brief Type traits for double (double precision)
 */
template<>
struct PgTypeTraits<double> {
    static constexpr Oid pgOid = oid::FLOAT8;
    static constexpr const char* pgTypeName = "double precision";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static std::string toString(double value) {
        return std::to_string(value);
    }
    
    [[nodiscard]] static double fromString(const char* str) {
        return std::stod(str);
    }
};

/**
 * @brief Type traits for std::string (text/varchar)
 */
template<>
struct PgTypeTraits<std::string> {
    static constexpr Oid pgOid = oid::TEXT;
    static constexpr const char* pgTypeName = "text";
    static constexpr bool isNullable = false;
    
    [[nodiscard]] static const std::string& toString(const std::string& value) {
        return value;
    }
    
    [[nodiscard]] static std::string fromString(const char* str) {
        return str ? std::string(str) : std::string();
    }
};

/**
 * @brief Type traits for std::optional<T>
 * 
 * Wraps any type to make it nullable.
 */
template<typename T>
struct PgTypeTraits<std::optional<T>> {
    using InnerTraits = PgTypeTraits<T>;
    
    static constexpr Oid pgOid = InnerTraits::pgOid;
    static constexpr const char* pgTypeName = InnerTraits::pgTypeName;
    static constexpr bool isNullable = true;
    
    [[nodiscard]] static std::string toString(const std::optional<T>& value) {
        if (value) {
            return InnerTraits::toString(*value);
        }
        return "";  // NULL represented as empty (will be handled specially)
    }
    
    [[nodiscard]] static std::optional<T> fromString(const char* str) {
        if (!str) {
            return std::nullopt;
        }
        return InnerTraits::fromString(str);
    }
};

/**
 * @brief Utility to ensure string_view is null-terminated for C API calls
 * 
 * libpq requires null-terminated strings. This helper ensures safety
 * when converting from std::string_view.
 */
class NullTerminatedString {
    std::string storage_;
    const char* ptr_;
    
public:
    explicit NullTerminatedString(std::string_view sv)
        : storage_(sv)
        , ptr_(storage_.c_str()) {}
    
    explicit NullTerminatedString(const std::string& s)
        : ptr_(s.c_str()) {}
    
    explicit NullTerminatedString(const char* s)
        : ptr_(s) {}
    
    [[nodiscard]] const char* c_str() const noexcept {
        return ptr_;
    }
    
    [[nodiscard]] operator const char*() const noexcept {
        return ptr_;
    }
};

/**
 * @brief Helper to convert a value to its PostgreSQL parameter representation
 */
template<typename T>
struct ParamConverter {
    std::string value;
    const char* ptr;
    bool isNull;
    
    explicit ParamConverter(const T& v)
        : value(PgTypeTraits<T>::toString(v))
        , ptr(value.c_str())
        , isNull(false) {}
};

template<typename T>
struct ParamConverter<std::optional<T>> {
    std::string value;
    const char* ptr;
    bool isNull;
    
    explicit ParamConverter(const std::optional<T>& v)
        : isNull(!v.has_value()) {
        if (v) {
            value = PgTypeTraits<T>::toString(*v);
            ptr = value.c_str();
        } else {
            ptr = nullptr;
        }
    }
};

} // namespace pq
