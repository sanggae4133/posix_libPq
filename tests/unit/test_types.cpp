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
