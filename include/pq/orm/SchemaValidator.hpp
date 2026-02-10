#pragma once

/**
 * @file SchemaValidator.hpp
 * @brief Entity-to-database schema validation utilities
 */

#include "Entity.hpp"
#include "../core/Connection.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <typeinfo>

namespace pq {
namespace orm {

enum class ValidationIssueType : uint8_t {
    ConnectionError,
    TableNotFound,
    ColumnNotFound,
    TypeMismatch,
    NullableMismatch,
    LengthMismatch,
    ExtraColumn
};

struct ValidationIssue {
    ValidationIssueType type{ValidationIssueType::ConnectionError};
    std::string entityName;
    std::string tableName;
    std::string columnName;
    std::string expected;
    std::string actual;
    std::string message;
};

struct ValidationResult {
    std::vector<ValidationIssue> errors;
    std::vector<ValidationIssue> warnings;

    [[nodiscard]] bool isValid() const noexcept {
        return errors.empty();
    }

    [[nodiscard]] std::size_t errorCount() const noexcept {
        return errors.size();
    }

    [[nodiscard]] std::size_t warningCount() const noexcept {
        return warnings.size();
    }

    [[nodiscard]] std::string summary() const {
        std::ostringstream oss;
        oss << "errors=" << errors.size() << ", warnings=" << warnings.size();

        if (!errors.empty()) {
            oss << ", first_error=\"" << errors.front().message << "\"";
        } else if (!warnings.empty()) {
            oss << ", first_warning=\"" << warnings.front().message << "\"";
        }

        return oss.str();
    }
};

class SchemaValidator {
public:
    explicit SchemaValidator(SchemaValidationMode mode = SchemaValidationMode::Strict)
        : mode_(mode) {}

    [[nodiscard]] SchemaValidationMode mode() const noexcept {
        return mode_;
    }

    template<typename Entity>
    [[nodiscard]] ValidationResult validate(core::Connection& conn) const {
        static_assert(isEntityV<Entity>, "Entity must be registered with PQ_REGISTER_ENTITY");

        ValidationResult result;
        const auto& meta = EntityMeta<Entity>::metadata();
        auto tableRef = splitQualifiedTableName(meta.tableName());
        std::string schemaName = tableRef.schemaName;
        const std::string tableName = tableRef.tableName;
        const std::string displayTableName = tableRef.displayName();
        const std::string entityName = typeid(Entity).name();

        if (!conn.isConnected()) {
            addError(result,
                     ValidationIssueType::ConnectionError,
                     entityName,
                     tableName,
                     "",
                     "",
                     "",
                     "Schema validation failed: connection is not established");
            return result;
        }

        const auto tableExists = [&]() -> DbResult<core::QueryResult> {
            if (tableRef.schemaExplicit) {
                return conn.execute(
                    "SELECT table_schema FROM information_schema.tables "
                    "WHERE table_schema = $1 AND table_name = $2 "
                    "LIMIT 1",
                    std::vector<std::string>{schemaName, tableName});
            }

            return conn.execute(
                "SELECT table_schema FROM information_schema.tables "
                "WHERE table_name = $1 "
                "AND table_schema = ANY(current_schemas(true)) "
                "ORDER BY COALESCE(array_position(current_schemas(true), table_schema), 2147483647) "
                "LIMIT 1",
                std::vector<std::string>{tableName});
        }();
        if (!tableExists) {
            addError(result,
                     ValidationIssueType::ConnectionError,
                     entityName,
                     displayTableName,
                     "",
                     "",
                     "",
                     "Schema validation query failed while checking table existence: " +
                         tableExists.error().message);
            return result;
        }

        if (tableExists->empty()) {
            addMismatch(result,
                        ValidationIssueType::TableNotFound,
                        entityName,
                        displayTableName,
                        "",
                        std::string(meta.tableName()),
                        "<missing>",
                        "Table not found in database",
                        true);
            return result;
        }

        if (!tableRef.schemaExplicit) {
            schemaName = tableExists->row(0).template get<std::string>("table_schema");
        }

        const auto columnsResult = conn.execute(
            "SELECT column_name, is_nullable, data_type, udt_name, "
            "COALESCE(character_maximum_length, -1) AS max_length "
            "FROM information_schema.columns "
            "WHERE table_schema = $1 AND table_name = $2 "
            "ORDER BY ordinal_position",
            std::vector<std::string>{schemaName, tableName});
        if (!columnsResult) {
            addError(result,
                     ValidationIssueType::ConnectionError,
                     entityName,
                     displayTableName,
                     "",
                     "",
                     "",
                     "Schema validation query failed while reading columns: " +
                         columnsResult.error().message);
            return result;
        }

        std::unordered_map<std::string, DbColumn> dbColumns;
        dbColumns.reserve(static_cast<std::size_t>(columnsResult->rowCount()));

        for (const auto& row : *columnsResult) {
            DbColumn dbColumn;
            dbColumn.columnName = row.get<std::string>("column_name");
            dbColumn.dataType = toLower(row.get<std::string>("data_type"));
            dbColumn.udtName = toLower(row.get<std::string>("udt_name"));
            dbColumn.isNullable = (toLower(row.get<std::string>("is_nullable")) == "yes");
            dbColumn.maxLength = row.get<int>("max_length");
            dbColumns.emplace(dbColumn.columnName, std::move(dbColumn));
        }

        std::unordered_set<std::string> entityColumnNames;
        entityColumnNames.reserve(meta.columns().size());

        for (const auto& column : meta.columns()) {
            const std::string columnNameStr(column.info.columnName);
            entityColumnNames.insert(columnNameStr);

            const auto it = dbColumns.find(columnNameStr);
            if (it == dbColumns.end()) {
                addMismatch(result,
                            ValidationIssueType::ColumnNotFound,
                            entityName,
                            displayTableName,
                            columnNameStr,
                            columnNameStr,
                            "<missing>",
                            "Column defined in entity is missing in database",
                            true);
                continue;
            }

            const auto typeValidation = validateColumnType(column.info, it->second);
            if (!typeValidation.typeCompatible) {
                addMismatch(result,
                            ValidationIssueType::TypeMismatch,
                            entityName,
                            displayTableName,
                            columnNameStr,
                            typeValidation.expectedType,
                            typeValidation.actualType,
                            "Column type mismatch",
                            true);
            }

            if (typeValidation.lengthMismatch) {
                addMismatch(result,
                            ValidationIssueType::LengthMismatch,
                            entityName,
                            displayTableName,
                            columnNameStr,
                            typeValidation.expectedLength,
                            typeValidation.actualLength,
                            "VARCHAR/CHAR length mismatch",
                            true);
            }

            if (column.info.isNullable != it->second.isNullable) {
                addMismatch(result,
                            ValidationIssueType::NullableMismatch,
                            entityName,
                            displayTableName,
                            columnNameStr,
                            column.info.isNullable ? "nullable" : "not-null",
                            it->second.isNullable ? "nullable" : "not-null",
                            "Column nullable constraint mismatch",
                            true);
            }
        }

        for (const auto& [dbColumnName, _] : dbColumns) {
            if (entityColumnNames.find(dbColumnName) == entityColumnNames.end()) {
                addMismatch(result,
                            ValidationIssueType::ExtraColumn,
                            entityName,
                            displayTableName,
                            dbColumnName,
                            "<not mapped>",
                            dbColumnName,
                            "Database has extra column not mapped in entity",
                            false);
            }
        }

        return result;
    }

private:
    struct DbColumn {
        std::string columnName;
        std::string dataType;
        std::string udtName;
        bool isNullable{false};
        int maxLength{-1};
    };

    struct TypeValidationResult {
        bool typeCompatible{true};
        bool lengthMismatch{false};
        std::string expectedType;
        std::string actualType;
        std::string expectedLength;
        std::string actualLength;
    };

    SchemaValidationMode mode_;

    static void addError(ValidationResult& result,
                         ValidationIssueType type,
                         const std::string& entityName,
                         const std::string& tableName,
                         const std::string& columnName,
                         const std::string& expected,
                         const std::string& actual,
                         const std::string& message) {
        result.errors.push_back(ValidationIssue{
            type, entityName, tableName, columnName, expected, actual, message});
    }

    void addMismatch(ValidationResult& result,
                     ValidationIssueType type,
                     const std::string& entityName,
                     const std::string& tableName,
                     const std::string& columnName,
                     const std::string& expected,
                     const std::string& actual,
                     const std::string& message,
                     bool strictAsError) const {
        ValidationIssue issue{type, entityName, tableName, columnName, expected, actual, message};

        if (strictAsError && mode_ == SchemaValidationMode::Strict) {
            result.errors.push_back(std::move(issue));
        } else {
            result.warnings.push_back(std::move(issue));
        }
    }

    struct TableReference {
        std::string schemaName;
        std::string tableName;
        bool schemaExplicit{false};

        [[nodiscard]] std::string displayName() const {
            if (schemaExplicit) {
                return schemaName + "." + tableName;
            }
            return tableName;
        }
    };

    static TableReference splitQualifiedTableName(std::string_view tableName) {
        const auto dotPos = tableName.find('.');
        if (dotPos == std::string_view::npos) {
            return {"", std::string(tableName), false};
        }

        return {std::string(tableName.substr(0, dotPos)),
                std::string(tableName.substr(dotPos + 1)),
                true};
    }

    static std::string toLower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return value;
    }

    static bool isTypeInSet(std::string_view actual, const std::vector<std::string_view>& expected) {
        for (const auto expectedType : expected) {
            if (actual == expectedType) {
                return true;
            }
        }
        return false;
    }

    static std::string canonicalDbType(const std::string& dataType, const std::string& udtName) {
        if (dataType == "user-defined") {
            return udtName;
        }
        return dataType;
    }

    static std::string expectedTypeName(const ColumnInfo& info) {
        if (info.maxLength > 0) {
            return "varchar(" + std::to_string(info.maxLength) + ")";
        }

        switch (info.pgType) {
            case oid::BOOL: return "boolean";
            case oid::INT2: return "smallint";
            case oid::INT4: return "integer";
            case oid::INT8: return "bigint";
            case oid::FLOAT4: return "real";
            case oid::FLOAT8: return "double precision";
            case oid::TEXT: return "text";
            case oid::DATE: return "date";
            case oid::TIME: return "time";
            case oid::TIMESTAMP: return "timestamp";
            case oid::TIMESTAMPTZ: return "timestamptz";
            case oid::NUMERIC: return "numeric";
            case oid::UUID: return "uuid";
            case oid::JSONB: return "jsonb";
            default: return "oid(" + std::to_string(info.pgType) + ")";
        }
    }

    static TypeValidationResult validateColumnType(const ColumnInfo& info,
                                                   const DbColumn& dbColumn) {
        TypeValidationResult result;
        result.expectedType = expectedTypeName(info);
        result.actualType = canonicalDbType(dbColumn.dataType, dbColumn.udtName);

        if (info.maxLength > 0) {
            const bool varcharFamily = isTypeInSet(
                result.actualType,
                {"character varying", "varchar", "character", "char"});
            result.typeCompatible = varcharFamily;

            const int actualLength = dbColumn.maxLength;
            result.expectedLength = std::to_string(info.maxLength);
            result.actualLength = (actualLength < 0) ? "<unbounded>" : std::to_string(actualLength);
            result.lengthMismatch = (actualLength != info.maxLength);
            return result;
        }

        switch (info.pgType) {
            case oid::BOOL:
                result.typeCompatible = isTypeInSet(result.actualType, {"boolean", "bool"});
                break;
            case oid::INT2:
                result.typeCompatible = isTypeInSet(result.actualType, {"smallint", "int2"});
                break;
            case oid::INT4:
                result.typeCompatible = isTypeInSet(result.actualType, {"integer", "int4"});
                break;
            case oid::INT8:
                result.typeCompatible = isTypeInSet(result.actualType, {"bigint", "int8"});
                break;
            case oid::FLOAT4:
                result.typeCompatible = isTypeInSet(result.actualType, {"real", "float4"});
                break;
            case oid::FLOAT8:
                result.typeCompatible = isTypeInSet(result.actualType, {"double precision", "float8"});
                break;
            case oid::TEXT:
                result.typeCompatible = isTypeInSet(
                    result.actualType,
                    {"text", "character varying", "varchar", "character", "char"});
                break;
            case oid::DATE:
                result.typeCompatible = isTypeInSet(result.actualType, {"date"});
                break;
            case oid::TIME:
                result.typeCompatible = isTypeInSet(
                    result.actualType, {"time", "time without time zone"});
                break;
            case oid::TIMESTAMP:
                result.typeCompatible = isTypeInSet(
                    result.actualType, {"timestamp", "timestamp without time zone"});
                break;
            case oid::TIMESTAMPTZ:
                result.typeCompatible = isTypeInSet(
                    result.actualType, {"timestamptz", "timestamp with time zone"});
                break;
            case oid::NUMERIC:
                result.typeCompatible = isTypeInSet(result.actualType, {"numeric", "decimal"});
                break;
            case oid::UUID:
                result.typeCompatible = isTypeInSet(result.actualType, {"uuid"});
                break;
            case oid::JSONB:
                result.typeCompatible = isTypeInSet(result.actualType, {"jsonb"});
                break;
            default:
                result.typeCompatible = true;
                break;
        }

        return result;
    }
};

} // namespace orm
} // namespace pq
