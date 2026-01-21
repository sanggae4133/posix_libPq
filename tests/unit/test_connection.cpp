/**
 * @file test_connection.cpp
 * @brief Unit tests for Connection and ConnectionConfig classes
 */

#include <gtest/gtest.h>
#include <pq/core/Connection.hpp>
#include <pq/core/Result.hpp>
#include <string>

using namespace pq;
using namespace pq::core;

class ConnectionConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// ConnectionConfig Tests
// ============================================================================

TEST_F(ConnectionConfigTest, DefaultValues) {
    ConnectionConfig config;
    
    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 5432);
    EXPECT_TRUE(config.database.empty());
    EXPECT_TRUE(config.user.empty());
    EXPECT_TRUE(config.password.empty());
    EXPECT_TRUE(config.options.empty());
    EXPECT_EQ(config.connectTimeoutSec, 10);
}

TEST_F(ConnectionConfigTest, ToConnectionString) {
    ConnectionConfig config;
    config.host = "db.example.com";
    config.port = 5433;
    config.database = "testdb";
    config.user = "testuser";
    config.password = "secret";
    config.connectTimeoutSec = 30;
    
    std::string connStr = config.toConnectionString();
    
    // Verify all parts are present
    EXPECT_NE(connStr.find("host=db.example.com"), std::string::npos);
    EXPECT_NE(connStr.find("port=5433"), std::string::npos);
    EXPECT_NE(connStr.find("dbname=testdb"), std::string::npos);
    EXPECT_NE(connStr.find("user=testuser"), std::string::npos);
    EXPECT_NE(connStr.find("password=secret"), std::string::npos);
    EXPECT_NE(connStr.find("connect_timeout=30"), std::string::npos);
}

TEST_F(ConnectionConfigTest, ToConnectionStringMinimal) {
    ConnectionConfig config;
    config.database = "mydb";
    config.user = "myuser";
    
    std::string connStr = config.toConnectionString();
    
    EXPECT_NE(connStr.find("host=localhost"), std::string::npos);
    EXPECT_NE(connStr.find("port=5432"), std::string::npos);
    EXPECT_NE(connStr.find("dbname=mydb"), std::string::npos);
    EXPECT_NE(connStr.find("user=myuser"), std::string::npos);
}

TEST_F(ConnectionConfigTest, FromConnectionString) {
    // Note: Current implementation stores the entire connection string in options
    // and returns default values for individual fields
    std::string connStr = "host=myhost port=5433 dbname=mydb user=myuser password=mypass";
    
    auto config = ConnectionConfig::fromConnectionString(connStr);
    
    // The connection string is stored in options
    EXPECT_EQ(config.options, connStr);
    // Default values are returned for individual fields
    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 5432);
}

TEST_F(ConnectionConfigTest, FromConnectionStringPartial) {
    std::string connStr = "host=otherhost dbname=otherdb";
    
    auto config = ConnectionConfig::fromConnectionString(connStr);
    
    // The connection string is stored in options
    EXPECT_EQ(config.options, connStr);
    // Default values for individual fields
    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 5432);
}

TEST_F(ConnectionConfigTest, FromConnectionStringEmpty) {
    auto config = ConnectionConfig::fromConnectionString("");
    
    // Should return default values
    EXPECT_EQ(config.host, "localhost");
    EXPECT_EQ(config.port, 5432);
    EXPECT_TRUE(config.options.empty());
}

TEST_F(ConnectionConfigTest, FromConnectionStringStoresInOptions) {
    std::string connStr = "host='my host' dbname='my db' password='pass word'";
    
    auto config = ConnectionConfig::fromConnectionString(connStr);
    
    // Connection string is stored in options for pass-through
    EXPECT_EQ(config.options, connStr);
}

// ============================================================================
// Connection Tests (without actual database)
// ============================================================================

TEST_F(ConnectionTest, DefaultConstruction) {
    Connection conn;
    
    EXPECT_FALSE(conn.isConnected());
    EXPECT_FALSE(conn.inTransaction());
    EXPECT_EQ(conn.raw(), nullptr);
}

TEST_F(ConnectionTest, NotConnectedStatus) {
    Connection conn;
    
    EXPECT_EQ(conn.status(), CONNECTION_BAD);
}

TEST_F(ConnectionTest, NotConnectedServerVersion) {
    Connection conn;
    
    EXPECT_EQ(conn.serverVersion(), 0);
}

TEST_F(ConnectionTest, ExecuteWithoutConnection) {
    Connection conn;
    
    auto result = conn.execute("SELECT 1");
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
    EXPECT_NE(result.error().message.find("Not connected"), std::string::npos);
}

TEST_F(ConnectionTest, ExecuteUpdateWithoutConnection) {
    Connection conn;
    
    auto result = conn.executeUpdate("UPDATE test SET x = 1");
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, ExecuteParamsWithoutConnection) {
    Connection conn;
    
    auto result = conn.executeParams("SELECT $1", 42);
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, PrepareWithoutConnection) {
    Connection conn;
    
    auto result = conn.prepare("test_stmt", "SELECT $1");
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, ExecutePreparedWithoutConnection) {
    Connection conn;
    
    auto result = conn.executePrepared("test_stmt", {"value"});
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, BeginTransactionWithoutConnection) {
    Connection conn;
    
    auto result = conn.beginTransaction();
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, CommitWithoutConnection) {
    Connection conn;
    
    auto result = conn.commit();
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, RollbackWithoutConnection) {
    Connection conn;
    
    auto result = conn.rollback();
    
    EXPECT_FALSE(result.hasValue());
    EXPECT_TRUE(result.hasError());
}

TEST_F(ConnectionTest, DisconnectSafe) {
    Connection conn;
    
    // Should not crash when disconnecting an unconnected connection
    EXPECT_NO_THROW(conn.disconnect());
    EXPECT_FALSE(conn.isConnected());
}

TEST_F(ConnectionTest, MoveConstruction) {
    Connection conn1;
    Connection conn2 = std::move(conn1);
    
    EXPECT_FALSE(conn2.isConnected());
}

TEST_F(ConnectionTest, MoveAssignment) {
    Connection conn1;
    Connection conn2;
    
    conn2 = std::move(conn1);
    
    EXPECT_FALSE(conn2.isConnected());
}

TEST_F(ConnectionTest, ConnectInvalidConnectionString) {
    Connection conn;
    
    // This should fail gracefully (no crash)
    auto result = conn.connect("invalid_connection_string_that_wont_work");
    
    // The result depends on libpq behavior, but it should not crash
    EXPECT_FALSE(conn.isConnected());
}

TEST_F(ConnectionTest, ConnectWithConfig) {
    Connection conn;
    
    ConnectionConfig config;
    config.host = "nonexistent.invalid.host";
    config.database = "nonexistent";
    
    auto result = conn.connect(config);
    
    // Should fail but not crash
    EXPECT_FALSE(conn.isConnected());
}

// ============================================================================
// DbError Tests
// ============================================================================

TEST_F(ConnectionTest, DbErrorConstruction) {
    DbError error("Test error", "42P01", 1);
    
    EXPECT_EQ(error.message, "Test error");
    EXPECT_EQ(error.sqlState, "42P01");
    EXPECT_EQ(error.errorCode, 1);
    EXPECT_STREQ(error.what(), "Test error");
}

TEST_F(ConnectionTest, DbErrorDefaultConstruction) {
    DbError error;
    
    EXPECT_TRUE(error.message.empty());
    EXPECT_TRUE(error.sqlState.empty());
    EXPECT_EQ(error.errorCode, 0);
}

TEST_F(ConnectionTest, DbErrorMessageOnly) {
    DbError error("Simple error");
    
    EXPECT_EQ(error.message, "Simple error");
    EXPECT_TRUE(error.sqlState.empty());
    EXPECT_EQ(error.errorCode, 0);
}
