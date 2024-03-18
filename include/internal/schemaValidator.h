#ifndef SCHEMAVALIDATOR_H
#define SCHEMAVALIDATOR_H

#include "jsonVariant.h"

namespace JsonSerializationInternal
{
    inline bool isInteger(double d)
    {
        return d == (int) d;
    }

    // todo add ref and definition support
    class SchemaValidator
    {
    public:
        SchemaValidator() = default;
        void validate(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& jsonVariant) const;

    private:
        const JsonSerialization::Variant* valueFromMap(const JsonSerialization::VariantMap& schemaVariantMap, const char* key, JsonSerialization::Variant::Type type, bool required, bool* isRef = nullptr) const;
        void compare(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const;
        void compareMap(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const;
        void compareVector(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const;
        void compareString(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const;
        void compareNumber(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const;
        void compareInteger(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const;
        void checkBoolean(const JsonSerialization::Variant& jsonVariant) const;
        void checkNull(const JsonSerialization::Variant& jsonVariant) const;
        const JsonSerialization::VariantMap& fromRef(const std::string &refPath, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const;
        std::vector<std::string> tokenize(const std::string &str, char delim) const;
    };
}

#endif // SCHEMAVALIDATOR_H
