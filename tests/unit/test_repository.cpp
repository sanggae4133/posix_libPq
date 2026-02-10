/**
 * @file test_repository.cpp
 * @brief Unit tests for Repository template behavior
 */

#include <gtest/gtest.h>
#include <pq/orm/Repository.hpp>
#include <pq/orm/Entity.hpp>
#include <pq/core/Connection.hpp>
#include <string>
#include <string_view>
#include <tuple>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

struct StringPkEntity {
    std::string id;
    std::string name;

    PQ_ENTITY(StringPkEntity, "string_pk_entities")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(StringPkEntity)

struct CompositePkEntity {
    int orderId{0};
    int productId{0};
    std::string name;

    PQ_ENTITY(CompositePkEntity, "composite_pk_entities")
        PQ_COLUMN(orderId, "order_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(productId, "product_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(CompositePkEntity)

class RepositoryTemplateTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RepositoryTemplateTest, StringPrimaryKeyFindById) {
    Connection conn;
    Repository<StringPkEntity, std::string> repo(conn);

    auto result = repo.findById(std::string{"user-1"});

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, StringViewPrimaryKeyExistsById) {
    Connection conn;
    Repository<StringPkEntity, std::string_view> repo(conn);

    auto result = repo.existsById(std::string_view{"user-2"});

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CStringPrimaryKeyRemoveById) {
    Connection conn;
    Repository<StringPkEntity, const char*> repo(conn);

    auto result = repo.removeById("user-3");

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyFindByIdTuple) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.findById(std::make_tuple(10, 20));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyFindByIdVariadic) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.findById(10, 20);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyExistsByIdVariadic) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.existsById(10, 20);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyExistsByIdTuple) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.existsById(std::make_tuple(10, 20));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyRemoveByIdVariadic) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.removeById(10, 20);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyRemoveByIdTuple) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    auto result = repo.removeById(std::make_tuple(10, 20));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyRemoveEntityUsesMultiPkPath) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int>> repo(conn);

    CompositePkEntity entity;
    entity.orderId = 10;
    entity.productId = 20;
    entity.name = "item";

    auto result = repo.remove(entity);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyScalarPkTypeReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, int> repo(conn);

    auto result = repo.findById(10);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Composite primary key entity requires tuple PK type"),
              std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyScalarPkTypeRemoveByIdReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, int> repo(conn);

    auto result = repo.removeById(10);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Composite primary key entity requires tuple PK type"),
              std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyScalarPkTypeExistsByIdReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, int> repo(conn);

    auto result = repo.existsById(10);

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Composite primary key entity requires tuple PK type"),
              std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyTupleSizeMismatchReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int, int>> repo(conn);

    auto result = repo.findById(std::make_tuple(10, 20, 30));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Primary key count mismatch"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyTupleSizeMismatchExistsReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int, int>> repo(conn);

    auto result = repo.existsById(std::make_tuple(10, 20, 30));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Primary key count mismatch"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, CompositePrimaryKeyTupleSizeMismatchRemoveReturnsError) {
    Connection conn;
    Repository<CompositePkEntity, std::tuple<int, int, int>> repo(conn);

    auto result = repo.removeById(std::make_tuple(10, 20, 30));

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Primary key count mismatch"), std::string::npos);
}

TEST_F(RepositoryTemplateTest, AutoSchemaValidationStrictBlocksOperationWhenValidationFails) {
    Connection conn;
    MapperConfig config;
    config.autoValidateSchema = true;
    config.schemaValidationMode = SchemaValidationMode::Strict;

    Repository<StringPkEntity, std::string> repo(conn, config);

    auto result = repo.findById(std::string{"user-1"});

    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Schema validation failed"), std::string::npos);
    EXPECT_NE(result.error().message.find("connection is not established"), std::string::npos);
}
