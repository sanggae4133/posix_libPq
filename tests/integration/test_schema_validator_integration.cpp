/**
 * @file test_schema_validator_integration.cpp
 * @brief Integration tests for SchemaValidator and repository auto-validation
 */

#include <gtest/gtest.h>
#include <pq/pq.hpp>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <optional>
#include <string>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

namespace {
constexpr const char* kConnEnv = "PQ_TEST_CONN_STR";

bool hasIssueType(const std::vector<ValidationIssue>& issues, ValidationIssueType type) {
    return std::any_of(issues.begin(), issues.end(), [type](const ValidationIssue& issue) {
        return issue.type == type;
    });
}
} // namespace

struct SchemaValidEntity {
    int id{0};
    std::string code;
    std::optional<std::string> note;

    PQ_ENTITY(SchemaValidEntity, "it_schema_valid")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN_VARCHAR(code, "code", 16, PQ_NOT_NULL)
        PQ_COLUMN(note, "note", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaValidEntity)

struct SchemaMissingTableEntity {
    int id{0};

    PQ_ENTITY(SchemaMissingTableEntity, "it_schema_missing_table")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaMissingTableEntity)

struct SchemaMissingColumnEntity {
    int id{0};
    std::string name;

    PQ_ENTITY(SchemaMissingColumnEntity, "it_schema_missing_column")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaMissingColumnEntity)

struct SchemaTypeMismatchEntity {
    int id{0};
    int quantity{0};

    PQ_ENTITY(SchemaTypeMismatchEntity, "it_schema_type_mismatch")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaTypeMismatchEntity)

struct SchemaNullableMismatchEntity {
    int id{0};
    std::optional<std::string> note;

    PQ_ENTITY(SchemaNullableMismatchEntity, "it_schema_nullable_mismatch")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(note, "note", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaNullableMismatchEntity)

struct SchemaLengthMismatchEntity {
    int id{0};
    std::string code;

    PQ_ENTITY(SchemaLengthMismatchEntity, "it_schema_length_mismatch")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN_VARCHAR(code, "code", 12, PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaLengthMismatchEntity)

struct SchemaExtraColumnEntity {
    int id{0};
    std::string name;

    PQ_ENTITY(SchemaExtraColumnEntity, "it_schema_extra_column")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaExtraColumnEntity)

struct SchemaAutoValidationStrictEntity {
    int id{0};
    int quantity{0};

    PQ_ENTITY(SchemaAutoValidationStrictEntity, "it_schema_auto_validation_strict")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaAutoValidationStrictEntity)

struct SchemaAutoValidationLenientEntity {
    int id{0};
    int quantity{0};

    PQ_ENTITY(SchemaAutoValidationLenientEntity, "it_schema_auto_validation_lenient")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(SchemaAutoValidationLenientEntity)

class SchemaValidatorIntegrationTest : public ::testing::Test {
protected:
    Connection conn_;
    bool connected_{false};

    void SetUp() override {
        const char* connStr = std::getenv(kConnEnv);
        if (!connStr || std::string(connStr).empty()) {
            GTEST_SKIP() << "Set PQ_TEST_CONN_STR to run integration tests.";
        }

        auto connected = conn_.connect(connStr);
        if (!connected) {
            GTEST_SKIP() << "Unable to connect to PostgreSQL for integration tests: "
                         << connected.error().message;
        }
        connected_ = true;
    }

    void TearDown() override {
        if (!connected_) {
            return;
        }

        constexpr std::array<const char*, 9> tables{
            "it_schema_valid",
            "it_schema_missing_column",
            "it_schema_type_mismatch",
            "it_schema_nullable_mismatch",
            "it_schema_length_mismatch",
            "it_schema_extra_column",
            "it_schema_auto_validation_strict",
            "it_schema_auto_validation_lenient",
            "it_schema_missing_table"};

        for (const auto* table : tables) {
            auto dropResult = conn_.execute(std::string("DROP TABLE IF EXISTS ") + table);
            (void)dropResult;
        }
    }

    void execOrFail(std::string_view sql) {
        auto result = conn_.execute(sql);
        ASSERT_TRUE(result.hasValue()) << result.error().message;
    }
};

TEST_F(SchemaValidatorIntegrationTest, StrictModePassesWhenSchemaMatches) {
    execOrFail("DROP TABLE IF EXISTS it_schema_valid");
    execOrFail(
        "CREATE TABLE it_schema_valid ("
        "id INTEGER PRIMARY KEY, "
        "code VARCHAR(16) NOT NULL, "
        "note TEXT)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaValidEntity>(conn_);

    EXPECT_TRUE(result.isValid());
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(result.warnings.empty());
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsTableNotFound) {
    execOrFail("DROP TABLE IF EXISTS it_schema_missing_table");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaMissingTableEntity>(conn_);

    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(hasIssueType(result.errors, ValidationIssueType::TableNotFound));
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsMissingColumn) {
    execOrFail("DROP TABLE IF EXISTS it_schema_missing_column");
    execOrFail(
        "CREATE TABLE it_schema_missing_column ("
        "id INTEGER PRIMARY KEY)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaMissingColumnEntity>(conn_);

    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(hasIssueType(result.errors, ValidationIssueType::ColumnNotFound));
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsTypeMismatch) {
    execOrFail("DROP TABLE IF EXISTS it_schema_type_mismatch");
    execOrFail(
        "CREATE TABLE it_schema_type_mismatch ("
        "id INTEGER PRIMARY KEY, "
        "quantity TEXT NOT NULL)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaTypeMismatchEntity>(conn_);

    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(hasIssueType(result.errors, ValidationIssueType::TypeMismatch));
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsNullableMismatch) {
    execOrFail("DROP TABLE IF EXISTS it_schema_nullable_mismatch");
    execOrFail(
        "CREATE TABLE it_schema_nullable_mismatch ("
        "id INTEGER PRIMARY KEY, "
        "note TEXT NOT NULL)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaNullableMismatchEntity>(conn_);

    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(hasIssueType(result.errors, ValidationIssueType::NullableMismatch));
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsLengthMismatch) {
    execOrFail("DROP TABLE IF EXISTS it_schema_length_mismatch");
    execOrFail(
        "CREATE TABLE it_schema_length_mismatch ("
        "id INTEGER PRIMARY KEY, "
        "code VARCHAR(30) NOT NULL)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaLengthMismatchEntity>(conn_);

    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(hasIssueType(result.errors, ValidationIssueType::LengthMismatch));
}

TEST_F(SchemaValidatorIntegrationTest, StrictModeReportsExtraColumnsAsWarnings) {
    execOrFail("DROP TABLE IF EXISTS it_schema_extra_column");
    execOrFail(
        "CREATE TABLE it_schema_extra_column ("
        "id INTEGER PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "extra_col TEXT)");

    SchemaValidator validator(SchemaValidationMode::Strict);
    auto result = validator.validate<SchemaExtraColumnEntity>(conn_);

    EXPECT_TRUE(result.isValid());
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(hasIssueType(result.warnings, ValidationIssueType::ExtraColumn));
}

TEST_F(SchemaValidatorIntegrationTest, LenientModeDowngradesMismatchesToWarnings) {
    execOrFail("DROP TABLE IF EXISTS it_schema_type_mismatch");
    execOrFail(
        "CREATE TABLE it_schema_type_mismatch ("
        "id INTEGER PRIMARY KEY, "
        "quantity TEXT NOT NULL)");

    SchemaValidator validator(SchemaValidationMode::Lenient);
    auto result = validator.validate<SchemaTypeMismatchEntity>(conn_);

    EXPECT_TRUE(result.isValid());
    EXPECT_TRUE(result.errors.empty());
    EXPECT_TRUE(hasIssueType(result.warnings, ValidationIssueType::TypeMismatch));
}

TEST_F(SchemaValidatorIntegrationTest, RepositoryAutoValidationStrictBlocksOnMismatch) {
    execOrFail("DROP TABLE IF EXISTS it_schema_auto_validation_strict");
    execOrFail(
        "CREATE TABLE it_schema_auto_validation_strict ("
        "id INTEGER PRIMARY KEY, "
        "quantity TEXT NOT NULL)");

    MapperConfig config;
    config.autoValidateSchema = true;
    config.schemaValidationMode = SchemaValidationMode::Strict;
    Repository<SchemaAutoValidationStrictEntity, int> repo(conn_, config);

    auto result = repo.findAll();

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Schema validation failed"), std::string::npos);
    EXPECT_NE(result.error().message.find("Column type mismatch"), std::string::npos);
}

TEST_F(SchemaValidatorIntegrationTest, RepositoryAutoValidationLenientAllowsOperation) {
    execOrFail("DROP TABLE IF EXISTS it_schema_auto_validation_lenient");
    execOrFail(
        "CREATE TABLE it_schema_auto_validation_lenient ("
        "id INTEGER PRIMARY KEY, "
        "quantity TEXT NOT NULL)");

    MapperConfig config;
    config.autoValidateSchema = true;
    config.schemaValidationMode = SchemaValidationMode::Lenient;
    Repository<SchemaAutoValidationLenientEntity, int> repo(conn_, config);

    auto count = repo.count();

    EXPECT_TRUE(count.hasValue()) << count.error().message;
    EXPECT_EQ(*count, 0);
}
