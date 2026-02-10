/**
 * @file test_repository_integration.cpp
 * @brief Integration tests for repository CRUD flows (real PostgreSQL)
 */

#include <gtest/gtest.h>
#include <pq/pq.hpp>
#include <cstdlib>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

namespace {
constexpr const char* kConnEnv = "PQ_TEST_CONN_STR";
} // namespace

struct IntegrationUser {
    int id{0};
    std::string name;
    std::optional<std::string> email;

    PQ_ENTITY(IntegrationUser, "it_users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(email, "email", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(IntegrationUser)

struct IntegrationOrderItem {
    int orderId{0};
    int productId{0};
    int quantity{0};
    std::optional<std::string> note;

    PQ_ENTITY(IntegrationOrderItem, "it_order_items")
        PQ_COLUMN(orderId, "order_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(productId, "product_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
        PQ_COLUMN(note, "note", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(IntegrationOrderItem)

class RepositoryIntegrationTest : public ::testing::Test {
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
        if (connected_) {
            // TEMP tables are session-scoped; explicit cleanup keeps retries deterministic.
            auto dropUsers = conn_.execute("DROP TABLE IF EXISTS it_users");
            auto dropOrderItems = conn_.execute("DROP TABLE IF EXISTS it_order_items");
            (void)dropUsers;
            (void)dropOrderItems;
        }
    }

    void execOrFail(std::string_view sql) {
        auto result = conn_.execute(sql);
        ASSERT_TRUE(result.hasValue()) << result.error().message;
    }
};

TEST_F(RepositoryIntegrationTest, SinglePrimaryKeyCrudFlow) {
    execOrFail("DROP TABLE IF EXISTS it_users");
    execOrFail(
        "CREATE TEMP TABLE it_users ("
        "id SERIAL PRIMARY KEY, "
        "name TEXT NOT NULL, "
        "email TEXT)");

    Repository<IntegrationUser, int> repo(conn_);

    IntegrationUser user;
    user.name = "alice";
    user.email = "alice@example.com";

    auto saved = repo.save(user);
    ASSERT_TRUE(saved.hasValue()) << saved.error().message;
    EXPECT_GT(saved->id, 0);
    EXPECT_EQ(saved->name, "alice");
    ASSERT_TRUE(saved->email.has_value());
    EXPECT_EQ(saved->email.value(), "alice@example.com");

    auto found = repo.findById(saved->id);
    ASSERT_TRUE(found.hasValue()) << found.error().message;
    ASSERT_TRUE(found->has_value());
    EXPECT_EQ(found->value().name, "alice");

    auto exists = repo.existsById(saved->id);
    ASSERT_TRUE(exists.hasValue()) << exists.error().message;
    EXPECT_TRUE(*exists);

    IntegrationUser updated = *saved;
    updated.name = "alice-updated";
    updated.email = std::nullopt;

    auto updateResult = repo.update(updated);
    ASSERT_TRUE(updateResult.hasValue()) << updateResult.error().message;
    EXPECT_EQ(updateResult->name, "alice-updated");
    EXPECT_FALSE(updateResult->email.has_value());

    auto removed = repo.remove(updated);
    ASSERT_TRUE(removed.hasValue()) << removed.error().message;
    EXPECT_EQ(*removed, 1);

    auto existsAfterRemove = repo.existsById(saved->id);
    ASSERT_TRUE(existsAfterRemove.hasValue()) << existsAfterRemove.error().message;
    EXPECT_FALSE(*existsAfterRemove);

    auto count = repo.count();
    ASSERT_TRUE(count.hasValue()) << count.error().message;
    EXPECT_EQ(*count, 0);
}

TEST_F(RepositoryIntegrationTest, CompositePrimaryKeyCrudFlow) {
    execOrFail("DROP TABLE IF EXISTS it_order_items");
    execOrFail(
        "CREATE TEMP TABLE it_order_items ("
        "order_id INTEGER NOT NULL, "
        "product_id INTEGER NOT NULL, "
        "quantity INTEGER NOT NULL, "
        "note TEXT, "
        "PRIMARY KEY(order_id, product_id))");

    Repository<IntegrationOrderItem, std::tuple<int, int>> repo(conn_);

    IntegrationOrderItem item;
    item.orderId = 1001;
    item.productId = 42;
    item.quantity = 2;
    item.note = "fragile";

    auto saved = repo.save(item);
    ASSERT_TRUE(saved.hasValue()) << saved.error().message;
    EXPECT_EQ(saved->orderId, 1001);
    EXPECT_EQ(saved->productId, 42);
    EXPECT_EQ(saved->quantity, 2);
    ASSERT_TRUE(saved->note.has_value());
    EXPECT_EQ(saved->note.value(), "fragile");

    auto foundVariadic = repo.findById(1001, 42);
    ASSERT_TRUE(foundVariadic.hasValue()) << foundVariadic.error().message;
    ASSERT_TRUE(foundVariadic->has_value());
    EXPECT_EQ(foundVariadic->value().quantity, 2);

    auto foundTuple = repo.findById(std::make_tuple(1001, 42));
    ASSERT_TRUE(foundTuple.hasValue()) << foundTuple.error().message;
    ASSERT_TRUE(foundTuple->has_value());
    EXPECT_EQ(foundTuple->value().quantity, 2);

    IntegrationOrderItem updated = foundTuple->value();
    updated.quantity = 9;
    updated.note = std::nullopt;

    auto updatedResult = repo.update(updated);
    ASSERT_TRUE(updatedResult.hasValue()) << updatedResult.error().message;
    EXPECT_EQ(updatedResult->quantity, 9);
    EXPECT_FALSE(updatedResult->note.has_value());

    auto exists = repo.existsById(1001, 42);
    ASSERT_TRUE(exists.hasValue()) << exists.error().message;
    EXPECT_TRUE(*exists);

    auto removedByTuple = repo.removeById(std::make_tuple(1001, 42));
    ASSERT_TRUE(removedByTuple.hasValue()) << removedByTuple.error().message;
    EXPECT_EQ(*removedByTuple, 1);

    auto existsAfterTupleRemove = repo.existsById(1001, 42);
    ASSERT_TRUE(existsAfterTupleRemove.hasValue()) << existsAfterTupleRemove.error().message;
    EXPECT_FALSE(*existsAfterTupleRemove);

    // Re-insert and verify remove(entity) path for composite PK
    auto savedAgain = repo.save(item);
    ASSERT_TRUE(savedAgain.hasValue()) << savedAgain.error().message;

    auto removedByEntity = repo.remove(*savedAgain);
    ASSERT_TRUE(removedByEntity.hasValue()) << removedByEntity.error().message;
    EXPECT_EQ(*removedByEntity, 1);
}
