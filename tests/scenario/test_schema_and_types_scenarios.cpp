/**
 * @file test_schema_and_types_scenarios.cpp
 * @brief Scenario tests for schema validation and extended types without external DB
 */

#include <gtest/gtest.h>
#include <pq/pq.hpp>
#include <optional>
#include <string>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

struct ScenarioSchemaEntity {
    int id{0};
    std::string name;

    PQ_ENTITY(ScenarioSchemaEntity, "scenario_schema_entities")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(ScenarioSchemaEntity)

TEST(SchemaScenarioTest, ManualSchemaValidationReturnsStructuredConnectionError) {
    Connection conn;  // Intentionally disconnected
    SchemaValidator validator(SchemaValidationMode::Strict);

    auto validation = validator.validate<ScenarioSchemaEntity>(conn);

    EXPECT_FALSE(validation.isValid());
    ASSERT_EQ(validation.errors.size(), 1u);
    EXPECT_EQ(validation.errors[0].type, ValidationIssueType::ConnectionError);
    EXPECT_NE(validation.errors[0].message.find("connection is not established"),
              std::string::npos);
}

TEST(SchemaScenarioTest, RepositoryAutoValidationCanBeEnabledViaConfig) {
    Connection conn;  // Intentionally disconnected
    MapperConfig config;
    config.autoValidateSchema = true;
    config.schemaValidationMode = SchemaValidationMode::Strict;

    Repository<ScenarioSchemaEntity, int> repo(conn, config);
    auto result = repo.findAll();

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Schema validation failed"), std::string::npos);
}

TEST(ExtendedTypeScenarioTest, TypeTraitsProvideExpectedRoundTripRepresentations) {
    const auto timestamp = std::chrono::system_clock::time_point{} +
                           std::chrono::milliseconds(1739186705123LL);
    const auto timestampText =
        PgTypeTraits<std::chrono::system_clock::time_point>::toString(timestamp);
    EXPECT_EQ(PgTypeTraits<std::chrono::system_clock::time_point>::fromString(timestampText.c_str()),
              timestamp);

    TimestampTz tzValue{timestamp, 9 * 60};
    const auto tzText = PgTypeTraits<TimestampTz>::toString(tzValue);
    const auto tzParsed = PgTypeTraits<TimestampTz>::fromString(tzText.c_str());
    EXPECT_EQ(tzParsed.timePoint, tzValue.timePoint);

    Date date{2026, 2, 10};
    Time time{11, 22, 33, 444};
    Numeric amount{"12345.678901234567"};
    Uuid uuid{"550e8400-e29b-41d4-a716-446655440000"};
    Jsonb json{R"({"ok":true})"};

    EXPECT_EQ(PgTypeTraits<Date>::fromString(PgTypeTraits<Date>::toString(date).c_str()), date);
    EXPECT_EQ(PgTypeTraits<Time>::fromString(PgTypeTraits<Time>::toString(time).c_str()), time);
    EXPECT_EQ(PgTypeTraits<Numeric>::fromString(PgTypeTraits<Numeric>::toString(amount).c_str()),
              amount);
    EXPECT_EQ(PgTypeTraits<Uuid>::fromString(PgTypeTraits<Uuid>::toString(uuid).c_str()), uuid);
    EXPECT_EQ(PgTypeTraits<Jsonb>::fromString(PgTypeTraits<Jsonb>::toString(json).c_str()), json);
}
