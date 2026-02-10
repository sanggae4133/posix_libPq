/**
 * @file test_composite_pk_scenarios.cpp
 * @brief Scenario tests aligned with composite PK specification stories
 */

#include <gtest/gtest.h>
#include <pq/orm/Mapper.hpp>
#include <pq/orm/Repository.hpp>
#include <tuple>
#include <optional>
#include <string>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

struct ScenarioOrderItem {
    int orderId{0};
    int productId{0};
    int quantity{0};
    std::optional<std::string> note;

    PQ_ENTITY(ScenarioOrderItem, "scenario_order_items")
        PQ_COLUMN(orderId, "order_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(productId, "product_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
        PQ_COLUMN(note, "note", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(ScenarioOrderItem)

struct ScenarioUser {
    int id{0};
    std::string name;
    std::optional<std::string> email;

    PQ_ENTITY(ScenarioUser, "scenario_users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(ScenarioUser)

TEST(CompositePkScenarioTest, SelectByIdSqlMatchesSpecificationForm) {
    SqlBuilder<ScenarioOrderItem> builder;

    EXPECT_EQ(builder.selectByIdSql(),
              "SELECT * FROM scenario_order_items WHERE order_id = $1 AND product_id = $2");
}

TEST(CompositePkScenarioTest, UpdateUsesAllPrimaryKeysInWhereAndParams) {
    SqlBuilder<ScenarioOrderItem> builder;
    ScenarioOrderItem item;
    item.orderId = 15;
    item.productId = 28;
    item.quantity = 3;
    item.note = "fragile";

    EXPECT_EQ(builder.updateSql(),
              "UPDATE scenario_order_items SET quantity = $1, note = $2 "
              "WHERE order_id = $3 AND product_id = $4 RETURNING *");

    auto params = builder.updateParams(item);
    ASSERT_EQ(params.size(), 4u);
    EXPECT_EQ(*params[0], "3");
    EXPECT_EQ(*params[1], "fragile");
    EXPECT_EQ(*params[2], "15");
    EXPECT_EQ(*params[3], "28");
}

TEST(CompositePkScenarioTest, RepositorySupportsVariadicCompositeFindExistsRemoveById) {
    Connection conn;  // Intentionally unconnected: we only validate query path + API shape
    Repository<ScenarioOrderItem, std::tuple<int, int>> repo(conn);

    auto findResult = repo.findById(15, 28);
    ASSERT_TRUE(findResult.hasError());
    EXPECT_NE(findResult.error().message.find("Not connected"), std::string::npos);

    auto existsResult = repo.existsById(15, 28);
    ASSERT_TRUE(existsResult.hasError());
    EXPECT_NE(existsResult.error().message.find("Not connected"), std::string::npos);

    auto removeResult = repo.removeById(15, 28);
    ASSERT_TRUE(removeResult.hasError());
    EXPECT_NE(removeResult.error().message.find("Not connected"), std::string::npos);
}

TEST(CompositePkScenarioTest, SinglePrimaryKeyBehaviorRemainsBackwardCompatible) {
    SqlBuilder<ScenarioUser> builder;
    ScenarioUser user;
    user.id = 9;
    user.name = "legacy";
    user.email = std::nullopt;

    EXPECT_EQ(builder.selectByIdSql(), "SELECT * FROM scenario_users WHERE id = $1");
    EXPECT_EQ(builder.deleteSql(), "DELETE FROM scenario_users WHERE id = $1");

    auto updateParams = builder.updateParams(user);
    ASSERT_EQ(updateParams.size(), 3u);
    EXPECT_EQ(*updateParams[0], "legacy");
    EXPECT_FALSE(updateParams[1].has_value());
    EXPECT_EQ(*updateParams[2], "9");
}
