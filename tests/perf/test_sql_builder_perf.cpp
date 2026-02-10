/**
 * @file test_sql_builder_perf.cpp
 * @brief Lightweight performance regression checks for SQL builder paths
 */

#include <gtest/gtest.h>
#include <pq/orm/Mapper.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <string>

using namespace pq;
using namespace pq::orm;

namespace {
bool perfTestsEnabled() {
    const char* env = std::getenv("PQ_ENABLE_PERF_TESTS");
    return env && std::string(env) == "1";
}
} // namespace

struct PerfSinglePkEntity {
    int id{0};
    std::string name;
    std::optional<std::string> description;

    PQ_ENTITY(PerfSinglePkEntity, "perf_single_pk")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(description, "description", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(PerfSinglePkEntity)

struct PerfCompositePkEntity {
    int orderId{0};
    int productId{0};
    int quantity{0};
    std::optional<std::string> note;

    PQ_ENTITY(PerfCompositePkEntity, "perf_composite_pk")
        PQ_COLUMN(orderId, "order_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(productId, "product_id", PQ_PRIMARY_KEY)
        PQ_COLUMN(quantity, "quantity", PQ_NOT_NULL)
        PQ_COLUMN(note, "note", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(PerfCompositePkEntity)

TEST(SqlBuilderPerfTest, CompositePrimaryKeySqlGenerationRegressionGuard) {
    if (!perfTestsEnabled()) {
        GTEST_SKIP() << "Set PQ_ENABLE_PERF_TESTS=1 to run performance tests.";
    }

    constexpr int kIterations = 50000;
    volatile std::size_t sink = 0;

    SqlBuilder<PerfSinglePkEntity> singleBuilder;
    SqlBuilder<PerfCompositePkEntity> compositeBuilder;

    auto measureMs = [&](auto&& op) {
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < kIterations; ++i) {
            sink += op().size();
        }
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };

    const auto singleMs = measureMs([&]() { return singleBuilder.selectByIdSql(); });
    const auto compositeMs = measureMs([&]() { return compositeBuilder.selectByIdSql(); });

    EXPECT_GT(sink, 0u);
    EXPECT_LT(compositeMs, std::max<int64_t>(1, singleMs * 10))
        << "Composite PK SQL generation regressed unexpectedly";
}

TEST(SqlBuilderPerfTest, CompositePrimaryKeyUpdateParamsRegressionGuard) {
    if (!perfTestsEnabled()) {
        GTEST_SKIP() << "Set PQ_ENABLE_PERF_TESTS=1 to run performance tests.";
    }

    constexpr int kIterations = 50000;
    volatile std::size_t sink = 0;

    SqlBuilder<PerfCompositePkEntity> builder;
    PerfCompositePkEntity entity;
    entity.orderId = 11;
    entity.productId = 22;
    entity.quantity = 1;
    entity.note = "bulk";

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        entity.quantity = i % 100;
        auto params = builder.updateParams(entity);
        sink += params.size();
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_GT(sink, 0u);
    EXPECT_LT(elapsedMs, 8000) << "updateParams performance regressed unexpectedly";
}

TEST(SqlBuilderPerfTest, ExtendedTypeConversionRegressionGuard) {
    if (!perfTestsEnabled()) {
        GTEST_SKIP() << "Set PQ_ENABLE_PERF_TESTS=1 to run performance tests.";
    }

    constexpr int kIterations = 30000;
    volatile std::size_t sink = 0;

    const auto ts = std::chrono::system_clock::time_point{} +
                    std::chrono::milliseconds(1739186705123LL);
    const TimestampTz tsTz{ts, 9 * 60};
    const Date date{2026, 2, 10};
    const Time time{11, 22, 33, 444};
    const Numeric numeric{"123456789012345.123456789012345"};
    const Uuid uuid{"550e8400-e29b-41d4-a716-446655440000"};
    const Jsonb jsonb{R"({"type":"perf","ok":true})"};

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        auto tsText = PgTypeTraits<std::chrono::system_clock::time_point>::toString(ts);
        auto tsTzText = PgTypeTraits<TimestampTz>::toString(tsTz);
        auto dateText = PgTypeTraits<Date>::toString(date);
        auto timeText = PgTypeTraits<Time>::toString(time);
        auto numericText = PgTypeTraits<Numeric>::toString(numeric);
        auto uuidText = PgTypeTraits<Uuid>::toString(uuid);
        auto jsonbText = PgTypeTraits<Jsonb>::toString(jsonb);

        sink += tsText.size() + tsTzText.size() + dateText.size() + timeText.size() +
                numericText.size() + uuidText.size() + jsonbText.size();
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_GT(sink, 0u);
    EXPECT_LT(elapsedMs, 10000) << "Extended type conversion performance regressed unexpectedly";
}
