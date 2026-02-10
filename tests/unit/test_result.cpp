/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type
 */

#include <gtest/gtest.h>
#include <pq/core/Result.hpp>
#include <string>

using namespace pq;

class ResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ResultTest, DefaultConstructedHasValue) {
    DbResult<int> result(42);
    EXPECT_TRUE(result.hasValue());
    EXPECT_FALSE(result.hasError());
    EXPECT_EQ(result.value(), 42);
}

TEST_F(ResultTest, ErrorResult) {
    auto result = DbResult<int>::error(DbError{"Something went wrong"});
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error().message, "Something went wrong");
}

TEST_F(ResultTest, BoolConversion) {
    DbResult<int> success(42);
    DbResult<int> failure = DbResult<int>::error(DbError{"error"});
    
    EXPECT_TRUE(static_cast<bool>(success));
    EXPECT_FALSE(static_cast<bool>(failure));
    
    if (success) {
        EXPECT_EQ(*success, 42);
    }
}

TEST_F(ResultTest, ValueOr) {
    DbResult<int> success(42);
    DbResult<int> failure = DbResult<int>::error(DbError{"error"});
    
    EXPECT_EQ(success.valueOr(0), 42);
    EXPECT_EQ(failure.valueOr(99), 99);
}

TEST_F(ResultTest, Map) {
    DbResult<int> result(10);
    auto mapped = result.map([](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.hasValue());
    EXPECT_EQ(mapped.value(), 20);
}

TEST_F(ResultTest, MapError) {
    auto result = DbResult<int>::error(DbError{"error"});
    auto mapped = result.map([](int x) { return x * 2; });
    
    EXPECT_TRUE(mapped.hasError());
    EXPECT_EQ(mapped.error().message, "error");
}

TEST_F(ResultTest, VoidResult) {
    DbResult<void> success = DbResult<void>::ok();
    DbResult<void> failure = DbResult<void>::error(DbError{"error"});
    
    EXPECT_TRUE(success.hasValue());
    EXPECT_FALSE(success.hasError());
    
    EXPECT_FALSE(failure.hasValue());
    EXPECT_TRUE(failure.hasError());
}

TEST_F(ResultTest, StringResult) {
    DbResult<std::string> result("hello");
    EXPECT_TRUE(result.hasValue());
    EXPECT_EQ(result.value(), "hello");
}

TEST_F(ResultTest, PointerAccess) {
    struct Data {
        int value;
        int getValue() const { return value; }
    };
    
    DbResult<Data> result(Data{42});
    EXPECT_EQ(result->getValue(), 42);
    EXPECT_EQ((*result).value, 42);
}

TEST_F(ResultTest, MoveSemantics) {
    DbResult<std::string> result("hello");
    std::string moved = std::move(result).value();
    EXPECT_EQ(moved, "hello");
}

// Additional edge case tests

TEST_F(ResultTest, ErrorWithSqlState) {
    auto result = DbResult<int>::error(DbError{"Connection failed", "08001", 500});
    
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error().message, "Connection failed");
    EXPECT_EQ(result.error().sqlState, "08001");
    EXPECT_EQ(result.error().errorCode, 500);
}

TEST_F(ResultTest, ValueThrowsOnError) {
    auto result = DbResult<int>::error(DbError{"error"});

    auto callValue = [&]() {
        auto value = result.value();
        (void)value;
    };

    EXPECT_THROW(callValue(), std::runtime_error);
}

TEST_F(ResultTest, ErrorThrowsOnValue) {
    DbResult<int> result(42);

    auto callError = [&]() {
        auto& error = result.error();
        (void)error;
    };

    EXPECT_THROW(callError(), std::runtime_error);
}

TEST_F(ResultTest, MapToString) {
    DbResult<int> result(42);
    auto mapped = result.map([](int x) { return "value: " + std::to_string(x); });
    
    EXPECT_TRUE(mapped.hasValue());
    EXPECT_EQ(mapped.value(), "value: 42");
}

TEST_F(ResultTest, MapWithDifferentType) {
    DbResult<int> result(42);
    auto mapped = result.map([](int x) { return std::to_string(x); });
    
    EXPECT_TRUE(mapped.hasValue());
    EXPECT_EQ(mapped.value(), "42");
}

TEST_F(ResultTest, ChainedMap) {
    DbResult<int> result(10);
    auto mapped = result
        .map([](int x) { return x * 2; })
        .map([](int x) { return x + 5; });
    
    EXPECT_TRUE(mapped.hasValue());
    EXPECT_EQ(mapped.value(), 25);
}

TEST_F(ResultTest, VoidResultError) {
    auto result = DbResult<void>::error(DbError{"void error", "00000", 1});
    
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error().message, "void error");
    EXPECT_EQ(result.error().sqlState, "00000");
}

TEST_F(ResultTest, InPlaceErrorConstruction) {
    Result<int, DbError> result(inPlaceError, "in-place error", "42000", 100);
    
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error().message, "in-place error");
    EXPECT_EQ(result.error().sqlState, "42000");
}

TEST_F(ResultTest, MoveOnlyType) {
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(MoveOnly&&) = default;
        MoveOnly& operator=(MoveOnly&&) = default;
        MoveOnly(const MoveOnly&) = delete;
        MoveOnly& operator=(const MoveOnly&) = delete;
    };
    
    DbResult<MoveOnly> result(MoveOnly{42});
    EXPECT_TRUE(result.hasValue());
    EXPECT_EQ(result.value().value, 42);
    
    MoveOnly moved = std::move(result).value();
    EXPECT_EQ(moved.value, 42);
}

TEST_F(ResultTest, ValueOrWithRvalue) {
    DbResult<std::string> result = DbResult<std::string>::error(DbError{"error"});
    
    std::string value = std::move(result).valueOr("default");
    EXPECT_EQ(value, "default");
}

TEST_F(ResultTest, ConstAccess) {
    const DbResult<int> result(42);
    
    EXPECT_TRUE(result.hasValue());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(*result, 42);
}
