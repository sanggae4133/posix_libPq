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
