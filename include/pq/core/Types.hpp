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
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <cstdlib>
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
 * @brief Date-only value type (YYYY-MM-DD)
 */
struct Date {
    int year{1970};
    int month{1};
    int day{1};
};

inline bool operator==(const Date& lhs, const Date& rhs) noexcept {
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day;
}

/**
 * @brief Time-only value type (HH:MM:SS.mmm)
 */
struct Time {
    int hour{0};
    int minute{0};
    int second{0};
    int millisecond{0};
};

inline bool operator==(const Time& lhs, const Time& rhs) noexcept {
    return lhs.hour == rhs.hour && lhs.minute == rhs.minute &&
           lhs.second == rhs.second && lhs.millisecond == rhs.millisecond;
}

/**
 * @brief Timestamp with explicit timezone offset information
 *
 * `timePoint` stores UTC instant and `offsetMinutes` stores the textual timezone offset.
 */
struct TimestampTz {
    std::chrono::system_clock::time_point timePoint{};
    int offsetMinutes{0};
};

inline bool operator==(const TimestampTz& lhs, const TimestampTz& rhs) noexcept {
    return lhs.timePoint == rhs.timePoint && lhs.offsetMinutes == rhs.offsetMinutes;
}

/**
 * @brief String-backed NUMERIC value
 */
struct Numeric {
    std::string value;

    Numeric() = default;
    Numeric(const char* v) : value(v ? v : "") {}
    Numeric(std::string v) : value(std::move(v)) {}
    Numeric(std::string_view v) : value(v) {}
};

inline bool operator==(const Numeric& lhs, const Numeric& rhs) noexcept {
    return lhs.value == rhs.value;
}

/**
 * @brief String-backed UUID value
 */
struct Uuid {
    std::string value;

    Uuid() = default;
    Uuid(const char* v) : value(v ? v : "") {}
    Uuid(std::string v) : value(std::move(v)) {}
    Uuid(std::string_view v) : value(v) {}
};

inline bool operator==(const Uuid& lhs, const Uuid& rhs) noexcept {
    return lhs.value == rhs.value;
}

/**
 * @brief String-backed JSONB value
 */
struct Jsonb {
    std::string value;

    Jsonb() = default;
    Jsonb(const char* v) : value(v ? v : "") {}
    Jsonb(std::string v) : value(std::move(v)) {}
    Jsonb(std::string_view v) : value(v) {}
};

inline bool operator==(const Jsonb& lhs, const Jsonb& rhs) noexcept {
    return lhs.value == rhs.value;
}

namespace detail {

inline std::tm gmtimeUtc(std::time_t epochSec) {
    std::tm tmValue{};
#if defined(_WIN32)
    gmtime_s(&tmValue, &epochSec);
#else
    gmtime_r(&epochSec, &tmValue);
#endif
    return tmValue;
}

inline std::time_t timegmUtc(std::tm* tmValue) {
#if defined(_WIN32)
    return _mkgmtime(tmValue);
#elif defined(__APPLE__) || defined(__linux__)
    return timegm(tmValue);
#else
    return std::mktime(tmValue);
#endif
}

inline std::pair<std::time_t, int> splitEpochSecondsAndMillis(
        const std::chrono::system_clock::time_point& tp) {
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count();

    std::time_t seconds = static_cast<std::time_t>(totalMs / 1000);
    int millis = static_cast<int>(totalMs % 1000);

    if (millis < 0) {
        millis += 1000;
        --seconds;
    }

    return {seconds, millis};
}

inline std::string zeroPad(int value, int width) {
    std::ostringstream oss;
    oss << std::setw(width) << std::setfill('0') << value;
    return oss.str();
}

inline int parseTwoDigit(std::string_view text, std::size_t pos) {
    if (pos + 2 > text.size() ||
        !std::isdigit(static_cast<unsigned char>(text[pos])) ||
        !std::isdigit(static_cast<unsigned char>(text[pos + 1]))) {
        throw std::invalid_argument("Invalid 2-digit field");
    }
    return (text[pos] - '0') * 10 + (text[pos + 1] - '0');
}

inline void parseDateParts(std::string_view text, int& year, int& month, int& day) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        throw std::invalid_argument("Invalid date format (expected YYYY-MM-DD)");
    }

    year = std::stoi(std::string(text.substr(0, 4)));
    month = parseTwoDigit(text, 5);
    day = parseTwoDigit(text, 8);
}

inline int parseFractionToMicros(std::string_view fractionText) {
    if (fractionText.empty()) {
        return 0;
    }

    std::string digits;
    digits.reserve(6);

    for (const char ch : fractionText) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            throw std::invalid_argument("Invalid fractional second digits");
        }
        if (digits.size() < 6) {
            digits.push_back(ch);
        }
    }

    while (digits.size() < 6) {
        digits.push_back('0');
    }

    return std::stoi(digits);
}

inline void parseTimeParts(std::string_view text,
                           int& hour,
                           int& minute,
                           int& second,
                           int& micros) {
    if (text.size() < 8 || text[2] != ':' || text[5] != ':') {
        throw std::invalid_argument("Invalid time format (expected HH:MM:SS[.fraction])");
    }

    hour = parseTwoDigit(text, 0);
    minute = parseTwoDigit(text, 3);
    second = parseTwoDigit(text, 6);
    micros = 0;

    if (text.size() == 8) {
        return;
    }

    if (text[8] != '.') {
        throw std::invalid_argument("Invalid time format");
    }

    micros = parseFractionToMicros(text.substr(9));
}

inline int parseOffsetMinutes(std::string_view text) {
    if (text.empty() || text == "Z" || text == "z") {
        return 0;
    }

    if (text[0] != '+' && text[0] != '-') {
        throw std::invalid_argument("Invalid timezone offset format");
    }

    const int sign = (text[0] == '-') ? -1 : 1;
    text.remove_prefix(1);

    int hours = 0;
    int minutes = 0;

    if (text.size() == 2) {
        hours = std::stoi(std::string(text));
    } else if (text.size() == 4) {
        hours = std::stoi(std::string(text.substr(0, 2)));
        minutes = std::stoi(std::string(text.substr(2, 2)));
    } else if (text.size() == 5 && text[2] == ':') {
        hours = std::stoi(std::string(text.substr(0, 2)));
        minutes = std::stoi(std::string(text.substr(3, 2)));
    } else {
        throw std::invalid_argument("Invalid timezone offset format");
    }

    return sign * (hours * 60 + minutes);
}

inline std::string formatOffsetMinutes(int offsetMinutes) {
    const char sign = (offsetMinutes < 0) ? '-' : '+';
    const int absMinutes = std::abs(offsetMinutes);
    const int hours = absMinutes / 60;
    const int minutes = absMinutes % 60;

    return std::string(1, sign) + zeroPad(hours, 2) + ":" + zeroPad(minutes, 2);
}

inline std::chrono::system_clock::time_point makeUtcTimePoint(
        int year, int month, int day, int hour, int minute, int second, int micros) {
    std::tm tmValue{};
    tmValue.tm_year = year - 1900;
    tmValue.tm_mon = month - 1;
    tmValue.tm_mday = day;
    tmValue.tm_hour = hour;
    tmValue.tm_min = minute;
    tmValue.tm_sec = second;

    const std::time_t epochSec = timegmUtc(&tmValue);
    return std::chrono::system_clock::from_time_t(epochSec) + std::chrono::microseconds(micros);
}

} // namespace detail

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

// Note: On platforms where int == int32_t (e.g., macOS ARM64), 
// PgTypeTraits<int32_t> above already handles 'int' type.

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
 * @brief Type traits for date-only values
 */
template<>
struct PgTypeTraits<Date> {
    static constexpr Oid pgOid = oid::DATE;
    static constexpr const char* pgTypeName = "date";
    static constexpr bool isNullable = false;

    [[nodiscard]] static std::string toString(const Date& value) {
        return detail::zeroPad(value.year, 4) + "-" +
               detail::zeroPad(value.month, 2) + "-" +
               detail::zeroPad(value.day, 2);
    }

    [[nodiscard]] static Date fromString(const char* str) {
        if (!str) {
            throw std::invalid_argument("NULL date string");
        }
        Date value{};
        detail::parseDateParts(str, value.year, value.month, value.day);
        return value;
    }
};

/**
 * @brief Type traits for time-only values
 */
template<>
struct PgTypeTraits<Time> {
    static constexpr Oid pgOid = oid::TIME;
    static constexpr const char* pgTypeName = "time";
    static constexpr bool isNullable = false;

    [[nodiscard]] static std::string toString(const Time& value) {
        return detail::zeroPad(value.hour, 2) + ":" +
               detail::zeroPad(value.minute, 2) + ":" +
               detail::zeroPad(value.second, 2) + "." +
               detail::zeroPad(value.millisecond, 3);
    }

    [[nodiscard]] static Time fromString(const char* str) {
        if (!str) {
            throw std::invalid_argument("NULL time string");
        }

        int hour = 0;
        int minute = 0;
        int second = 0;
        int micros = 0;
        detail::parseTimeParts(str, hour, minute, second, micros);

        Time value{};
        value.hour = hour;
        value.minute = minute;
        value.second = second;
        value.millisecond = micros / 1000;
        return value;
    }
};

/**
 * @brief Type traits for timestamp without timezone
 */
template<>
struct PgTypeTraits<std::chrono::system_clock::time_point> {
    static constexpr Oid pgOid = oid::TIMESTAMP;
    static constexpr const char* pgTypeName = "timestamp";
    static constexpr bool isNullable = false;

    [[nodiscard]] static std::string toString(const std::chrono::system_clock::time_point& value) {
        const auto [epochSec, millis] = detail::splitEpochSecondsAndMillis(value);
        const std::tm tmValue = detail::gmtimeUtc(epochSec);

        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S")
            << "." << detail::zeroPad(millis, 3);
        return oss.str();
    }

    [[nodiscard]] static std::chrono::system_clock::time_point fromString(const char* str) {
        if (!str) {
            throw std::invalid_argument("NULL timestamp string");
        }

        std::string_view text(str);
        const auto separatorPos = text.find_first_of(" T");
        if (separatorPos == std::string_view::npos) {
            throw std::invalid_argument("Invalid timestamp format");
        }

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int micros = 0;

        detail::parseDateParts(text.substr(0, separatorPos), year, month, day);

        std::string_view timePart = text.substr(separatorPos + 1);
        const auto tzPos = timePart.find_first_of("+-Zz");
        if (tzPos != std::string_view::npos) {
            timePart = timePart.substr(0, tzPos);
        }

        detail::parseTimeParts(timePart, hour, minute, second, micros);
        return detail::makeUtcTimePoint(year, month, day, hour, minute, second, micros);
    }
};

/**
 * @brief Type traits for timestamp with timezone
 */
template<>
struct PgTypeTraits<TimestampTz> {
    static constexpr Oid pgOid = oid::TIMESTAMPTZ;
    static constexpr const char* pgTypeName = "timestamptz";
    static constexpr bool isNullable = false;

    [[nodiscard]] static std::string toString(const TimestampTz& value) {
        const auto localTime = value.timePoint + std::chrono::minutes(value.offsetMinutes);
        const auto [epochSec, millis] = detail::splitEpochSecondsAndMillis(localTime);
        const std::tm tmValue = detail::gmtimeUtc(epochSec);

        std::ostringstream oss;
        oss << std::put_time(&tmValue, "%Y-%m-%d %H:%M:%S")
            << "." << detail::zeroPad(millis, 3)
            << detail::formatOffsetMinutes(value.offsetMinutes);
        return oss.str();
    }

    [[nodiscard]] static TimestampTz fromString(const char* str) {
        if (!str) {
            throw std::invalid_argument("NULL timestamptz string");
        }

        std::string_view text(str);
        const auto separatorPos = text.find_first_of(" T");
        if (separatorPos == std::string_view::npos) {
            throw std::invalid_argument("Invalid timestamptz format");
        }

        std::string_view datePart = text.substr(0, separatorPos);
        std::string_view timeAndOffset = text.substr(separatorPos + 1);

        std::size_t tzPos = std::string_view::npos;
        for (std::size_t i = 0; i < timeAndOffset.size(); ++i) {
            const char ch = timeAndOffset[i];
            if ((ch == '+' || ch == '-' || ch == 'Z' || ch == 'z') && i >= 8) {
                tzPos = i;
                break;
            }
        }

        const std::string_view timePart =
            (tzPos == std::string_view::npos) ? timeAndOffset : timeAndOffset.substr(0, tzPos);
        const std::string_view offsetPart =
            (tzPos == std::string_view::npos) ? std::string_view{} : timeAndOffset.substr(tzPos);

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        int micros = 0;

        detail::parseDateParts(datePart, year, month, day);
        detail::parseTimeParts(timePart, hour, minute, second, micros);

        const int offsetMinutes = detail::parseOffsetMinutes(offsetPart);
        const auto localTime = detail::makeUtcTimePoint(year, month, day, hour, minute, second, micros);

        TimestampTz value{};
        value.timePoint = localTime - std::chrono::minutes(offsetMinutes);
        value.offsetMinutes = offsetMinutes;
        return value;
    }
};

/**
 * @brief Type traits for string-backed NUMERIC
 */
template<>
struct PgTypeTraits<Numeric> {
    static constexpr Oid pgOid = oid::NUMERIC;
    static constexpr const char* pgTypeName = "numeric";
    static constexpr bool isNullable = false;

    [[nodiscard]] static const std::string& toString(const Numeric& value) {
        return value.value;
    }

    [[nodiscard]] static Numeric fromString(const char* str) {
        return Numeric{str ? std::string(str) : std::string()};
    }
};

/**
 * @brief Type traits for string-backed UUID
 */
template<>
struct PgTypeTraits<Uuid> {
    static constexpr Oid pgOid = oid::UUID;
    static constexpr const char* pgTypeName = "uuid";
    static constexpr bool isNullable = false;

    [[nodiscard]] static const std::string& toString(const Uuid& value) {
        return value.value;
    }

    [[nodiscard]] static Uuid fromString(const char* str) {
        return Uuid{str ? std::string(str) : std::string()};
    }
};

/**
 * @brief Type traits for string-backed JSONB
 */
template<>
struct PgTypeTraits<Jsonb> {
    static constexpr Oid pgOid = oid::JSONB;
    static constexpr const char* pgTypeName = "jsonb";
    static constexpr bool isNullable = false;

    [[nodiscard]] static const std::string& toString(const Jsonb& value) {
        return value.value;
    }

    [[nodiscard]] static Jsonb fromString(const char* str) {
        return Jsonb{str ? std::string(str) : std::string()};
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
