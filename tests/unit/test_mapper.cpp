/**
 * @file test_mapper.cpp
 * @brief Unit tests for EntityMapper and SqlBuilder classes
 */

#include <gtest/gtest.h>
#include <pq/orm/Mapper.hpp>
#include <pq/orm/Entity.hpp>
#include <string>
#include <optional>

using namespace pq;
using namespace pq::orm;

// Test entities for mapper tests
struct MapperTestUser {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    int age{0};
    
    PQ_ENTITY(MapperTestUser, "mapper_test_users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
        PQ_COLUMN(age, "age", PQ_NOT_NULL)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(MapperTestUser)

struct MapperTestProduct {
    int64_t productId{0};
    std::string productName;
    double price{0.0};
    bool active{true};
    std::optional<std::string> description;
    
    PQ_ENTITY(MapperTestProduct, "mapper_test_products")
        PQ_COLUMN(productId, "product_id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(productName, "product_name", PQ_NOT_NULL | PQ_UNIQUE)
        PQ_COLUMN(price, "price", PQ_NOT_NULL)
        PQ_COLUMN(active, "is_active")
        PQ_COLUMN(description, "description")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(MapperTestProduct)

// Entity without primary key for error testing
struct NoPkEntity {
    std::string value;
    
    PQ_ENTITY(NoPkEntity, "no_pk_table")
        PQ_COLUMN(value, "value")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(NoPkEntity)

class SqlBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class EntityMapperTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// SqlBuilder Tests
// ============================================================================

TEST_F(SqlBuilderTest, InsertSqlExcludesAutoIncrement) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.insertSql();
    
    // Should not include 'id' column (auto-increment)
    EXPECT_EQ(sql.find("(id,"), std::string::npos);
    EXPECT_NE(sql.find("INSERT INTO mapper_test_users"), std::string::npos);
    EXPECT_NE(sql.find("(name, email, age)"), std::string::npos);
    EXPECT_NE(sql.find("VALUES ($1, $2, $3)"), std::string::npos);
    EXPECT_NE(sql.find("RETURNING *"), std::string::npos);
}

TEST_F(SqlBuilderTest, InsertSqlIncludesAutoIncrement) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.insertSql(true);  // Include auto-increment
    
    EXPECT_NE(sql.find("(id, name, email, age)"), std::string::npos);
    EXPECT_NE(sql.find("VALUES ($1, $2, $3, $4)"), std::string::npos);
}

TEST_F(SqlBuilderTest, SelectAllSql) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.selectAllSql();
    
    EXPECT_EQ(sql, "SELECT * FROM mapper_test_users");
}

TEST_F(SqlBuilderTest, SelectByIdSql) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.selectByIdSql();
    
    EXPECT_EQ(sql, "SELECT * FROM mapper_test_users WHERE id = $1");
}

TEST_F(SqlBuilderTest, SelectByIdSqlProduct) {
    SqlBuilder<MapperTestProduct> builder;
    
    std::string sql = builder.selectByIdSql();
    
    EXPECT_EQ(sql, "SELECT * FROM mapper_test_products WHERE product_id = $1");
}

TEST_F(SqlBuilderTest, UpdateSql) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.updateSql();
    
    EXPECT_NE(sql.find("UPDATE mapper_test_users"), std::string::npos);
    EXPECT_NE(sql.find("SET name = $1, email = $2, age = $3"), std::string::npos);
    EXPECT_NE(sql.find("WHERE id = $4"), std::string::npos);
    EXPECT_NE(sql.find("RETURNING *"), std::string::npos);
}

TEST_F(SqlBuilderTest, UpdateSqlProduct) {
    SqlBuilder<MapperTestProduct> builder;
    
    std::string sql = builder.updateSql();
    
    EXPECT_NE(sql.find("UPDATE mapper_test_products"), std::string::npos);
    EXPECT_NE(sql.find("SET product_name = $1"), std::string::npos);
    EXPECT_NE(sql.find("WHERE product_id = $5"), std::string::npos);
}

TEST_F(SqlBuilderTest, DeleteSql) {
    SqlBuilder<MapperTestUser> builder;
    
    std::string sql = builder.deleteSql();
    
    EXPECT_EQ(sql, "DELETE FROM mapper_test_users WHERE id = $1");
}

TEST_F(SqlBuilderTest, DeleteSqlProduct) {
    SqlBuilder<MapperTestProduct> builder;
    
    std::string sql = builder.deleteSql();
    
    EXPECT_EQ(sql, "DELETE FROM mapper_test_products WHERE product_id = $1");
}

TEST_F(SqlBuilderTest, InsertParams) {
    SqlBuilder<MapperTestUser> builder;
    
    MapperTestUser user;
    user.id = 1;  // Should be ignored
    user.name = "John";
    user.email = "john@example.com";
    user.age = 30;
    
    auto params = builder.insertParams(user);
    
    // Should have 3 params (excluding auto-increment id)
    ASSERT_EQ(params.size(), 3u);
    EXPECT_EQ(params[0], "John");
    EXPECT_EQ(params[1], "john@example.com");
    EXPECT_EQ(params[2], "30");
}

TEST_F(SqlBuilderTest, InsertParamsWithNull) {
    SqlBuilder<MapperTestUser> builder;
    
    MapperTestUser user;
    user.name = "Jane";
    user.email = std::nullopt;  // NULL
    user.age = 25;
    
    auto params = builder.insertParams(user);
    
    ASSERT_EQ(params.size(), 3u);
    EXPECT_EQ(params[0], "Jane");
    EXPECT_TRUE(params[1].empty());  // NULL represented as empty
    EXPECT_EQ(params[2], "25");
}

TEST_F(SqlBuilderTest, InsertParamsIncludeAutoIncrement) {
    SqlBuilder<MapperTestUser> builder;
    
    MapperTestUser user;
    user.id = 100;
    user.name = "Test";
    user.email = std::nullopt;
    user.age = 20;
    
    auto params = builder.insertParams(user, true);
    
    ASSERT_EQ(params.size(), 4u);
    EXPECT_EQ(params[0], "100");
    EXPECT_EQ(params[1], "Test");
}

TEST_F(SqlBuilderTest, UpdateParams) {
    SqlBuilder<MapperTestUser> builder;
    
    MapperTestUser user;
    user.id = 42;
    user.name = "Updated";
    user.email = "updated@example.com";
    user.age = 35;
    
    auto params = builder.updateParams(user);
    
    // Should have 4 params: name, email, age, id (pk last)
    ASSERT_EQ(params.size(), 4u);
    EXPECT_EQ(params[0], "Updated");
    EXPECT_EQ(params[1], "updated@example.com");
    EXPECT_EQ(params[2], "35");
    EXPECT_EQ(params[3], "42");  // Primary key last
}

TEST_F(SqlBuilderTest, PrimaryKeyValue) {
    SqlBuilder<MapperTestUser> builder;
    
    MapperTestUser user;
    user.id = 999;
    
    std::string pkValue = builder.primaryKeyValue(user);
    
    EXPECT_EQ(pkValue, "999");
}

TEST_F(SqlBuilderTest, PrimaryKeyValueInt64) {
    SqlBuilder<MapperTestProduct> builder;
    
    MapperTestProduct product;
    product.productId = 123456789012345LL;
    
    std::string pkValue = builder.primaryKeyValue(product);
    
    EXPECT_EQ(pkValue, "123456789012345");
}

TEST_F(SqlBuilderTest, MetadataAccess) {
    SqlBuilder<MapperTestUser> builder;
    
    const auto& meta = builder.metadata();
    
    EXPECT_EQ(meta.tableName(), "mapper_test_users");
    EXPECT_EQ(meta.columns().size(), 4u);
}

TEST_F(SqlBuilderTest, NoPrimaryKeySelectById) {
    SqlBuilder<NoPkEntity> builder;
    
    EXPECT_THROW(builder.selectByIdSql(), std::logic_error);
}

TEST_F(SqlBuilderTest, NoPrimaryKeyUpdate) {
    SqlBuilder<NoPkEntity> builder;
    
    EXPECT_THROW(builder.updateSql(), std::logic_error);
}

TEST_F(SqlBuilderTest, NoPrimaryKeyDelete) {
    SqlBuilder<NoPkEntity> builder;
    
    EXPECT_THROW(builder.deleteSql(), std::logic_error);
}

TEST_F(SqlBuilderTest, NoPrimaryKeyValue) {
    SqlBuilder<NoPkEntity> builder;
    NoPkEntity entity;
    
    EXPECT_THROW(builder.primaryKeyValue(entity), std::logic_error);
}

// ============================================================================
// EntityMapper Tests
// ============================================================================

TEST_F(EntityMapperTest, DefaultConfig) {
    EntityMapper<MapperTestUser> mapper;
    
    const auto& meta = mapper.metadata();
    EXPECT_EQ(meta.tableName(), "mapper_test_users");
}

TEST_F(EntityMapperTest, CustomConfig) {
    MapperConfig config;
    config.strictColumnMapping = false;
    config.ignoreExtraColumns = true;
    
    EntityMapper<MapperTestUser> mapper(config);
    
    // Should not throw with custom config
    EXPECT_EQ(mapper.metadata().tableName(), "mapper_test_users");
}

TEST_F(EntityMapperTest, MetadataAccess) {
    EntityMapper<MapperTestUser> mapper;
    
    const auto& meta = mapper.metadata();
    
    EXPECT_EQ(meta.tableName(), "mapper_test_users");
    EXPECT_EQ(meta.columns().size(), 4u);
    
    const auto* pk = meta.primaryKey();
    ASSERT_NE(pk, nullptr);
    EXPECT_EQ(pk->info.columnName, "id");
}

// ============================================================================
// MappingException Tests
// ============================================================================

TEST_F(EntityMapperTest, MappingExceptionMessage) {
    MappingException ex("Test mapping error");
    
    EXPECT_STREQ(ex.what(), "Test mapping error");
}

TEST_F(EntityMapperTest, MappingExceptionInheritance) {
    MappingException ex("Error");
    
    // Should be catchable as std::runtime_error
    try {
        throw ex;
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "Error");
    }
}

// ============================================================================
// MapperConfig Tests
// ============================================================================

TEST_F(EntityMapperTest, DefaultMapperConfig) {
    MapperConfig config = defaultMapperConfig();
    
    // Default is strict mapping enabled
    EXPECT_TRUE(config.strictColumnMapping);
    EXPECT_FALSE(config.ignoreExtraColumns);
}

// ============================================================================
// Complex Entity Tests
// ============================================================================

TEST_F(SqlBuilderTest, ProductInsertSql) {
    SqlBuilder<MapperTestProduct> builder;
    
    std::string sql = builder.insertSql();
    
    // Verify column order and naming
    EXPECT_NE(sql.find("INSERT INTO mapper_test_products"), std::string::npos);
    EXPECT_NE(sql.find("product_name"), std::string::npos);
    EXPECT_NE(sql.find("price"), std::string::npos);
    EXPECT_NE(sql.find("is_active"), std::string::npos);
    EXPECT_NE(sql.find("description"), std::string::npos);
}

TEST_F(SqlBuilderTest, ProductInsertParams) {
    SqlBuilder<MapperTestProduct> builder;
    
    MapperTestProduct product;
    product.productId = 1;
    product.productName = "Widget";
    product.price = 19.99;
    product.active = true;
    product.description = "A great widget";
    
    auto params = builder.insertParams(product);
    
    // 4 params (excluding auto-increment productId)
    ASSERT_EQ(params.size(), 4u);
    EXPECT_EQ(params[0], "Widget");
    // Price will be string representation
    EXPECT_NE(params[1].find("19.99"), std::string::npos);
    EXPECT_EQ(params[2], "t");  // bool true
    EXPECT_EQ(params[3], "A great widget");
}

TEST_F(SqlBuilderTest, ProductInsertParamsNullDescription) {
    SqlBuilder<MapperTestProduct> builder;
    
    MapperTestProduct product;
    product.productName = "Gadget";
    product.price = 29.99;
    product.active = false;
    product.description = std::nullopt;
    
    auto params = builder.insertParams(product);
    
    ASSERT_EQ(params.size(), 4u);
    EXPECT_EQ(params[2], "f");  // bool false
    EXPECT_TRUE(params[3].empty());  // NULL description
}
