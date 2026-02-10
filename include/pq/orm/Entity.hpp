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
#include <mutex>

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
    int maxLength{-1};               // VARCHAR/CHAR length limit (-1: unspecified)
    
    [[nodiscard]] bool isPrimaryKey() const noexcept {
        return hasFlag(flags, ColumnFlags::PrimaryKey);
    }
    
    [[nodiscard]] bool isAutoIncrement() const noexcept {
        return hasFlag(flags, ColumnFlags::AutoIncrement);
    }

    [[nodiscard]] bool hasLengthLimit() const noexcept {
        return maxLength > 0;
    }
};

/**
 * @brief Schema validation mode
 */
enum class SchemaValidationMode : uint8_t {
    Strict,
    Lenient
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
    bool autoValidateSchema = false;  // Validate schema on first repository use
    SchemaValidationMode schemaValidationMode = SchemaValidationMode::Strict;
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
    std::vector<std::size_t> primaryKeyIndices_;
    
public:
    EntityMetadata(std::string_view tableName)
        : tableName_(tableName) {}
    
    template<typename FieldType>
    void addColumn(std::string_view fieldName,
                   std::string_view columnName,
                   FieldType Entity::* memberPtr,
                   ColumnFlags flags = ColumnFlags::None,
                   int maxLength = -1) {
        ColumnDescriptor<Entity> desc;
        desc.info = ColumnInfo{
            fieldName,
            columnName,
            PgTypeTraits<FieldType>::pgOid,
            flags,
            PgTypeTraits<FieldType>::isNullable,
            maxLength
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
        
        if constexpr (isOptionalV<FieldType>) {
            desc.isNull = [memberPtr](const Entity& e) -> bool {
                return !(e.*memberPtr).has_value();
            };
        } else {
            desc.isNull = [](const Entity&) -> bool {
                return false;
            };
        }
        
        columns_.push_back(std::move(desc));
        
        if (hasFlag(flags, ColumnFlags::PrimaryKey)) {
            primaryKeyIndices_.push_back(columns_.size() - 1);
        }
    }
    
    [[nodiscard]] std::string_view tableName() const noexcept {
        return tableName_;
    }
    
    [[nodiscard]] const DescriptorList& columns() const noexcept {
        return columns_;
    }
    
    [[nodiscard]] const ColumnDescriptor<Entity>* primaryKey() const noexcept {
        if (primaryKeyIndices_.empty()) {
            return nullptr;
        }
        return &columns_[primaryKeyIndices_.front()];
    }

    [[nodiscard]] const std::vector<std::size_t>& primaryKeyIndices() const noexcept {
        return primaryKeyIndices_;
    }

    [[nodiscard]] std::vector<const ColumnDescriptor<Entity>*> primaryKeys() const {
        std::vector<const ColumnDescriptor<Entity>*> pks;
        pks.reserve(primaryKeyIndices_.size());

        for (const auto index : primaryKeyIndices_) {
            pks.push_back(&columns_[index]);
        }

        return pks;
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
        static std::once_flag initialized;                                     \
        std::call_once(initialized, []() {                                     \
            _pqRegisterColumns(meta);                                          \
        });                                                                    \
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
 * @brief Define a VARCHAR/CHAR-like column with length metadata
 *
 * @param Field The C++ member variable name
 * @param ColumnName The PostgreSQL column name
 * @param Length Maximum length (n in VARCHAR(n))
 * @param ... Optional flags (PQ_PRIMARY_KEY, PQ_AUTO_INCREMENT, etc.)
 */
#define PQ_COLUMN_VARCHAR(Field, ColumnName, Length, ...)                       \
    meta.addColumn(#Field, ColumnName, &_PqEntityType::Field,                  \
                   pq::orm::ColumnFlags::None __VA_OPT__(|) __VA_ARGS__,       \
                   Length);

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
