/**
 * @file test_query_result.cpp
 * @brief Unit tests for QueryResult, Row, and RowIterator classes
 */

#include <gtest/gtest.h>
#include <pq/core/QueryResult.hpp>
#include <pq/core/PqHandle.hpp>
#include <string>
#include <optional>

using namespace pq;
using namespace pq::core;

class QueryResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test QueryResult with null PGresult
TEST_F(QueryResultTest, NullResult) {
    QueryResult result(PgResultPtr(nullptr));
    
    EXPECT_FALSE(result.isValid());
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.rowCount(), 0);
    EXPECT_EQ(result.columnCount(), 0);
    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.affectedRows(), 0);
    EXPECT_EQ(result.status(), PGRES_FATAL_ERROR);
    EXPECT_STREQ(result.errorMessage(), "No result");
    EXPECT_STREQ(result.sqlState(), "");
}

// Test QueryResult empty state
TEST_F(QueryResultTest, EmptyResult) {
    QueryResult result(PgResultPtr(nullptr));
    
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(result.first().has_value());
}

// Test QueryResult column operations with null
TEST_F(QueryResultTest, NullResultColumnOperations) {
    QueryResult result(PgResultPtr(nullptr));
    
    EXPECT_STREQ(result.columnName(0), "");
    EXPECT_EQ(result.columnIndex("test"), -1);
    EXPECT_EQ(result.columnType(0), 0u);
    
    auto names = result.columnNames();
    EXPECT_TRUE(names.empty());
}

// Test QueryResult row access bounds checking
TEST_F(QueryResultTest, RowAccessBoundsCheck) {
    QueryResult result(PgResultPtr(nullptr));
    
    EXPECT_THROW(result.row(0), std::out_of_range);
    EXPECT_THROW(result.row(-1), std::out_of_range);
    EXPECT_THROW(result[0], std::out_of_range);
}

// Test QueryResult move semantics
TEST_F(QueryResultTest, MoveSemantics) {
    QueryResult result1(PgResultPtr(nullptr));
    QueryResult result2 = std::move(result1);
    
    EXPECT_FALSE(result2.isValid());
    EXPECT_EQ(result2.rowCount(), 0);
}

// Test RowIterator equality
TEST_F(QueryResultTest, RowIteratorEquality) {
    QueryResult result(PgResultPtr(nullptr));
    
    auto begin = result.begin();
    auto end = result.end();
    
    // Empty result: begin == end
    EXPECT_EQ(begin, end);
}

// Test RowIterator inequality
TEST_F(QueryResultTest, RowIteratorInequality) {
    // With null result, begin and end should be equal (both at row 0, count 0)
    QueryResult result(PgResultPtr(nullptr));
    
    auto it1 = result.begin();
    auto it2 = result.end();
    
    EXPECT_FALSE(it1 != it2);  // They should be equal
}

// Test range-based for loop with empty result
TEST_F(QueryResultTest, RangeBasedForEmpty) {
    QueryResult result(PgResultPtr(nullptr));
    
    int count = 0;
    for ([[maybe_unused]] const auto& row : result) {
        ++count;
    }
    
    EXPECT_EQ(count, 0);
}

// Test QueryResult raw pointer access
TEST_F(QueryResultTest, RawPointerAccess) {
    QueryResult result(PgResultPtr(nullptr));
    
    EXPECT_EQ(result.raw(), nullptr);
}

// Test Row column index lookup
TEST_F(QueryResultTest, RowColumnIndexLookup) {
    // Note: Can't test with real data without a connection,
    // but we can verify the interface exists and compiles
    QueryResult result(PgResultPtr(nullptr));
    
    // This test mainly verifies compilation
    EXPECT_TRUE(true);
}

// Test PgTypeTraits integration with Row::get
TEST_F(QueryResultTest, TypeTraitsIntegration) {
    // Verify that Row::get template compiles with various types
    // Actual runtime testing requires a real database connection
    
    // Test that the template instantiates correctly
    // (compile-time check)
    using IntType = decltype(std::declval<Row>().get<int>(0));
    using StringType = decltype(std::declval<Row>().get<std::string>(0));
    using OptIntType = decltype(std::declval<Row>().get<std::optional<int>>(0));
    
    EXPECT_TRUE((std::is_same_v<IntType, int>));
    EXPECT_TRUE((std::is_same_v<StringType, std::string>));
    EXPECT_TRUE((std::is_same_v<OptIntType, std::optional<int>>));
}

// Test RowIterator increment operators
TEST_F(QueryResultTest, RowIteratorIncrement) {
    // Test post-increment returns old value
    QueryResult result(PgResultPtr(nullptr));
    
    auto it = result.begin();
    auto old = it++;
    
    // Both should be at end for empty result
    EXPECT_EQ(old, result.begin());
}

// Test QueryResult status values
TEST_F(QueryResultTest, StatusValues) {
    QueryResult result(PgResultPtr(nullptr));
    
    // Null result should return PGRES_FATAL_ERROR
    EXPECT_EQ(result.status(), PGRES_FATAL_ERROR);
}
