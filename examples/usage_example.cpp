/**
 * @file usage_example.cpp
 * @brief Complete usage example for PosixLibPq ORM library
 * 
 * This example demonstrates:
 * - Entity definition using macros
 * - Repository CRUD operations
 * - Raw query execution
 * - Transaction management
 * - Error handling
 */

#include <pq/pq.hpp>
#include <iostream>
#include <string>

// ============================================================================
// Entity Definitions
// ============================================================================

/**
 * @brief User entity mapped to "users" table
 */
struct User {
    int id{0};
    std::string name;
    std::string email;
    std::optional<int> age;
    
    // Entity mapping using macros
    PQ_ENTITY(User, "users")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NONE)
        PQ_COLUMN(email, "email", PQ_NONE)
        PQ_COLUMN(age, "age", PQ_NONE)  // Nullable column
    PQ_ENTITY_END()
};

// Register entity for metadata access
PQ_REGISTER_ENTITY(User)

/**
 * @brief Product entity mapped to "products" table
 */
struct Product {
    int64_t id{0};
    std::string name;
    double price{0.0};
    std::optional<std::string> description;
    
    PQ_ENTITY(Product, "products")
        PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
        PQ_COLUMN(name, "name", PQ_NOT_NULL)
        PQ_COLUMN(price, "price", PQ_NOT_NULL)
        PQ_COLUMN(description, "description", PQ_NONE)
    PQ_ENTITY_END()
};

PQ_REGISTER_ENTITY(Product)

// ============================================================================
// Helper Functions
// ============================================================================

void printUser(const User& user) {
    std::cout << "User { id: " << user.id 
              << ", name: \"" << user.name << "\""
              << ", email: \"" << user.email << "\"";
    if (user.age) {
        std::cout << ", age: " << *user.age;
    } else {
        std::cout << ", age: NULL";
    }
    std::cout << " }" << std::endl;
}

void printProduct(const Product& product) {
    std::cout << "Product { id: " << product.id 
              << ", name: \"" << product.name << "\""
              << ", price: " << product.price;
    if (product.description) {
        std::cout << ", description: \"" << *product.description << "\"";
    }
    std::cout << " }" << std::endl;
}

// ============================================================================
// Main Example
// ============================================================================

int main(int argc, char* argv[]) {
    // Connection configuration
    pq::ConnectionConfig config;
    config.host = "localhost";
    config.port = 5432;
    config.database = "testdb";
    config.user = "root";
    config.password = "1234";
    
    // Alternative: use connection string
    // pq::Connection conn("host=localhost port=5432 dbname=testdb user=postgres");
    
    std::cout << "=== PosixLibPq ORM Usage Example ===" << std::endl << std::endl;
    
    // =========================================================================
    // 1. Connection
    // =========================================================================
    std::cout << "1. Connecting to database..." << std::endl;
    
    pq::Connection conn;
    auto connectResult = conn.connect(config);
    
    if (!connectResult) {
        std::cerr << "Connection failed: " << connectResult.error().message << std::endl;
        return 1;
    }
    
    std::cout << "   Connected! Server version: " << conn.serverVersion() << std::endl;
    std::cout << std::endl;
    
    // =========================================================================
    // 2. Create tables (raw query)
    // =========================================================================
    std::cout << "2. Creating tables..." << std::endl;
    
    auto createUsersTable = conn.execute(R"(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            email VARCHAR(255) NOT NULL,
            age INTEGER
        )
    )");
    
    if (!createUsersTable) {
        std::cerr << "   Failed to create users table: " 
                  << createUsersTable.error().message << std::endl;
        return 1;
    }
    std::cout << "   Users table created." << std::endl;
    
    auto createProductsTable = conn.execute(R"(
        CREATE TABLE IF NOT EXISTS products (
            id BIGSERIAL PRIMARY KEY,
            name VARCHAR(200) NOT NULL,
            price DOUBLE PRECISION NOT NULL,
            description TEXT
        )
    )");
    
    if (!createProductsTable) {
        std::cerr << "   Failed to create products table: " 
                  << createProductsTable.error().message << std::endl;
        return 1;
    }
    std::cout << "   Products table created." << std::endl;
    std::cout << std::endl;
    
    // =========================================================================
    // 3. Repository - save()
    // =========================================================================
    std::cout << "3. Saving entities with Repository..." << std::endl;
    
    pq::Repository<User, int> userRepo(conn);
    
    User newUser;
    newUser.name = "John Doe";
    newUser.email = "john@example.com";
    newUser.age = 30;
    
    auto savedUser = userRepo.save(newUser);
    if (savedUser) {
        std::cout << "   Saved: ";
        printUser(*savedUser);
    } else {
        std::cerr << "   Save failed: " << savedUser.error().message << std::endl;
    }
    
    // Save another user without age (NULL)
    User anotherUser;
    anotherUser.name = "Jane Smith";
    anotherUser.email = "jane@example.com";
    // age is std::nullopt (NULL)
    
    auto savedUser2 = userRepo.save(anotherUser);
    if (savedUser2) {
        std::cout << "   Saved: ";
        printUser(*savedUser2);
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 4. Repository - findById()
    // =========================================================================
    std::cout << "4. Finding entity by ID..." << std::endl;
    
    auto foundUser = userRepo.findById(savedUser->id);
    if (foundUser && *foundUser) {
        std::cout << "   Found: ";
        printUser(**foundUser);
    } else if (foundUser) {
        std::cout << "   User not found" << std::endl;
    } else {
        std::cerr << "   Query error: " << foundUser.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 5. Repository - findAll()
    // =========================================================================
    std::cout << "5. Finding all entities..." << std::endl;
    
    auto allUsers = userRepo.findAll();
    if (allUsers) {
        std::cout << "   Found " << allUsers->size() << " users:" << std::endl;
        for (const auto& user : *allUsers) {
            std::cout << "   - ";
            printUser(user);
        }
    } else {
        std::cerr << "   Query error: " << allUsers.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 6. Repository - update()
    // =========================================================================
    std::cout << "6. Updating entity..." << std::endl;
    
    savedUser->name = "John Updated";
    savedUser->age = 31;
    
    auto updatedUser = userRepo.update(*savedUser);
    if (updatedUser) {
        std::cout << "   Updated: ";
        printUser(*updatedUser);
    } else {
        std::cerr << "   Update failed: " << updatedUser.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 7. Raw Query with Parameters (for custom lookups)
    // =========================================================================
    std::cout << "7. Executing raw query with parameters..." << std::endl;
    
    // Since findById is just an example, you can use raw queries for any lookup
    std::vector<std::string> params = {"john@example.com"};
    auto queryResult = userRepo.executeQueryOne(
        "SELECT * FROM users WHERE email = $1",
        params
    );
    
    if (queryResult && *queryResult) {
        std::cout << "   Found by email: ";
        printUser(**queryResult);
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 8. Transaction Management
    // =========================================================================
    std::cout << "8. Transaction example..." << std::endl;
    
    {
        pq::Transaction tx(conn);
        if (!tx) {
            std::cerr << "   Failed to begin transaction" << std::endl;
        } else {
            std::cout << "   Transaction started" << std::endl;
            
            // Save a product
            pq::Repository<Product, int64_t> productRepo(conn);
            Product product;
            product.name = "Widget";
            product.price = 19.99;
            product.description = "A useful widget";
            
            auto savedProduct = productRepo.save(product);
            if (savedProduct) {
                std::cout << "   Saved in transaction: ";
                printProduct(*savedProduct);
            }
            
            // Commit the transaction
            auto commitResult = tx.commit();
            if (commitResult) {
                std::cout << "   Transaction committed!" << std::endl;
            } else {
                std::cerr << "   Commit failed: " << commitResult.error().message << std::endl;
            }
        }
    }  // If commit wasn't called, destructor would auto-rollback
    std::cout << std::endl;
    
    // =========================================================================
    // 9. Repository - remove()
    // =========================================================================
    std::cout << "9. Removing entity..." << std::endl;
    
    auto removeResult = userRepo.remove(*savedUser2);
    if (removeResult) {
        std::cout << "   Removed " << *removeResult << " row(s)" << std::endl;
    } else {
        std::cerr << "   Remove failed: " << removeResult.error().message << std::endl;
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 10. Strict Mapping Policy (Edge Case)
    // =========================================================================
    std::cout << "10. Demonstrating strict mapping policy..." << std::endl;
    
    // By default, strict mapping throws an error if result has unmapped columns
    // You can relax this:
    pq::MapperConfig relaxedConfig;
    relaxedConfig.strictColumnMapping = true;
    relaxedConfig.ignoreExtraColumns = true;  // Allow extra columns
    
    pq::Repository<User, int> relaxedRepo(conn, relaxedConfig);
    
    // This query returns extra columns that aren't mapped to User
    auto rawResult = conn.execute(
        "SELECT id, name, email, age, 'extra_value' AS extra_column FROM users LIMIT 1"
    );
    
    if (rawResult) {
        std::cout << "   Query returned " << rawResult->columnCount() << " columns" << std::endl;
        
        // With strict mapping (default), this would throw
        // With relaxedConfig.ignoreExtraColumns = true, extra columns are ignored
        try {
            pq::orm::EntityMapper<User> mapper(relaxedConfig);
            if (!rawResult->empty()) {
                auto user = mapper.mapRow((*rawResult)[0]);
                std::cout << "   Mapped with extra columns ignored: ";
                printUser(user);
            }
        } catch (const pq::orm::MappingException& e) {
            std::cerr << "   Mapping error (expected with strict mode): " << e.what() << std::endl;
        }
    }
    std::cout << std::endl;
    
    // =========================================================================
    // 11. Cleanup
    // =========================================================================
    std::cout << "11. Cleanup..." << std::endl;
    
    conn.execute("DROP TABLE IF EXISTS users");
    conn.execute("DROP TABLE IF EXISTS products");
    
    std::cout << "   Tables dropped." << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Example Complete ===" << std::endl;
    
    return 0;
}
