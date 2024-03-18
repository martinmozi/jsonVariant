#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <string>
#include <vector>
#include <map>
#include "jsonVariant.h"

namespace JsonSerializationInternal
{
    class JsonParser
    {
    public:
        JsonParser() = default;
        void fromJson(const std::string& jsonStr, const std::string& jsonSchema, JsonSerialization::Variant& jsonVariant) const;
        void fromJson(const std::string& jsonStr, JsonSerialization::Variant& jsonVariant) const;

    private:
        JsonSerialization::VariantVector parseArray(const char *& pData) const;
        JsonSerialization::VariantMap parseMap(const char *& pData) const;
        JsonSerialization::Variant parseObject(const char *& pData) const;
        std::string parseKey(const char *& pData) const;
        JsonSerialization::Variant parseValue(const char *& pData) const;
        std::string parseString(const char *& pData) const;
        bool parseBoolean(const char *& pData) const;
        double parseNumber(const char *& pData) const;
        JsonSerialization::Variant parseNull(const char *& pData) const;
        void gotoValue(const char *& pData) const;
        std::string trim(const std::string& jsonStr) const;
        bool isIgnorable(char d) const;
    };
}

#endif // JSONPARSER_H
