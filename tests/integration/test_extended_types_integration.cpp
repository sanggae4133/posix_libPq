/**
 * @file test_extended_types_integration.cpp
 * @brief Integration tests for extended PostgreSQL type mappings
 */

#include <gtest/gtest.h>
#include <pq/pq.hpp>
#include <cstdlib>
#include <optional>
#include <string>

using namespace pq;
using namespace pq::orm;
using namespace pq::core;

namespace {
constexpr const char* kConnEnv = "PQ_TEST_CONN_STR";
} // namespace

struct ExtendedTypesEntity {
    int id{0};
    std::chrono::system_clock::time_point createdAt;
    TimestampTz updatedAt;
    Date shipDate;
    Time shipTime;
    Numeric amount;
    Uuid externalId;
    Jsonb payload;
    std::optional<std::chrono::system_clock::time_point> optionalTs;
    std::optional<TimestampTz> optionalTstz;
    std::optional<Date> optionalDate;
    std::optional<Time> optionalTime;
    std::optional<Numeric> optionalAmount;
    std::optional<Uuid> optionalUuid;
    std::optional<Jsonb> optionalPayload;

    PQ_ENTITY(ExtendedTypesEntity, "it_extended_types")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(createdAt, "created_at", PQ_NOT_NULL)
        PQ_COLUMN(updatedAt, "updated_at", PQ_NOT_NULL)
        PQ_COLUMN(shipDate, "ship_date", PQ_NOT_NULL)
        PQ_COLUMN(shipTime, "ship_time", PQ_NOT_NULL)
        PQ_COLUMN(amount, "amount", PQ_NOT_NULL)
        PQ_COLUMN(externalId, "external_id", PQ_NOT_NULL)
        PQ_COLUMN(payload, "payload", PQ_NOT_NULL)
        PQ_COLUMN(optionalTs, "optional_ts", PQ_NONE)
        PQ_COLUMN(optionalTstz, "optional_tstz", PQ_NONE)
        PQ_COLUMN(optionalDate, "optional_date", PQ_NONE)
        PQ_COLUMN(optionalTime, "optional_time", PQ_NONE)
        PQ_COLUMN(optionalAmount, "optional_amount", PQ_NONE)
        PQ_COLUMN(optionalUuid, "optional_uuid", PQ_NONE)
        PQ_COLUMN(optionalPayload, "optional_payload", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(ExtendedTypesEntity)

class ExtendedTypesIntegrationTest : public ::testing::Test {
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
        auto dropResult = conn_.execute("DROP TABLE IF EXISTS it_extended_types");
        (void)dropResult;
    }

    void execOrFail(std::string_view sql) {
        auto result = conn_.execute(sql);
        ASSERT_TRUE(result.hasValue()) << result.error().message;
    }
};

TEST_F(ExtendedTypesIntegrationTest, RoundTripForExtendedTypesAndOptionalNullHandling) {
    execOrFail("DROP TABLE IF EXISTS it_extended_types");
    execOrFail(
        "CREATE TABLE it_extended_types ("
        "id INTEGER PRIMARY KEY, "
        "created_at TIMESTAMP NOT NULL, "
        "updated_at TIMESTAMPTZ NOT NULL, "
        "ship_date DATE NOT NULL, "
        "ship_time TIME NOT NULL, "
        "amount NUMERIC(30,15) NOT NULL, "
        "external_id UUID NOT NULL, "
        "payload JSONB NOT NULL, "
        "optional_ts TIMESTAMP NULL, "
        "optional_tstz TIMESTAMPTZ NULL, "
        "optional_date DATE NULL, "
        "optional_time TIME NULL, "
        "optional_amount NUMERIC(30,15) NULL, "
        "optional_uuid UUID NULL, "
        "optional_payload JSONB NULL)");

    Repository<ExtendedTypesEntity, int> repo(conn_);

    ExtendedTypesEntity entity;
    entity.id = 1;
    entity.createdAt = std::chrono::system_clock::time_point{} +
                       std::chrono::milliseconds(1739186705123LL);
    entity.updatedAt = TimestampTz{
        std::chrono::system_clock::time_point{} + std::chrono::milliseconds(1739186705123LL),
        9 * 60};
    entity.shipDate = Date{2026, 2, 10};
    entity.shipTime = Time{14, 5, 6, 789};
    entity.amount = Numeric{"123456789012345.123456789012345"};
    entity.externalId = Uuid{"550e8400-e29b-41d4-a716-446655440000"};
    entity.payload = Jsonb{R"({"type":"order","items":2})"};
    entity.optionalTs = std::nullopt;
    entity.optionalTstz = std::nullopt;
    entity.optionalDate = std::nullopt;
    entity.optionalTime = std::nullopt;
    entity.optionalAmount = std::nullopt;
    entity.optionalUuid = Uuid{"de305d54-75b4-431b-adb2-eb6b9e546014"};
    entity.optionalPayload = Jsonb{R"({"optional":true})"};

    auto saved = repo.save(entity);
    ASSERT_TRUE(saved.hasValue()) << saved.error().message;

    auto foundResult = repo.findById(1);
    ASSERT_TRUE(foundResult.hasValue()) << foundResult.error().message;
    ASSERT_TRUE(foundResult->has_value());

    const auto& found = foundResult->value();
    EXPECT_EQ(found.id, entity.id);
    EXPECT_EQ(found.createdAt, entity.createdAt);
    EXPECT_EQ(found.updatedAt.timePoint, entity.updatedAt.timePoint);
    EXPECT_EQ(found.shipDate, entity.shipDate);
    EXPECT_EQ(found.shipTime, entity.shipTime);
    EXPECT_EQ(found.amount.value, entity.amount.value);
    EXPECT_EQ(found.externalId.value, entity.externalId.value);
    EXPECT_NE(found.payload.value.find("\"type\""), std::string::npos);
    EXPECT_FALSE(found.optionalTs.has_value());
    EXPECT_FALSE(found.optionalTstz.has_value());
    EXPECT_FALSE(found.optionalDate.has_value());
    EXPECT_FALSE(found.optionalTime.has_value());
    EXPECT_FALSE(found.optionalAmount.has_value());
    ASSERT_TRUE(found.optionalUuid.has_value());
    EXPECT_EQ(found.optionalUuid->value, entity.optionalUuid->value);
    ASSERT_TRUE(found.optionalPayload.has_value());
    EXPECT_NE(found.optionalPayload->value.find("optional"), std::string::npos);

    ExtendedTypesEntity updated = found;
    updated.optionalTs = std::chrono::system_clock::time_point{} +
                         std::chrono::milliseconds(1739190305123LL);
    updated.optionalTstz = TimestampTz{
        std::chrono::system_clock::time_point{} + std::chrono::milliseconds(1739190305123LL),
        -5 * 60};
    updated.optionalDate = Date{2027, 1, 1};
    updated.optionalTime = Time{1, 2, 3, 4};
    updated.optionalAmount = Numeric{"42.123456789012345"};
    updated.payload = Jsonb{R"({"type":"updated"})"};

    auto updateResult = repo.update(updated);
    ASSERT_TRUE(updateResult.hasValue()) << updateResult.error().message;
    EXPECT_TRUE(updateResult->optionalTs.has_value());
    EXPECT_TRUE(updateResult->optionalTstz.has_value());
    EXPECT_TRUE(updateResult->optionalDate.has_value());
    EXPECT_TRUE(updateResult->optionalTime.has_value());
    EXPECT_TRUE(updateResult->optionalAmount.has_value());
    EXPECT_EQ(updateResult->optionalAmount->value, "42.123456789012345");
    EXPECT_EQ(updateResult->optionalTs.value(), updated.optionalTs.value());
    EXPECT_EQ(updateResult->optionalTstz->timePoint, updated.optionalTstz->timePoint);
}
