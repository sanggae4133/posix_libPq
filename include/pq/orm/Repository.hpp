#pragma once

/**
 * @file Repository.hpp
 * @brief Generic repository pattern for entity CRUD operations
 * 
 * Provides a type-safe interface for database operations on entities.
 * Supports save, findById, findAll, update, remove operations.
 */

#include "Entity.hpp"
#include "Mapper.hpp"
#include "../core/Connection.hpp"
#include "../core/Result.hpp"
#include <vector>
#include <optional>
#include <type_traits>

namespace pq {
namespace orm {

/**
 * @brief Generic repository for entity CRUD operations
 * 
 * @tparam Entity The entity type (must be registered with PQ_REGISTER_ENTITY)
 * @tparam PK The primary key type
 * 
 * Usage:
 * @code
 * Repository<User, int> userRepo(connection);
 * 
 * // Save
 * User newUser{"John", "john@email.com"};
 * auto saved = userRepo.save(newUser);
 * 
 * // Find
 * auto user = userRepo.findById(1);
 * auto allUsers = userRepo.findAll();
 * 
 * // Update
 * user->name = "Jane";
 * userRepo.update(*user);
 * 
 * // Remove
 * userRepo.remove(*user);
 * @endcode
 */
template<typename Entity, typename PK = int>
class Repository {
    static_assert(isEntityV<Entity>, "Entity must be registered with PQ_REGISTER_ENTITY");
    
    core::Connection& conn_;
    EntityMapper<Entity> mapper_;
    SqlBuilder<Entity> sqlBuilder_;
    MapperConfig config_;
    MapperConfig mapperConfigSnapshot_;
    
public:
    using EntityType = Entity;
    using PrimaryKeyType = PK;
    
    /**
     * @brief Construct repository with a database connection
     * @param conn Database connection (must outlive the repository)
     * @param config Optional mapper configuration
     */
    explicit Repository(core::Connection& conn, 
                        const MapperConfig& config = defaultMapperConfig())
        : conn_(conn)
        , mapper_(config)
        , config_(config)
        , mapperConfigSnapshot_(config) {}
    
    /**
     * @brief Save a new entity to the database
     * @param entity Entity to save (id field ignored if auto-increment)
     * @return Saved entity with generated id, or error
     */
    [[nodiscard]] DbResult<Entity> save(const Entity& entity) {
        auto sql = sqlBuilder_.insertSql();
        auto params = sqlBuilder_.insertParams(entity);
        
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<Entity>::error(std::move(result).error());
        }
        
        if (result->empty()) {
            return DbResult<Entity>::error(DbError{"Insert did not return entity"});
        }
        
        try {
            return mapper().mapRow((*result)[0]);
        } catch (const MappingException& e) {
            return DbResult<Entity>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Save multiple entities
     * @param entities Entities to save
     * @return Vector of saved entities with generated ids
     */
    [[nodiscard]] DbResult<std::vector<Entity>> saveAll(const std::vector<Entity>& entities) {
        std::vector<Entity> saved;
        saved.reserve(entities.size());
        
        for (const auto& entity : entities) {
            auto result = save(entity);
            if (!result) {
                return DbResult<std::vector<Entity>>::error(std::move(result).error());
            }
            saved.push_back(std::move(*result));
        }
        
        return saved;
    }
    
    /**
     * @brief Find an entity by its primary key
     * @param id Primary key value
     * @return Entity if found, empty optional if not found, or error
     */
    [[nodiscard]] DbResult<std::optional<Entity>> findById(const PK& id) {
        auto sql = sqlBuilder_.selectByIdSql();
        std::vector<std::string> params = {toPkString(id)};
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<std::optional<Entity>>::error(std::move(result).error());
        }
        
        if (result->empty()) {
            return std::optional<Entity>{std::nullopt};
        }
        
        try {
            return std::optional<Entity>{mapper().mapRow((*result)[0])};
        } catch (const MappingException& e) {
            return DbResult<std::optional<Entity>>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Find all entities
     * @return Vector of all entities
     */
    [[nodiscard]] DbResult<std::vector<Entity>> findAll() {
        auto sql = sqlBuilder_.selectAllSql();
        auto result = conn_.execute(sql);
        
        if (!result) {
            return DbResult<std::vector<Entity>>::error(std::move(result).error());
        }
        
        try {
            return mapper().mapAll(*result);
        } catch (const MappingException& e) {
            return DbResult<std::vector<Entity>>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Update an existing entity
     * @param entity Entity to update (must have valid primary key)
     * @return Updated entity, or error
     */
    [[nodiscard]] DbResult<Entity> update(const Entity& entity) {
        auto sql = sqlBuilder_.updateSql();
        auto params = sqlBuilder_.updateParams(entity);
        
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<Entity>::error(std::move(result).error());
        }
        
        if (result->empty()) {
            return DbResult<Entity>::error(DbError{"Entity not found for update"});
        }
        
        try {
            return mapper().mapRow((*result)[0]);
        } catch (const MappingException& e) {
            return DbResult<Entity>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Remove an entity by its primary key value
     * @param id Primary key of entity to remove
     * @return Number of rows affected, or error
     */
    [[nodiscard]] DbResult<int> removeById(const PK& id) {
        auto sql = sqlBuilder_.deleteSql();
        std::vector<std::string> params = {toPkString(id)};
        auto result = conn_.execute(sql, params);
        if (!result) {
            return DbResult<int>::error(std::move(result).error());
        }
        return result->affectedRows();
    }
    
    /**
     * @brief Remove an entity
     * @param entity Entity to remove (uses primary key)
     * @return Number of rows affected, or error
     */
    [[nodiscard]] DbResult<int> remove(const Entity& entity) {
        auto pkValue = sqlBuilder_.primaryKeyValue(entity);
        auto sql = sqlBuilder_.deleteSql();
        std::vector<std::string> params = {pkValue};
        auto result = conn_.execute(sql, params);
        if (!result) {
            return DbResult<int>::error(std::move(result).error());
        }
        return result->affectedRows();
    }
    
    /**
     * @brief Remove multiple entities
     * @param entities Entities to remove
     * @return Total number of rows affected
     */
    [[nodiscard]] DbResult<int> removeAll(const std::vector<Entity>& entities) {
        int totalAffected = 0;
        
        for (const auto& entity : entities) {
            auto result = remove(entity);
            if (!result) {
                return DbResult<int>::error(std::move(result).error());
            }
            totalAffected += *result;
        }
        
        return totalAffected;
    }
    
    /**
     * @brief Count all entities
     * @return Total count
     */
    [[nodiscard]] DbResult<int64_t> count() {
        auto sql = "SELECT COUNT(*) FROM " + std::string(sqlBuilder_.metadata().tableName());
        auto result = conn_.execute(sql);
        
        if (!result) {
            return DbResult<int64_t>::error(std::move(result).error());
        }
        
        if (result->empty()) {
            return int64_t{0};
        }
        
        return result->row(0).template get<int64_t>(0);
    }
    
    /**
     * @brief Check if entity with given id exists
     * @param id Primary key to check
     */
    [[nodiscard]] DbResult<bool> existsById(const PK& id) {
        const auto& meta = sqlBuilder_.metadata();
        const auto* pk = meta.primaryKey();
        if (!pk) {
            return DbResult<bool>::error(DbError{"Entity has no primary key"});
        }
        
        auto sql = "SELECT 1 FROM " + std::string(meta.tableName()) +
                   " WHERE " + std::string(pk->info.columnName) + " = $1 LIMIT 1";
        
        std::vector<std::string> params = {toPkString(id)};
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<bool>::error(std::move(result).error());
        }
        
        return !result->empty();
    }
    
    /**
     * @brief Execute a custom query and map results to entities
     * @param sql SQL query
     * @param params Query parameters
     * @return Vector of mapped entities
     */
    [[nodiscard]] DbResult<std::vector<Entity>> executeQuery(
            std::string_view sql,
            const std::vector<std::string>& params = {}) {
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<std::vector<Entity>>::error(std::move(result).error());
        }
        
        try {
            return mapper().mapAll(*result);
        } catch (const MappingException& e) {
            return DbResult<std::vector<Entity>>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Execute a custom query and map to single optional entity
     */
    [[nodiscard]] DbResult<std::optional<Entity>> executeQueryOne(
            std::string_view sql,
            const std::vector<std::string>& params = {}) {
        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<std::optional<Entity>>::error(std::move(result).error());
        }
        
        try {
            return mapper().mapOne(*result);
        } catch (const MappingException& e) {
            return DbResult<std::optional<Entity>>::error(DbError{e.what()});
        }
    }
    
    /**
     * @brief Get the underlying connection
     */
    [[nodiscard]] core::Connection& connection() noexcept {
        return conn_;
    }
    
    /**
     * @brief Get mapper configuration
     */
    [[nodiscard]] MapperConfig& config() noexcept {
        return config_;
    }

private:
    [[nodiscard]] EntityMapper<Entity>& mapper() {
        syncMapperConfig();
        return mapper_;
    }

    void syncMapperConfig() {
        if (mapperConfigSnapshot_.strictColumnMapping != config_.strictColumnMapping ||
            mapperConfigSnapshot_.ignoreExtraColumns != config_.ignoreExtraColumns) {
            mapper_.setConfig(config_);
            mapperConfigSnapshot_ = config_;
        }
    }

    [[nodiscard]] static std::string toPkString(const PK& id) {
        using PKDecay = std::decay_t<PK>;

        if constexpr (std::is_same_v<PKDecay, std::string>) {
            return id;
        } else if constexpr (std::is_same_v<PKDecay, std::string_view>) {
            return std::string(id);
        } else if constexpr (std::is_same_v<PKDecay, const char*> ||
                             std::is_same_v<PKDecay, char*>) {
            return std::string(id);
        } else {
            return PgTypeTraits<PK>::toString(id);
        }
    }
};

} // namespace orm
} // namespace pq
