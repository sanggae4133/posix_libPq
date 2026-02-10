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
#include "SchemaValidator.hpp"
#include "../core/Connection.hpp"
#include "../core/Result.hpp"
#include <vector>
#include <optional>
#include <tuple>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

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
    bool schemaValidationAttempted_{false};
    std::optional<DbError> schemaValidationError_;
    
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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<Entity>::error(std::move(*validationError));
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<std::optional<Entity>>::error(std::move(*validationError));
        }

        std::string sql;
        std::vector<std::string> params;

        try {
            sql = sqlBuilder_.selectByIdSql();
            params = toPkParams(id, sqlBuilder_.metadata().primaryKeyIndices().size());
        } catch (const std::exception& e) {
            return DbResult<std::optional<Entity>>::error(DbError{e.what()});
        }

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

    template<typename... Args,
             typename = std::enable_if_t<(sizeof...(Args) > 1)>>
    [[nodiscard]] DbResult<std::optional<Entity>> findById(Args&&... ids) {
        using PKDecay = std::decay_t<PK>;
        if constexpr (isTupleV<PKDecay>) {
            static_assert(sizeof...(Args) == std::tuple_size<PKDecay>::value,
                          "findById argument count must match tuple PK size");
            static_assert(tuplePkTypesMatchV<PKDecay, Args...>,
                          "findById argument types must match tuple PK element types");
        } else {
            static_assert(isTupleV<PKDecay>,
                          "Variadic findById requires tuple PK type for composite keys");
        }

        return findById(PK{std::forward<Args>(ids)...});
    }
    
    /**
     * @brief Find all entities
     * @return Vector of all entities
     */
    [[nodiscard]] DbResult<std::vector<Entity>> findAll() {
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<std::vector<Entity>>::error(std::move(*validationError));
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<Entity>::error(std::move(*validationError));
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<int>::error(std::move(*validationError));
        }

        std::string sql;
        std::vector<std::string> params;

        try {
            sql = sqlBuilder_.deleteSql();
            params = toPkParams(id, sqlBuilder_.metadata().primaryKeyIndices().size());
        } catch (const std::exception& e) {
            return DbResult<int>::error(DbError{e.what()});
        }

        auto result = conn_.execute(sql, params);
        if (!result) {
            return DbResult<int>::error(std::move(result).error());
        }
        return result->affectedRows();
    }

    template<typename... Args,
             typename = std::enable_if_t<(sizeof...(Args) > 1)>>
    [[nodiscard]] DbResult<int> removeById(Args&&... ids) {
        using PKDecay = std::decay_t<PK>;
        if constexpr (isTupleV<PKDecay>) {
            static_assert(sizeof...(Args) == std::tuple_size<PKDecay>::value,
                          "removeById argument count must match tuple PK size");
            static_assert(tuplePkTypesMatchV<PKDecay, Args...>,
                          "removeById argument types must match tuple PK element types");
        } else {
            static_assert(isTupleV<PKDecay>,
                          "Variadic removeById requires tuple PK type for composite keys");
        }

        return removeById(PK{std::forward<Args>(ids)...});
    }
    
    /**
     * @brief Remove an entity
     * @param entity Entity to remove (uses primary key)
     * @return Number of rows affected, or error
     */
    [[nodiscard]] DbResult<int> remove(const Entity& entity) {
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<int>::error(std::move(*validationError));
        }

        std::string sql;
        std::vector<std::string> params;

        try {
            sql = sqlBuilder_.deleteSql();
            params = sqlBuilder_.primaryKeyValues(entity);
        } catch (const std::exception& e) {
            return DbResult<int>::error(DbError{e.what()});
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<int64_t>::error(std::move(*validationError));
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<bool>::error(std::move(*validationError));
        }

        std::string sql;
        std::vector<std::string> params;

        try {
            sql = sqlBuilder_.selectByIdSql() + " LIMIT 1";
            params = toPkParams(id, sqlBuilder_.metadata().primaryKeyIndices().size());
        } catch (const std::exception& e) {
            return DbResult<bool>::error(DbError{e.what()});
        }

        auto result = conn_.execute(sql, params);
        
        if (!result) {
            return DbResult<bool>::error(std::move(result).error());
        }
        
        return !result->empty();
    }

    template<typename... Args,
             typename = std::enable_if_t<(sizeof...(Args) > 1)>>
    [[nodiscard]] DbResult<bool> existsById(Args&&... ids) {
        using PKDecay = std::decay_t<PK>;
        if constexpr (isTupleV<PKDecay>) {
            static_assert(sizeof...(Args) == std::tuple_size<PKDecay>::value,
                          "existsById argument count must match tuple PK size");
            static_assert(tuplePkTypesMatchV<PKDecay, Args...>,
                          "existsById argument types must match tuple PK element types");
        } else {
            static_assert(isTupleV<PKDecay>,
                          "Variadic existsById requires tuple PK type for composite keys");
        }

        return existsById(PK{std::forward<Args>(ids)...});
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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<std::vector<Entity>>::error(std::move(*validationError));
        }

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
        if (auto validationError = ensureSchemaValidated()) {
            return DbResult<std::optional<Entity>>::error(std::move(*validationError));
        }

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
            mapperConfigSnapshot_.ignoreExtraColumns != config_.ignoreExtraColumns ||
            mapperConfigSnapshot_.autoValidateSchema != config_.autoValidateSchema ||
            mapperConfigSnapshot_.schemaValidationMode != config_.schemaValidationMode) {
            mapper_.setConfig(config_);

            if (mapperConfigSnapshot_.autoValidateSchema != config_.autoValidateSchema ||
                mapperConfigSnapshot_.schemaValidationMode != config_.schemaValidationMode) {
                schemaValidationAttempted_ = false;
                schemaValidationError_.reset();
            }
            mapperConfigSnapshot_ = config_;
        }
    }

    template<typename T>
    struct IsTuple : std::false_type {};

    template<typename... Ts>
    struct IsTuple<std::tuple<Ts...>> : std::true_type {};

    template<typename T>
    inline static constexpr bool isTupleV = IsTuple<T>::value;

    template<typename Tuple, typename ArgTuple, std::size_t... Indices>
    [[nodiscard]] static constexpr bool tuplePkTypesMatchImpl(std::index_sequence<Indices...>) {
        return (std::is_same_v<
            std::decay_t<std::tuple_element_t<Indices, Tuple>>,
            std::decay_t<std::tuple_element_t<Indices, ArgTuple>>> && ...);
    }

    template<typename Tuple, typename... Args>
    static constexpr bool tuplePkTypesMatchV =
        (std::tuple_size_v<Tuple> == sizeof...(Args)) &&
        tuplePkTypesMatchImpl<Tuple, std::tuple<Args...>>(
            std::make_index_sequence<sizeof...(Args)>{});

    [[nodiscard]] std::optional<DbError> ensureSchemaValidated() {
        syncMapperConfig();

        if (!config_.autoValidateSchema) {
            return std::nullopt;
        }

        if (schemaValidationAttempted_) {
            return schemaValidationError_;
        }

        schemaValidationAttempted_ = true;
        SchemaValidator validator(config_.schemaValidationMode);
        auto validation = validator.template validate<Entity>(conn_);

        if (validation.isValid()) {
            schemaValidationError_.reset();
            return std::nullopt;
        }

        schemaValidationError_ = DbError{formatSchemaValidationError(validation)};
        return schemaValidationError_;
    }

    [[nodiscard]] static std::string formatSchemaValidationError(
            const ValidationResult& validation) {
        std::ostringstream oss;
        oss << "Schema validation failed: " << validation.errorCount()
            << " error(s), " << validation.warningCount() << " warning(s)";

        for (std::size_t i = 0; i < validation.errors.size(); ++i) {
            const auto& issue = validation.errors[i];
            oss << " | #" << (i + 1)
                << " type=" << static_cast<int>(issue.type)
                << ", entity=" << issue.entityName
                << ", table=" << issue.tableName;
            if (!issue.columnName.empty()) {
                oss << ", column=" << issue.columnName;
            }
            if (!issue.expected.empty()) {
                oss << ", expected=" << issue.expected;
            }
            if (!issue.actual.empty()) {
                oss << ", actual=" << issue.actual;
            }
            oss << ", message=" << issue.message;
        }

        return oss.str();
    }

    template<typename Value>
    [[nodiscard]] static std::string toPkComponentString(const Value& value) {
        using ValueDecay = std::decay_t<Value>;

        if constexpr (std::is_same_v<ValueDecay, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<ValueDecay, std::string_view>) {
            return std::string(value);
        } else if constexpr (std::is_same_v<ValueDecay, const char*> ||
                             std::is_same_v<ValueDecay, char*>) {
            return std::string(value);
        } else {
            return PgTypeTraits<ValueDecay>::toString(value);
        }
    }

    [[nodiscard]] static std::vector<std::string> toPkParams(const PK& id, std::size_t pkCount) {
        using PKDecay = std::decay_t<PK>;

        if constexpr (isTupleV<PKDecay>) {
            constexpr std::size_t tupleSize = std::tuple_size_v<PKDecay>;
            if (pkCount != tupleSize) {
                throw std::invalid_argument(
                    "Primary key count mismatch: repository PK tuple size is " +
                    std::to_string(tupleSize) + ", but entity defines " +
                    std::to_string(pkCount) + " primary key column(s)");
            }

            std::vector<std::string> params;
            params.reserve(tupleSize);
            std::apply([&params](const auto&... values) {
                (params.push_back(toPkComponentString(values)), ...);
            }, id);
            return params;
        } else {
            if (pkCount != 1) {
                throw std::invalid_argument(
                    "Composite primary key entity requires tuple PK type");
            }

            return {toPkComponentString(id)};
        }
    }
};

} // namespace orm
} // namespace pq
