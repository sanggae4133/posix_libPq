#pragma once

/**
 * @file Mapper.hpp
 * @brief Entity-to-Row and Row-to-Entity mapping utilities
 * 
 * Provides mapping between C++ entity objects and PostgreSQL query results
 * with configurable strict/lenient column matching.
 */

#include "Entity.hpp"
#include "../core/QueryResult.hpp"
#include "../core/Result.hpp"
#include <stdexcept>
#include <set>
#include <optional>

namespace pq {
namespace orm {

/**
 * @brief Exception thrown when column mapping fails
 */
class MappingException : public std::runtime_error {
public:
    explicit MappingException(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * @brief Maps query results to entity objects
 */
template<typename Entity>
class EntityMapper {
    static_assert(isEntityV<Entity>, "Entity must be registered with PQ_REGISTER_ENTITY");
    
    const EntityMetadata<Entity>& meta_;
    MapperConfig config_;
    
public:
    explicit EntityMapper(const MapperConfig& config = defaultMapperConfig())
        : meta_(EntityMeta<Entity>::metadata())
        , config_(config) {}

    /**
     * @brief Update mapper configuration
     */
    void setConfig(const MapperConfig& config) {
        config_ = config;
    }

    /**
     * @brief Get mapper configuration
     */
    [[nodiscard]] const MapperConfig& config() const noexcept {
        return config_;
    }
    
    /**
     * @brief Map a single row to an entity
     * @param row Query result row
     * @return Mapped entity
     * @throws MappingException if strict mapping fails
     */
    [[nodiscard]] Entity mapRow(const core::Row& row) const {
        // Validate columns if strict mapping is enabled
        if (config_.strictColumnMapping && !config_.ignoreExtraColumns) {
            validateColumns(row);
        }
        
        Entity entity{};
        
        for (const auto& col : meta_.columns()) {
            int idx = row.columnIndex(col.info.columnName.data());
            if (idx < 0) {
                throw MappingException(
                    std::string("Required column not found in result: ") +
                    std::string(col.info.columnName));
            }
            
            if (row.isNull(idx)) {
                if (!col.info.isNullable) {
                    throw MappingException(
                        std::string("NULL value in non-nullable column: ") +
                        std::string(col.info.columnName));
                }
                col.fromString(entity, nullptr);
            } else {
                col.fromString(entity, row.getRaw(idx));
            }
        }
        
        return entity;
    }
    
    /**
     * @brief Map all rows in a result to entities
     * @param result Query result
     * @return Vector of mapped entities
     */
    [[nodiscard]] std::vector<Entity> mapAll(const core::QueryResult& result) const {
        std::vector<Entity> entities;
        entities.reserve(result.rowCount());
        
        for (const auto& row : result) {
            entities.push_back(mapRow(row));
        }
        
        return entities;
    }
    
    /**
     * @brief Map result to optional entity (first row or empty)
     */
    [[nodiscard]] std::optional<Entity> mapOne(const core::QueryResult& result) const {
        if (result.empty()) {
            return std::nullopt;
        }
        return mapRow(result[0]);
    }
    
    /**
     * @brief Validate that result columns match entity columns (strict mode)
     */
    void validateColumns(const core::Row& row) const {
        std::set<std::string> resultColumns;
        for (int i = 0; i < row.columnCount(); ++i) {
            resultColumns.insert(row.columnName(i));
        }
        
        std::set<std::string> entityColumns;
        for (const auto& col : meta_.columns()) {
            entityColumns.insert(std::string(col.info.columnName));
        }
        
        // Check for extra columns in result
        for (const auto& col : resultColumns) {
            if (entityColumns.find(col) == entityColumns.end()) {
                throw MappingException(
                    "Result contains column not mapped to entity: " + col);
            }
        }
    }
    
    /**
     * @brief Get the metadata for this mapper
     */
    [[nodiscard]] const EntityMetadata<Entity>& metadata() const noexcept {
        return meta_;
    }
};

/**
 * @brief SQL generation utilities for entities
 */
template<typename Entity>
class SqlBuilder {
    static_assert(isEntityV<Entity>, "Entity must be registered with PQ_REGISTER_ENTITY");
    
    const EntityMetadata<Entity>& meta_;
    
public:
    SqlBuilder()
        : meta_(EntityMeta<Entity>::metadata()) {}
    
    /**
     * @brief Generate INSERT SQL
     * @param includeAutoIncrement Whether to include auto-increment columns
     */
    [[nodiscard]] std::string insertSql(bool includeAutoIncrement = false) const {
        std::string cols;
        std::string vals;
        int paramIndex = 1;
        
        for (const auto& col : meta_.columns()) {
            if (!includeAutoIncrement && col.info.isAutoIncrement()) {
                continue;
            }
            
            if (!cols.empty()) {
                cols += ", ";
                vals += ", ";
            }
            
            cols += col.info.columnName;
            vals += "$" + std::to_string(paramIndex++);
        }
        
        return "INSERT INTO " + std::string(meta_.tableName()) +
               " (" + cols + ") VALUES (" + vals + ") RETURNING *";
    }
    
    /**
     * @brief Generate SELECT all SQL
     */
    [[nodiscard]] std::string selectAllSql() const {
        return "SELECT * FROM " + std::string(meta_.tableName());
    }
    
    /**
     * @brief Generate SELECT by primary key SQL
     */
    [[nodiscard]] std::string selectByIdSql() const {
        const auto pks = meta_.primaryKeys();
        if (pks.empty()) {
            throw std::logic_error("Entity has no primary key defined");
        }

        std::string whereClause;
        int paramIndex = 1;
        for (const auto* pk : pks) {
            if (!whereClause.empty()) {
                whereClause += " AND ";
            }
            whereClause += std::string(pk->info.columnName) + " = $" + std::to_string(paramIndex++);
        }

        return "SELECT * FROM " + std::string(meta_.tableName()) +
               " WHERE " + whereClause;
    }
    
    /**
     * @brief Generate UPDATE SQL
     */
    [[nodiscard]] std::string updateSql() const {
        const auto pks = meta_.primaryKeys();
        if (pks.empty()) {
            throw std::logic_error("Entity has no primary key defined");
        }
        
        std::string sets;
        int paramIndex = 1;
        
        for (const auto& col : meta_.columns()) {
            if (col.info.isPrimaryKey()) {
                continue;  // Don't update primary key
            }
            
            if (!sets.empty()) {
                sets += ", ";
            }
            
            sets += std::string(col.info.columnName) + " = $" + std::to_string(paramIndex++);
        }

        std::string whereClause;
        for (const auto* pk : pks) {
            if (!whereClause.empty()) {
                whereClause += " AND ";
            }
            whereClause += std::string(pk->info.columnName) + " = $" + std::to_string(paramIndex++);
        }

        return "UPDATE " + std::string(meta_.tableName()) +
               " SET " + sets +
               " WHERE " + whereClause +
               " RETURNING *";
    }
    
    /**
     * @brief Generate DELETE SQL
     */
    [[nodiscard]] std::string deleteSql() const {
        const auto pks = meta_.primaryKeys();
        if (pks.empty()) {
            throw std::logic_error("Entity has no primary key defined");
        }

        std::string whereClause;
        int paramIndex = 1;
        for (const auto* pk : pks) {
            if (!whereClause.empty()) {
                whereClause += " AND ";
            }
            whereClause += std::string(pk->info.columnName) + " = $" + std::to_string(paramIndex++);
        }

        return "DELETE FROM " + std::string(meta_.tableName()) +
               " WHERE " + whereClause;
    }
    
    /**
     * @brief Get parameters for INSERT
     */
    [[nodiscard]] std::vector<std::optional<std::string>> insertParams(
            const Entity& entity,
            bool includeAutoIncrement = false) const {
        std::vector<std::optional<std::string>> params;
        
        for (const auto& col : meta_.columns()) {
            if (!includeAutoIncrement && col.info.isAutoIncrement()) {
                continue;
            }
            
            if (col.isNull(entity)) {
                params.push_back(std::nullopt);
            } else {
                params.emplace_back(col.toString(entity));
            }
        }
        
        return params;
    }
    
    /**
     * @brief Get parameters for UPDATE (values + pk)
     */
    [[nodiscard]] std::vector<std::optional<std::string>> updateParams(
            const Entity& entity) const {
        std::vector<std::optional<std::string>> params;
        const auto pks = meta_.primaryKeys();
        
        for (const auto& col : meta_.columns()) {
            if (col.info.isPrimaryKey()) {
                continue;
            }
            
            if (col.isNull(entity)) {
                params.push_back(std::nullopt);
            } else {
                params.emplace_back(col.toString(entity));
            }
        }
        
        // Add primary keys as last parameters
        for (const auto* pk : pks) {
            params.emplace_back(pk->toString(entity));
        }
        
        return params;
    }
    
    /**
     * @brief Get all primary key values as strings
     */
    [[nodiscard]] std::vector<std::string> primaryKeyValues(const Entity& entity) const {
        const auto pks = meta_.primaryKeys();
        if (pks.empty()) {
            throw std::logic_error("Entity has no primary key defined");
        }

        std::vector<std::string> values;
        values.reserve(pks.size());

        for (const auto* pk : pks) {
            values.push_back(pk->toString(entity));
        }

        return values;
    }

    /**
     * @brief Get primary key value as string (single PK only)
     */
    [[nodiscard]] std::string primaryKeyValue(const Entity& entity) const {
        const auto pks = meta_.primaryKeys();
        if (pks.empty()) {
            throw std::logic_error("Entity has no primary key defined");
        }
        if (pks.size() != 1) {
            throw std::logic_error("Entity has composite primary key; use primaryKeyValues()");
        }
        return pks.front()->toString(entity);
    }
    
    /**
     * @brief Get the metadata
     */
    [[nodiscard]] const EntityMetadata<Entity>& metadata() const noexcept {
        return meta_;
    }
};

} // namespace orm
} // namespace pq
