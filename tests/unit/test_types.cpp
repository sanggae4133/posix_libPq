/**
 * @file test_types.cpp
 * @brief Unit tests for TypeTraits system
 */

#include <gtest/gtest.h>
#include <pq/core/Types.hpp>
#include <optional>
#include <string>

using namespace pq;

class TypeTraitsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TypeTraitsTest, BoolTraits) {
    EXPECT_EQ(PgTypeTraits<bool>::pgOid, oid::BOOL);
    EXPECT_FALSE(PgTypeTraits<bool>::isNullable);
    
    EXPECT_EQ(PgTypeTraits<bool>::toString(true), "t");
    EXPECT_EQ(PgTypeTraits<bool>::toString(false), "f");
    
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("t"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("T"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("1"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("f"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("0"));
}

TEST_F(TypeTraitsTest, IntTraits) {
    EXPECT_EQ(PgTypeTraits<int>::pgOid, oid::INT4);
    EXPECT_FALSE(PgTypeTraits<int>::isNullable);
    
    EXPECT_EQ(PgTypeTraits<int>::toString(42), "42");
    EXPECT_EQ(PgTypeTraits<int>::toString(-123), "-123");
    
    EXPECT_EQ(PgTypeTraits<int>::fromString("42"), 42);
    EXPECT_EQ(PgTypeTraits<int>::fromString("-123"), -123);
}

TEST_F(TypeTraitsTest, Int64Traits) {
    EXPECT_EQ(PgTypeTraits<int64_t>::pgOid, oid::INT8);
    
    int64_t bigValue = 9223372036854775807LL;
    std::string bigStr = std::to_string(bigValue);
    
    EXPECT_EQ(PgTypeTraits<int64_t>::toString(bigValue), bigStr);
    EXPECT_EQ(PgTypeTraits<int64_t>::fromString(bigStr.c_str()), bigValue);
}

TEST_F(TypeTraitsTest, DoubleTraits) {
    EXPECT_EQ(PgTypeTraits<double>::pgOid, oid::FLOAT8);
    
    double value = 3.14159;
    auto str = PgTypeTraits<double>::toString(value);
    auto parsed = PgTypeTraits<double>::fromString(str.c_str());
    
    EXPECT_NEAR(parsed, value, 0.00001);
}

TEST_F(TypeTraitsTest, StringTraits) {
    EXPECT_EQ(PgTypeTraits<std::string>::pgOid, oid::TEXT);
    
    std::string value = "Hello, World!";
    EXPECT_EQ(PgTypeTraits<std::string>::toString(value), value);
    EXPECT_EQ(PgTypeTraits<std::string>::fromString(value.c_str()), value);
}

TEST_F(TypeTraitsTest, OptionalTraits) {
    using OptInt = std::optional<int>;
    
    EXPECT_EQ(PgTypeTraits<OptInt>::pgOid, oid::INT4);
    EXPECT_TRUE(PgTypeTraits<OptInt>::isNullable);
    
    OptInt value = 42;
    EXPECT_EQ(PgTypeTraits<OptInt>::toString(value), "42");
    
    OptInt nullValue = std::nullopt;
    EXPECT_EQ(PgTypeTraits<OptInt>::toString(nullValue), "");
    
    auto parsed = PgTypeTraits<OptInt>::fromString("42");
    EXPECT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, 42);
    
    auto parsedNull = PgTypeTraits<OptInt>::fromString(nullptr);
    EXPECT_FALSE(parsedNull.has_value());
}

TEST_F(TypeTraitsTest, IsOptionalTrait) {
    EXPECT_FALSE(isOptionalV<int>);
    EXPECT_FALSE(isOptionalV<std::string>);
    EXPECT_TRUE(isOptionalV<std::optional<int>>);
    EXPECT_TRUE(isOptionalV<std::optional<std::string>>);
}

TEST_F(TypeTraitsTest, NullTerminatedString) {
    std::string_view sv = "hello";
    NullTerminatedString nts(sv);
    
    EXPECT_STREQ(nts.c_str(), "hello");
    
    // Verify it's actually null-terminated
    const char* ptr = nts;
    EXPECT_EQ(ptr[5], '\0');
}

TEST_F(TypeTraitsTest, ParamConverter) {
    ParamConverter<int> intConv(42);
    EXPECT_STREQ(intConv.ptr, "42");
    EXPECT_FALSE(intConv.isNull);
    
    std::optional<int> nullOpt;
    ParamConverter<std::optional<int>> nullConv(nullOpt);
    EXPECT_TRUE(nullConv.isNull);
    EXPECT_EQ(nullConv.ptr, nullptr);
    
    std::optional<int> valueOpt = 99;
    ParamConverter<std::optional<int>> valueConv(valueOpt);
    EXPECT_FALSE(valueConv.isNull);
    EXPECT_STREQ(valueConv.ptr, "99");
}

// Additional edge case tests

TEST_F(TypeTraitsTest, Int16Traits) {
    EXPECT_EQ(PgTypeTraits<int16_t>::pgOid, oid::INT2);
    EXPECT_FALSE(PgTypeTraits<int16_t>::isNullable);
    
    EXPECT_EQ(PgTypeTraits<int16_t>::toString(32767), "32767");
    EXPECT_EQ(PgTypeTraits<int16_t>::toString(-32768), "-32768");
    
    EXPECT_EQ(PgTypeTraits<int16_t>::fromString("32767"), 32767);
    EXPECT_EQ(PgTypeTraits<int16_t>::fromString("-32768"), -32768);
}

TEST_F(TypeTraitsTest, FloatTraits) {
    EXPECT_EQ(PgTypeTraits<float>::pgOid, oid::FLOAT4);
    EXPECT_FALSE(PgTypeTraits<float>::isNullable);
    
    float value = 3.14f;
    auto str = PgTypeTraits<float>::toString(value);
    auto parsed = PgTypeTraits<float>::fromString(str.c_str());
    
    EXPECT_NEAR(parsed, value, 0.001f);
}

TEST_F(TypeTraitsTest, BoolEdgeCases) {
    // Various truthy values
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("true"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("TRUE"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("t"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("T"));
    EXPECT_TRUE(PgTypeTraits<bool>::fromString("1"));
    
    // Various falsy values
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("false"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("FALSE"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("f"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("F"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString("0"));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString(""));
    EXPECT_FALSE(PgTypeTraits<bool>::fromString(nullptr));
}

TEST_F(TypeTraitsTest, StringEmpty) {
    std::string empty;
    EXPECT_EQ(PgTypeTraits<std::string>::toString(empty), "");
    EXPECT_EQ(PgTypeTraits<std::string>::fromString(""), "");
}

TEST_F(TypeTraitsTest, StringWithSpecialChars) {
    std::string special = "Hello\nWorld\t!";
    EXPECT_EQ(PgTypeTraits<std::string>::toString(special), special);
    EXPECT_EQ(PgTypeTraits<std::string>::fromString(special.c_str()), special);
}

TEST_F(TypeTraitsTest, StringNullHandling) {
    auto result = PgTypeTraits<std::string>::fromString(nullptr);
    EXPECT_TRUE(result.empty());
}

TEST_F(TypeTraitsTest, OptionalString) {
    using OptStr = std::optional<std::string>;
    
    EXPECT_EQ(PgTypeTraits<OptStr>::pgOid, oid::TEXT);
    EXPECT_TRUE(PgTypeTraits<OptStr>::isNullable);
    
    OptStr value = "hello";
    EXPECT_EQ(PgTypeTraits<OptStr>::toString(value), "hello");
    
    OptStr nullValue = std::nullopt;
    EXPECT_EQ(PgTypeTraits<OptStr>::toString(nullValue), "");
    
    auto parsed = PgTypeTraits<OptStr>::fromString("world");
    EXPECT_TRUE(parsed.has_value());
    EXPECT_EQ(*parsed, "world");
    
    auto parsedNull = PgTypeTraits<OptStr>::fromString(nullptr);
    EXPECT_FALSE(parsedNull.has_value());
}

TEST_F(TypeTraitsTest, OptionalDouble) {
    using OptDouble = std::optional<double>;
    
    EXPECT_EQ(PgTypeTraits<OptDouble>::pgOid, oid::FLOAT8);
    EXPECT_TRUE(PgTypeTraits<OptDouble>::isNullable);
    
    OptDouble value = 3.14159;
    auto str = PgTypeTraits<OptDouble>::toString(value);
    auto parsed = PgTypeTraits<OptDouble>::fromString(str.c_str());
    
    EXPECT_TRUE(parsed.has_value());
    EXPECT_NEAR(*parsed, 3.14159, 0.00001);
}

TEST_F(TypeTraitsTest, OptionalBool) {
    using OptBool = std::optional<bool>;
    
    EXPECT_EQ(PgTypeTraits<OptBool>::pgOid, oid::BOOL);
    EXPECT_TRUE(PgTypeTraits<OptBool>::isNullable);
    
    OptBool trueVal = true;
    OptBool falseVal = false;
    OptBool nullVal = std::nullopt;
    
    EXPECT_EQ(PgTypeTraits<OptBool>::toString(trueVal), "t");
    EXPECT_EQ(PgTypeTraits<OptBool>::toString(falseVal), "f");
    EXPECT_EQ(PgTypeTraits<OptBool>::toString(nullVal), "");
}

TEST_F(TypeTraitsTest, NullTerminatedStringFromStdString) {
    std::string str = "test string";
    NullTerminatedString nts(str);
    
    EXPECT_STREQ(nts.c_str(), "test string");
}

TEST_F(TypeTraitsTest, NullTerminatedStringFromCString) {
    const char* cstr = "c string";
    NullTerminatedString nts(cstr);
    
    EXPECT_STREQ(nts.c_str(), "c string");
}

TEST_F(TypeTraitsTest, NullTerminatedStringConversion) {
    std::string_view sv = "view";
    NullTerminatedString nts(sv);
    
    const char* ptr = nts;  // Implicit conversion
    EXPECT_STREQ(ptr, "view");
}

TEST_F(TypeTraitsTest, ParamConverterString) {
    ParamConverter<std::string> conv("hello world");
    
    EXPECT_STREQ(conv.ptr, "hello world");
    EXPECT_FALSE(conv.isNull);
}

TEST_F(TypeTraitsTest, ParamConverterBool) {
    ParamConverter<bool> trueConv(true);
    ParamConverter<bool> falseConv(false);
    
    EXPECT_STREQ(trueConv.ptr, "t");
    EXPECT_STREQ(falseConv.ptr, "f");
    EXPECT_FALSE(trueConv.isNull);
    EXPECT_FALSE(falseConv.isNull);
}

TEST_F(TypeTraitsTest, ParamConverterDouble) {
    ParamConverter<double> conv(3.14);
    
    EXPECT_FALSE(conv.isNull);
    EXPECT_NE(std::string(conv.ptr).find("3.14"), std::string::npos);
}

TEST_F(TypeTraitsTest, OptionalInnerType) {
    EXPECT_TRUE((std::is_same_v<OptionalInnerT<int>, int>));
    EXPECT_TRUE((std::is_same_v<OptionalInnerT<std::optional<int>>, int>));
    EXPECT_TRUE((std::is_same_v<OptionalInnerT<std::string>, std::string>));
    EXPECT_TRUE((std::is_same_v<OptionalInnerT<std::optional<std::string>>, std::string>));
}

TEST_F(TypeTraitsTest, OidConstants) {
    // Verify OID constants match PostgreSQL
    EXPECT_EQ(oid::BOOL, 16u);
    EXPECT_EQ(oid::BYTEA, 17u);
    EXPECT_EQ(oid::INT8, 20u);
    EXPECT_EQ(oid::INT2, 21u);
    EXPECT_EQ(oid::INT4, 23u);
    EXPECT_EQ(oid::TEXT, 25u);
    EXPECT_EQ(oid::FLOAT4, 700u);
    EXPECT_EQ(oid::FLOAT8, 701u);
    EXPECT_EQ(oid::VARCHAR, 1043u);
    EXPECT_EQ(oid::DATE, 1082u);
    EXPECT_EQ(oid::TIME, 1083u);
    EXPECT_EQ(oid::TIMESTAMP, 1114u);
    EXPECT_EQ(oid::TIMESTAMPTZ, 1184u);
    EXPECT_EQ(oid::NUMERIC, 1700u);
    EXPECT_EQ(oid::UUID, 2950u);
    EXPECT_EQ(oid::JSONB, 3802u);
}

TEST_F(TypeTraitsTest, DateTraitsRoundTrip) {
    Date date{2026, 2, 10};

    EXPECT_EQ(PgTypeTraits<Date>::pgOid, oid::DATE);
    EXPECT_EQ(PgTypeTraits<Date>::toString(date), "2026-02-10");

    const auto parsed = PgTypeTraits<Date>::fromString("2026-02-10");
    EXPECT_EQ(parsed, date);
}

TEST_F(TypeTraitsTest, TimeTraitsRoundTripWithFractionalSeconds) {
    Time time{12, 34, 56, 789};

    EXPECT_EQ(PgTypeTraits<Time>::pgOid, oid::TIME);
    EXPECT_EQ(PgTypeTraits<Time>::toString(time), "12:34:56.789");

    const auto parsed = PgTypeTraits<Time>::fromString("12:34:56.789123");
    EXPECT_EQ(parsed.hour, 12);
    EXPECT_EQ(parsed.minute, 34);
    EXPECT_EQ(parsed.second, 56);
    EXPECT_EQ(parsed.millisecond, 789);
}

TEST_F(TypeTraitsTest, TimestampTraitsRoundTripPreservesMillisecondPrecision) {
    const auto input = std::chrono::system_clock::time_point{} +
                       std::chrono::milliseconds(1739186705123LL);

    EXPECT_EQ(PgTypeTraits<std::chrono::system_clock::time_point>::pgOid, oid::TIMESTAMP);

    const auto serialized =
        PgTypeTraits<std::chrono::system_clock::time_point>::toString(input);
    const auto parsed =
        PgTypeTraits<std::chrono::system_clock::time_point>::fromString(serialized.c_str());

    EXPECT_EQ(parsed, input);
}

TEST_F(TypeTraitsTest, TimestampTzTraitsRoundTripPreservesInstantAndOffset) {
    TimestampTz value;
    value.timePoint = std::chrono::system_clock::time_point{} +
                      std::chrono::milliseconds(1739186705123LL);
    value.offsetMinutes = 9 * 60;

    EXPECT_EQ(PgTypeTraits<TimestampTz>::pgOid, oid::TIMESTAMPTZ);

    const auto serialized = PgTypeTraits<TimestampTz>::toString(value);
    EXPECT_NE(serialized.find("+09:00"), std::string::npos);

    const auto parsed = PgTypeTraits<TimestampTz>::fromString(serialized.c_str());
    EXPECT_EQ(parsed.timePoint, value.timePoint);
    EXPECT_EQ(parsed.offsetMinutes, value.offsetMinutes);
}

TEST_F(TypeTraitsTest, NumericTraitsPreservePrecisionAsString) {
    Numeric value{"123456789012345.123456789012345"};

    EXPECT_EQ(PgTypeTraits<Numeric>::pgOid, oid::NUMERIC);
    EXPECT_EQ(PgTypeTraits<Numeric>::toString(value), value.value);

    const auto parsed = PgTypeTraits<Numeric>::fromString(value.value.c_str());
    EXPECT_EQ(parsed, value);
}

TEST_F(TypeTraitsTest, UuidTraitsRoundTripAsString) {
    Uuid value{"550e8400-e29b-41d4-a716-446655440000"};

    EXPECT_EQ(PgTypeTraits<Uuid>::pgOid, oid::UUID);
    EXPECT_EQ(PgTypeTraits<Uuid>::toString(value), value.value);

    const auto parsed = PgTypeTraits<Uuid>::fromString(value.value.c_str());
    EXPECT_EQ(parsed, value);
}

TEST_F(TypeTraitsTest, JsonbTraitsRoundTripAsString) {
    Jsonb value{R"({"a":1,"nested":{"b":"x"}})"};

    EXPECT_EQ(PgTypeTraits<Jsonb>::pgOid, oid::JSONB);
    EXPECT_EQ(PgTypeTraits<Jsonb>::toString(value), value.value);

    const auto parsed = PgTypeTraits<Jsonb>::fromString(value.value.c_str());
    EXPECT_EQ(parsed, value);
}

TEST_F(TypeTraitsTest, OptionalNewTypesHandleNullValues) {
    EXPECT_FALSE(PgTypeTraits<std::optional<Date>>::fromString(nullptr).has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<Time>>::fromString(nullptr).has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<std::chrono::system_clock::time_point>>::fromString(
                     nullptr)
                     .has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<TimestampTz>>::fromString(nullptr).has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<Numeric>>::fromString(nullptr).has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<Uuid>>::fromString(nullptr).has_value());
    EXPECT_FALSE(PgTypeTraits<std::optional<Jsonb>>::fromString(nullptr).has_value());

    std::optional<Numeric> numeric = Numeric{"42.000000000000001"};
    std::optional<Uuid> uuid = Uuid{"550e8400-e29b-41d4-a716-446655440000"};
    std::optional<Jsonb> jsonb = Jsonb{R"({"ok":true})"};

    EXPECT_EQ(PgTypeTraits<std::optional<Numeric>>::toString(numeric), "42.000000000000001");
    EXPECT_EQ(PgTypeTraits<std::optional<Uuid>>::toString(uuid),
              "550e8400-e29b-41d4-a716-446655440000");
    EXPECT_EQ(PgTypeTraits<std::optional<Jsonb>>::toString(jsonb), R"({"ok":true})");
}
