/**
 * @file test_schema_validator.cpp
 * @brief Unit tests for schema validation result and disconnected behavior
 */

#include <gtest/gtest.h>
#include <pq/orm/SchemaValidator.hpp>
#include <pq/orm/Entity.hpp>
#include <pq/core/Connection.hpp>
#include <string>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

struct SchemaUnitEntity {
    int id{0};
    std::string name;

    PQ_ENTITY(SchemaUnitEntity, "schema_unit_entities")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaUnitEntity)

TEST(SchemaValidatorUnitTest, ModeIsConfigurable) {
    SchemaValidator strictValidator(SchemaValidationMode::Strict);
    SchemaValidator lenientValidator(SchemaValidationMode::Lenient);

    EXPECT_EQ(strictValidator.mode(), SchemaValidationMode::Strict);
    EXPECT_EQ(lenientValidator.mode(), SchemaValidationMode::Lenient);
}

TEST(SchemaValidatorUnitTest, ValidateDisconnectedConnectionReturnsConnectionError) {
    Connection conn;  // Intentionally disconnected
    SchemaValidator validator(SchemaValidationMode::Strict);

    auto result = validator.validate<SchemaUnitEntity>(conn);

    EXPECT_FALSE(result.isValid());
    ASSERT_EQ(result.errors.size(), 1u);
    EXPECT_EQ(result.errors[0].type, ValidationIssueType::ConnectionError);
    EXPECT_NE(result.errors[0].message.find("connection is not established"), std::string::npos);
}

TEST(SchemaValidatorUnitTest, ValidationResultSummaryContainsCountsAndFirstIssue) {
    ValidationResult result;
    result.errors.push_back(ValidationIssue{
        ValidationIssueType::TypeMismatch,
        "EntityA",
        "table_a",
        "amount",
        "numeric",
        "text",
        "Column type mismatch"});
    result.warnings.push_back(ValidationIssue{
        ValidationIssueType::ExtraColumn,
        "EntityA",
        "table_a",
        "extra_col",
        "<not mapped>",
        "extra_col",
        "Database has extra column not mapped in entity"});

    const auto summary = result.summary();

    EXPECT_NE(summary.find("errors=1"), std::string::npos);
    EXPECT_NE(summary.find("warnings=1"), std::string::npos);
    EXPECT_NE(summary.find("first_error"), std::string::npos);
}
