/**
 * @file test_entity.cpp
 * @brief Unit tests for Entity mapping system
 */

#include <gtest/gtest.h>
#include <pq/orm/Entity.hpp>
#include <string>
#include <optional>

using namespace pq;
using namespace pq::orm;

// Test entity for unit tests
struct TestUser {
    int id{0};
    std::string name;
    std::optional<std::string> email;
    
    PQ_ENTITY(TestUser, "test_users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(TestUser)

// Another test entity
struct TestProduct {
    int64_t id{0};
    std::string name;
    double price{0.0};
    bool active{true};
    
    PQ_ENTITY(TestProduct, "test_products")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL | PQ_UNIQUE)
        PQ_COLUMN(price, "price", PQ_NOT_NULL)
        PQ_COLUMN(active, "is_active")
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(TestProduct)

class EntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// T025: Unit test for ColumnFlags enum bitwise operations
TEST_F(EntityTest, ColumnFlagsBitwiseOperations) {
    auto flags = ColumnFlags::PrimaryKey | ColumnFlags::AutoIncrement;
    
    EXPECT_TRUE(hasFlag(flags, ColumnFlags::PrimaryKey));
    EXPECT_TRUE(hasFlag(flags, ColumnFlags::AutoIncrement));
    EXPECT_FALSE(hasFlag(flags, ColumnFlags::NotNull));
    EXPECT_FALSE(hasFlag(flags, ColumnFlags::Unique));
    
    auto combined = ColumnFlags::NotNull | ColumnFlags::Unique | ColumnFlags::Index;
    EXPECT_TRUE(hasFlag(combined, ColumnFlags::NotNull));
    EXPECT_TRUE(hasFlag(combined, ColumnFlags::Unique));
    EXPECT_TRUE(hasFlag(combined, ColumnFlags::Index));
    EXPECT_FALSE(hasFlag(combined, ColumnFlags::PrimaryKey));
}

// T026: Unit test for ColumnInfo metadata extraction
TEST_F(EntityTest, ColumnInfoMetadataExtraction) {
    const auto& meta = EntityMeta<TestUser>::metadata();
    const auto& columns = meta.columns();
    
    EXPECT_EQ(columns.size(), 3);
    
    // Check id column
    EXPECT_EQ(columns[0].info.fieldName, "id");
    EXPECT_EQ(columns[0].info.columnName, "id");
    EXPECT_EQ(columns[0].info.pgType, oid::INT4);
    EXPECT_TRUE(columns[0].info.isPrimaryKey());
    EXPECT_TRUE(columns[0].info.isAutoIncrement());
    
    // Check name column
    EXPECT_EQ(columns[1].info.fieldName, "name");
    EXPECT_EQ(columns[1].info.columnName, "name");
    EXPECT_EQ(columns[1].info.pgType, oid::TEXT);
    EXPECT_FALSE(columns[1].info.isPrimaryKey());
    
    // Check email column (nullable)
    EXPECT_EQ(columns[2].info.fieldName, "email");
    EXPECT_EQ(columns[2].info.columnName, "email");
    EXPECT_TRUE(columns[2].info.isNullable);
}

// T027: Unit test for EntityMetadata table name and columns
TEST_F(EntityTest, EntityMetadataTableNameAndColumns) {
    const auto& meta = EntityMeta<TestUser>::metadata();
    
    EXPECT_EQ(meta.tableName(), "test_users");
    EXPECT_EQ(meta.columns().size(), 3);
    
    // Verify primary key
    const auto* pk = meta.primaryKey();
    ASSERT_NE(pk, nullptr);
    EXPECT_EQ(pk->info.columnName, "id");
    EXPECT_TRUE(pk->info.isPrimaryKey());
}

// T028: Unit test for PQ_ENTITY macro table registration
TEST_F(EntityTest, PqEntityMacroTableRegistration) {
    // TestUser should be registered
    EXPECT_EQ(EntityMeta<TestUser>::tableName, "test_users");
    
    // TestProduct should also be registered
    EXPECT_EQ(EntityMeta<TestProduct>::tableName, "test_products");
}

// T029: Unit test for PQ_COLUMN macro with PQ_PRIMARY_KEY flag
TEST_F(EntityTest, PqColumnMacroPrimaryKeyFlag) {
    const auto& meta = EntityMeta<TestProduct>::metadata();
    const auto* pk = meta.primaryKey();
    
    ASSERT_NE(pk, nullptr);
    EXPECT_EQ(pk->info.columnName, "id");
    EXPECT_TRUE(pk->info.isPrimaryKey());
    EXPECT_TRUE(pk->info.isAutoIncrement());
}

// T030: Unit test for PQ_REGISTER_ENTITY metadata access
TEST_F(EntityTest, PqRegisterEntityMetadataAccess) {
    // Verify IsEntity trait works correctly
    EXPECT_TRUE(isEntityV<TestUser>);
    EXPECT_TRUE(isEntityV<TestProduct>);
    
    // Non-registered types should not be entities
    struct NotAnEntity { int x; };
    EXPECT_FALSE(isEntityV<NotAnEntity>);
    
    // Access metadata via EntityMeta
    auto& userMeta = EntityMeta<TestUser>::metadata();
    EXPECT_EQ(userMeta.tableName(), "test_users");
    
    auto& productMeta = EntityMeta<TestProduct>::metadata();
    EXPECT_EQ(productMeta.tableName(), "test_products");
}

// Additional tests for column value extraction
TEST_F(EntityTest, ColumnValueExtraction) {
    TestUser user;
    user.id = 42;
    user.name = "John Doe";
    user.email = "john@example.com";
    
    const auto& meta = EntityMeta<TestUser>::metadata();
    const auto& columns = meta.columns();
    
    // Test toString extraction
    EXPECT_EQ(columns[0].toString(user), "42");
    EXPECT_EQ(columns[1].toString(user), "John Doe");
    EXPECT_EQ(columns[2].toString(user), "john@example.com");
}

TEST_F(EntityTest, ColumnNullableHandling) {
    TestUser user;
    user.id = 1;
    user.name = "Test";
    user.email = std::nullopt;  // NULL
    
    const auto& meta = EntityMeta<TestUser>::metadata();
    const auto& columns = meta.columns();
    
    // Check isNull function
    EXPECT_FALSE(columns[0].isNull(user));  // id
    EXPECT_FALSE(columns[1].isNull(user));  // name
    EXPECT_TRUE(columns[2].isNull(user));   // email (nullopt)
}

TEST_F(EntityTest, ColumnFromString) {
    TestUser user;
    const auto& meta = EntityMeta<TestUser>::metadata();
    const auto& columns = meta.columns();
    
    // Set values from strings
    columns[0].fromString(user, "100");
    columns[1].fromString(user, "Jane Doe");
    columns[2].fromString(user, "jane@example.com");
    
    EXPECT_EQ(user.id, 100);
    EXPECT_EQ(user.name, "Jane Doe");
    EXPECT_EQ(user.email.value(), "jane@example.com");
    
    // Test NULL handling for optional field
    columns[2].fromString(user, nullptr);
    EXPECT_FALSE(user.email.has_value());
}

TEST_F(EntityTest, FindColumnByName) {
    const auto& meta = EntityMeta<TestUser>::metadata();
    
    const auto* idCol = meta.findColumn("id");
    ASSERT_NE(idCol, nullptr);
    EXPECT_EQ(idCol->info.fieldName, "id");
    
    const auto* nameCol = meta.findColumn("name");
    ASSERT_NE(nameCol, nullptr);
    EXPECT_EQ(nameCol->info.fieldName, "name");
    
    const auto* nonExistent = meta.findColumn("nonexistent");
    EXPECT_EQ(nonExistent, nullptr);
}

TEST_F(EntityTest, MultipleTypeSupport) {
    TestProduct product;
    product.id = 12345678901234LL;
    product.name = "Widget";
    product.price = 19.99;
    product.active = true;
    
    const auto& meta = EntityMeta<TestProduct>::metadata();
    const auto& columns = meta.columns();
    
    // Verify int64_t
    EXPECT_EQ(columns[0].info.pgType, oid::INT8);
    EXPECT_EQ(columns[0].toString(product), "12345678901234");
    
    // Verify double
    EXPECT_EQ(columns[2].info.pgType, oid::FLOAT8);
    
    // Verify bool
    EXPECT_EQ(columns[3].info.pgType, oid::BOOL);
    EXPECT_EQ(columns[3].toString(product), "t");
}
