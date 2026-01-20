#pragma once

/**
 * @file Entity.hpp
 * @brief Macro-based declarative entity mapping system
 * 
 * Provides JPA-like declarative mapping of C++ classes to PostgreSQL tables
 * using macros that generate compile-time metadata.
 * 
 * Usage:
 * @code
 * struct User {
 *     int id;
 *     std::string name;
 *     std::optional<std::string> email;
 *     
 *     PQ_ENTITY(User, "users")
 *     PQ_COLUMN(id, "id", PQ_PRIMARY_KEY | PQ_AUTO_INCREMENT)
 *     PQ_COLUMN(name, "name")
 *     PQ_COLUMN(email, "email")
 *     PQ_ENTITY_END()
 * };
 * @endcode
 */

#include "../core/Types.hpp"
#include "../core/QueryResult.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <functional>
#include <type_traits>

namespace pq {
namespace orm {

/**
 * @brief Column flags for entity mapping
 */
enum class ColumnFlags : uint32_t {
    None           = 0,
    PrimaryKey     = 1 << 0,
    AutoIncrement  = 1 << 1,
    NotNull        = 1 << 2,
    Unique         = 1 << 3,
    Index          = 1 << 4,
};

// Bitwise operators for ColumnFlags
inline constexpr ColumnFlags operator|(ColumnFlags a, ColumnFlags b) {
    return static_cast<ColumnFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr ColumnFlags operator&(ColumnFlags a, ColumnFlags b) {
    return static_cast<ColumnFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline constexpr bool hasFlag(ColumnFlags flags, ColumnFlags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

/**
 * @brief Metadata for a single column
 */
struct ColumnInfo {
    std::string_view fieldName;      // C++ field name
    std::string_view columnName;     // Database column name
    Oid pgType;                      // PostgreSQL OID
    ColumnFlags flags;
    bool isNullable;
    
    [[nodiscard]] bool isPrimaryKey() const noexcept {
        return hasFlag(flags, ColumnFlags::PrimaryKey);
    }
    
    [[nodiscard]] bool isAutoIncrement() const noexcept {
        return hasFlag(flags, ColumnFlags::AutoIncrement);
    }
};

/**
 * @brief Metadata for an entity (table)
 */
template<typename T>
struct EntityMeta {
    // These are defined by the PQ_ENTITY macros
    // static constexpr std::string_view tableName;
    // static const std::vector<ColumnInfo>& columns();
    // static const ColumnInfo* primaryKey();
};

/**
 * @brief Type trait to detect if a type is a registered entity
 */
template<typename T, typename = void>
struct IsEntity : std::false_type {};

template<typename T>
struct IsEntity<T, std::void_t<decltype(EntityMeta<T>::tableName)>> : std::true_type {};

template<typename T>
inline constexpr bool isEntityV = IsEntity<T>::value;

/**
 * @brief Mapper configuration options
 */
struct MapperConfig {
    bool strictColumnMapping = true;  // Throw error on unmapped columns
    bool ignoreExtraColumns = false;  // Override strict mapping for extra columns
};

/**
 * @brief Default global mapper configuration
 */
inline MapperConfig& defaultMapperConfig() {
    static MapperConfig config;
    return config;
}

/**
 * @brief Helper to extract value from entity field
 */
template<typename Entity, typename FieldType>
struct FieldAccessor {
    using MemberPtr = FieldType Entity::*;
    MemberPtr ptr;
    
    FieldAccessor(MemberPtr p) : ptr(p) {}
    
    const FieldType& get(const Entity& e) const {
        return e.*ptr;
    }
    
    void set(Entity& e, const FieldType& value) const {
        e.*ptr = value;
    }
};

/**
 * @brief Column descriptor with type-erased getter/setter
 */
template<typename Entity>
struct ColumnDescriptor {
    ColumnInfo info;
    std::function<std::string(const Entity&)> toString;
    std::function<void(Entity&, const char*)> fromString;
    std::function<bool(const Entity&)> isNull;
};

/**
 * @brief Entity metadata registry
 */
template<typename Entity>
class EntityMetadata {
public:
    using DescriptorList = std::vector<ColumnDescriptor<Entity>>;
    
private:
    std::string_view tableName_;
    DescriptorList columns_;
    const ColumnDescriptor<Entity>* primaryKey_{nullptr};
    
public:
    EntityMetadata(std::string_view tableName)
        : tableName_(tableName) {}
    
    template<typename FieldType>
    void addColumn(std::string_view fieldName,
                   std::string_view columnName,
                   FieldType Entity::* memberPtr,
                   ColumnFlags flags = ColumnFlags::None) {
        ColumnDescriptor<Entity> desc;
        desc.info = ColumnInfo{
            fieldName,
            columnName,
            PgTypeTraits<FieldType>::pgOid,
            flags,
            PgTypeTraits<FieldType>::isNullable
        };
        
        desc.toString = [memberPtr](const Entity& e) -> std::string {
            return PgTypeTraits<FieldType>::toString(e.*memberPtr);
        };
        
        desc.fromString = [memberPtr](Entity& e, const char* str) {
            if constexpr (isOptionalV<FieldType>) {
                if (str) {
                    e.*memberPtr = PgTypeTraits<typename FieldType::value_type>::fromString(str);
                } else {
                    e.*memberPtr = std::nullopt;
                }
            } else {
                e.*memberPtr = PgTypeTraits<FieldType>::fromString(str);
            }
        };
        
        desc.isNull = [memberPtr](const Entity& e) -> bool {
            if constexpr (isOptionalV<FieldType>) {
                return !(e.*memberPtr).has_value();
            } else {
                return false;
            }
        };
        
        columns_.push_back(std::move(desc));
        
        if (hasFlag(flags, ColumnFlags::PrimaryKey)) {
            primaryKey_ = &columns_.back();
        }
    }
    
    [[nodiscard]] std::string_view tableName() const noexcept {
        return tableName_;
    }
    
    [[nodiscard]] const DescriptorList& columns() const noexcept {
        return columns_;
    }
    
    [[nodiscard]] const ColumnDescriptor<Entity>* primaryKey() const noexcept {
        return primaryKey_;
    }
    
    [[nodiscard]] const ColumnDescriptor<Entity>* findColumn(std::string_view name) const {
        for (const auto& col : columns_) {
            if (col.info.columnName == name) {
                return &col;
            }
        }
        return nullptr;
    }
};

} // namespace orm
} // namespace pq

// ============================================================================
// ENTITY MAPPING MACROS
// ============================================================================

/**
 * @brief Flags for column properties
 */
#define PQ_PRIMARY_KEY    pq::orm::ColumnFlags::PrimaryKey
#define PQ_AUTO_INCREMENT pq::orm::ColumnFlags::AutoIncrement
#define PQ_NOT_NULL       pq::orm::ColumnFlags::NotNull
#define PQ_UNIQUE         pq::orm::ColumnFlags::Unique
#define PQ_INDEX          pq::orm::ColumnFlags::Index
#define PQ_NONE           pq::orm::ColumnFlags::None

/**
 * @brief Begin entity definition
 * 
 * @param EntityType The C++ struct/class name
 * @param TableName The PostgreSQL table name (string literal)
 */
#define PQ_ENTITY(EntityType, TableName)                                       \
    using _PqEntityType = EntityType;                                          \
    static constexpr std::string_view _pqTableName = TableName;                \
                                                                               \
    static pq::orm::EntityMetadata<EntityType>& _pqMetadata() {                \
        static pq::orm::EntityMetadata<EntityType> meta(TableName);            \
        static bool initialized = false;                                       \
        if (!initialized) {                                                    \
            initialized = true;                                                \
            _pqRegisterColumns(meta);                                          \
        }                                                                      \
        return meta;                                                           \
    }                                                                          \
                                                                               \
    static void _pqRegisterColumns(pq::orm::EntityMetadata<EntityType>& meta) {

/**
 * @brief Define a column mapping
 * 
 * @param Field The C++ member variable name
 * @param ColumnName The PostgreSQL column name
 * @param ... Optional flags (PQ_PRIMARY_KEY, PQ_AUTO_INCREMENT, etc.)
 */
#define PQ_COLUMN(Field, ColumnName, ...)                                      \
    meta.addColumn(#Field, ColumnName, &_PqEntityType::Field,                  \
                   pq::orm::ColumnFlags::None __VA_OPT__(|) __VA_ARGS__);

/**
 * @brief Shorthand for column where C++ field name matches column name
 */
#define PQ_COLUMN_SAME(Field, ...)                                             \
    PQ_COLUMN(Field, #Field, __VA_ARGS__)

/**
 * @brief End entity definition
 */
#define PQ_ENTITY_END()                                                        \
    }

// ============================================================================
// ENTITY TRAITS SPECIALIZATION
// ============================================================================

/**
 * @brief Generate EntityMeta specialization from macro-defined metadata
 */
#define PQ_REGISTER_ENTITY(EntityType)                                         \
    namespace pq { namespace orm {                                             \
    template<>                                                                 \
    struct EntityMeta<EntityType> {                                            \
        static constexpr std::string_view tableName = EntityType::_pqTableName;\
                                                                               \
        static EntityMetadata<EntityType>& metadata() {                        \
            return EntityType::_pqMetadata();                                  \
        }                                                                      \
    };                                                                         \
    }}
