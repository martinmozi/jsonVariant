#include "internal/schemaValidator.h"
#include <stdexcept>
#include <regex>
#include <set>

void JsonSerializationInternal::SchemaValidator::validate(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& jsonVariant) const
{
   if (schemaVariant.type() != JsonSerialization::Variant::Type::Map)
       throw std::runtime_error("Bad schema type");

   const auto& schemaVariantMap = schemaVariant.toMap();
   compare(schemaVariantMap, jsonVariant, schemaVariantMap);
}

const JsonSerialization::Variant* JsonSerializationInternal::SchemaValidator::valueFromMap(const JsonSerialization::VariantMap& schemaVariantMap, const char* key, JsonSerialization::Variant::Type type, bool required, bool *isRef) const
{
    const auto it = schemaVariantMap.find(key);
    if (it == schemaVariantMap.end())
    {
        if (required)
        {
            const auto it = schemaVariantMap.find("$ref");
            if (it == schemaVariantMap.end())
            {
                throw std::runtime_error("Missing type in schema");
            }
            else
            {
                if (it->second.type() != JsonSerialization::Variant::Type::String)
                    throw std::runtime_error("Expected string for $ref in schema");
                *isRef = true;
                return &it->second;
            }
        }
        else
        {
            return nullptr;
        }
    }
    
    if (it->second.type() != type)
        throw std::runtime_error("Expected string for type in schema");

    return &it->second;
}

std::vector<std::string> JsonSerializationInternal::SchemaValidator::tokenize(const std::string &str, char delim) const
{
    std::vector<std::string> outVector;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        std::string pathStr = str.substr(start, end - start);
        if (pathStr != "#")
            outVector.push_back(pathStr);
    }
    return outVector;
}
const JsonSerialization::VariantMap& JsonSerializationInternal::SchemaValidator::fromRef(const std::string &refPath,  const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    std::vector<std::string> pathVector = tokenize(refPath, '/');
    const JsonSerialization::VariantMap * pVariantMap = &wholeSchemaVariantMap;
    for (const std::string & s : pathVector)
    {
        const auto it = pVariantMap->find(s);
        if (it == pVariantMap->end())
            throw std::runtime_error("Unable find ref according path");
        if (it->second.type() != JsonSerialization::Variant::Type::Map)
            throw std::runtime_error("Ref link is not valid");
        pVariantMap = &it->second.toMap();
    }
    return *pVariantMap;
}
void JsonSerializationInternal::SchemaValidator::compare(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    // todo enum and const - not implemented
    bool isRef = false;
    const std::string & typeStr = valueFromMap(schemaVariantMap, "type", JsonSerialization::Variant::Type::String, true, &isRef)->toString();
    if (isRef)
    {
        compare(fromRef(typeStr, wholeSchemaVariantMap), jsonVariant, wholeSchemaVariantMap);
    }
    else
    {
        if (typeStr == "object")
        {
            compareMap(schemaVariantMap, jsonVariant, wholeSchemaVariantMap);
        }
        else if (typeStr == "array")
        {
            compareVector(schemaVariantMap, jsonVariant, wholeSchemaVariantMap);
        }
        else if (typeStr == "integer")
        {
            compareInteger(schemaVariantMap, jsonVariant);
        }
        else if (typeStr == "number")
        {
            compareNumber(schemaVariantMap, jsonVariant);
        }
        else if (typeStr == "null")
        {
            checkNull(jsonVariant);
        }
        else if (typeStr == "boolean")
        {
            checkBoolean(jsonVariant);
        }
        else if (typeStr == "string")
        {
            compareString(schemaVariantMap, jsonVariant);
        }
        else
        {
            throw std::runtime_error("Unsupported type in json schema");
        }
    }
}
void JsonSerializationInternal::SchemaValidator::compareMap(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    std::set<std::string> required;
    const JsonSerialization::VariantMap &propertiesVariantMap = valueFromMap(schemaVariantMap, "properties", JsonSerialization::Variant::Type::Map, true)->toMap();
    const JsonSerialization::Variant *pRequiredVariant = valueFromMap(schemaVariantMap, "required", JsonSerialization::Variant::Type::Vector, false);
    if (pRequiredVariant)
    {
        const JsonSerialization::VariantVector& requiredVariantVector = pRequiredVariant->toVector();
        for (const auto &v : requiredVariantVector)
            required.insert(v.toString());
    }
    if (jsonVariant.type() != JsonSerialization::Variant::Type::Map)
        throw std::runtime_error("Map required");
    const JsonSerialization::VariantMap &jsonVariantMap = jsonVariant.toMap();
    for (const auto& it : propertiesVariantMap)
    {
        if (it.second.type() != JsonSerialization::Variant::Type::Map)
            throw std::runtime_error(std::string("Missing map for key: ") + it.first);
        auto iter = jsonVariantMap.find(it.first);
        if (iter == jsonVariantMap.end() && required.find(it.first) != required.end())
            throw std::runtime_error(std::string("Missing key in map: ") + it.first);
        compare(it.second.toMap(), iter->second, wholeSchemaVariantMap);
    }
    const JsonSerialization::Variant *pMinProperties = valueFromMap(schemaVariantMap, "minProperties", JsonSerialization::Variant::Type::Number, false);
    if (pMinProperties && pMinProperties->toInt() > (int)jsonVariantMap.size())
        throw std::runtime_error("Size of map is smaller as defined in minProperties");
    const JsonSerialization::Variant *pMaxProperties = valueFromMap(schemaVariantMap, "maxProperties", JsonSerialization::Variant::Type::Number, false);
    if (pMaxProperties && pMaxProperties->toInt() < (int)jsonVariantMap.size())
        throw std::runtime_error("Size of map is greater as defined in maxProperties");
    const JsonSerialization::Variant *pDependentRequired = valueFromMap(schemaVariantMap, "dependentRequired", JsonSerialization::Variant::Type::Map, false);
    if (pDependentRequired)
        throw std::runtime_error("not supported dependentRequired");
}
void JsonSerializationInternal::SchemaValidator::compareVector(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    const auto it = schemaVariantMap.find("items");
    if (it == schemaVariantMap.end())
        throw std::runtime_error("Expected items for schema vector");
    const JsonSerialization::VariantMap* pSchemaVariantMap = nullptr;
    const JsonSerialization::VariantVector* pSchemaVariantVector = nullptr;
    if (it->second.type() == JsonSerialization::Variant::Type::Map)
    {
        pSchemaVariantMap = &it->second.toMap();
    }
    else if (it->second.type() == JsonSerialization::Variant::Type::Vector)
    {
        const auto & schemaVariantVector = it->second.toVector();
        if (schemaVariantVector.empty())
            throw std::runtime_error("Expected non empty schema vector");
        if (schemaVariantVector.size() == 1)
        {
            const auto & schVariant = schemaVariantVector[0];
            if (schVariant.type() != JsonSerialization::Variant::Type::Map)
                throw std::runtime_error("Expected map for items vector in schema");
            pSchemaVariantMap = &schVariant.toMap();
        }
        else
        {
            pSchemaVariantVector = &schemaVariantVector;
        }
    }
    else
    {
        throw std::runtime_error("Expected map or vector for items in schema");
    }
    if (jsonVariant.type() != JsonSerialization::Variant::Type::Vector)
        throw std::runtime_error("Expected vector for items");
    const auto & variantVector = jsonVariant.toVector();
    if (pSchemaVariantMap != nullptr)
    {
        for (const auto& v : variantVector)
            compare(*pSchemaVariantMap, v, wholeSchemaVariantMap);
    }
    else if (pSchemaVariantVector != nullptr)
    {
        if (variantVector.size() != pSchemaVariantVector->size())
            throw std::runtime_error("Different size for heterogenous schema vector and checked vector");
        for (size_t i = 0; i < variantVector.size(); i++)
        {
            const auto& schVariant = pSchemaVariantVector->at(i);
            if (schVariant.type() != JsonSerialization::Variant::Type::Map)
                throw std::runtime_error("Expected map in json schema vector");
            compare(schVariant.toMap(), variantVector[i], wholeSchemaVariantMap);
        }    
    }
    const JsonSerialization::Variant *pMinItems = valueFromMap(schemaVariantMap, "minItems", JsonSerialization::Variant::Type::Number, false);
    if (pMinItems && pMinItems->toInt() > (int)variantVector.size())
        throw std::runtime_error("Too short vector");
    const JsonSerialization::Variant *pMaxItems = valueFromMap(schemaVariantMap, "maxItems", JsonSerialization::Variant::Type::Number, false);
    if (pMaxItems && pMaxItems->toInt() < (int)variantVector.size())
        throw std::runtime_error("Too long vector");
    const JsonSerialization::Variant *pMinContains = valueFromMap(schemaVariantMap, "minContains", JsonSerialization::Variant::Type::Number, false);
    if (pMinContains && pMinContains->toInt() > (int)variantVector.size())
        throw std::runtime_error("Too short vector");
    const JsonSerialization::Variant *pMaxContains = valueFromMap(schemaVariantMap, "maxContains", JsonSerialization::Variant::Type::Number, false);
    if (pMaxContains && pMaxContains->toInt() < (int)variantVector.size())
        throw std::runtime_error("Too long vector");
    const JsonSerialization::Variant *pUniqueItems = valueFromMap(schemaVariantMap, "uniqueItems", JsonSerialization::Variant::Type::Number, false);
    if (pUniqueItems && pUniqueItems->toBool() && variantVector.size() > 1)
    {
        for (const auto &v : variantVector)
        {
            size_t index = 1;
            for (size_t i = index; i < variantVector.size(); i++)
            {
                if (v == variantVector.at(i))
                    throw std::runtime_error("Some items in vector are not unique");
            }
        }
    }
}
void JsonSerializationInternal::SchemaValidator::compareString(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    if (jsonVariant.type() != JsonSerialization::Variant::Type::String)
        throw std::runtime_error("Expected string value");
    const std::string& value = jsonVariant.toString();
    const JsonSerialization::Variant *pMinLength = valueFromMap(schemaVariantMap, "minLength", JsonSerialization::Variant::Type::Number, false);
    if (pMinLength && pMinLength->toInt() > (int)value.size())
        throw std::runtime_error(std::string("Too short string: ") + value);
    const JsonSerialization::Variant *pMaxLength = valueFromMap(schemaVariantMap, "maxLength", JsonSerialization::Variant::Type::Number, false);
    if (pMaxLength && pMaxLength->toInt() < (int)value.size())
        throw std::runtime_error(std::string("Too long string: ") + value);
    const JsonSerialization::Variant *pPattern = valueFromMap(schemaVariantMap, "pattern", JsonSerialization::Variant::Type::String, false);
    if (pPattern)
    {
        std::regex expr(pPattern->toString());
        std::smatch sm;
        if (! std::regex_match(value, sm, expr))
            throw std::runtime_error(std::string("String doesn't match the pattern: ") + pPattern->toString());
    }
    const JsonSerialization::Variant *pFormat = valueFromMap(schemaVariantMap, "format", JsonSerialization::Variant::Type::String, false);
    if (pFormat)
    {
        // todo test and finish this
        std::string exprStr;
        const std::string& format = pFormat->toString();
        if (format == "date-time")
            exprStr = "xx"; //todo
        else if (format == "date")
            exprStr = "xx"; //todo
        else if (format == "time")
            exprStr = "xx"; //todo
        else if (format == "email")
            exprStr = "xx"; //todo
        else if (format == "hostname")
            exprStr = R"(^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$");
        else if (format == "ipv4")
            exprStr = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
        else if (format == "ipv6")
            exprStr = R"((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])))";
        else if (format == "uri")
            exprStr = R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])";
        else if (format == "json-pointer") // todo consider this
            exprStr = "";
        if (!exprStr.empty())
        {
            std::regex expr(pPattern->toString());
            std::smatch sm;
            if (! std::regex_match(value, sm, expr))
                throw std::runtime_error(std::string("String doesn't match the format pattern: ") + format);
        }
    }
}
void JsonSerializationInternal::SchemaValidator::compareNumber(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    if (jsonVariant.type() != JsonSerialization::Variant::Type::Number)
        throw std::runtime_error("Expected numeric value");
    double value = jsonVariant.toNumber();
    const JsonSerialization::Variant *pMinimum = valueFromMap(schemaVariantMap, "minimum", JsonSerialization::Variant::Type::Number, false);
    if (pMinimum && pMinimum->toNumber() > value)
        throw std::runtime_error("Numeric value is smaller than minimum");
    const JsonSerialization::Variant *pMaximum = valueFromMap(schemaVariantMap, "maximum", JsonSerialization::Variant::Type::Number, false);
    if (pMaximum && pMaximum->toNumber() < value)
        throw std::runtime_error("Numeric value is greater than maximum");
    const JsonSerialization::Variant *pExclusiveMinimum = valueFromMap(schemaVariantMap, "exclusiveMinimum", JsonSerialization::Variant::Type::Number, false);
    if (pExclusiveMinimum && pExclusiveMinimum->toNumber() >= value)
        throw std::runtime_error("Numeric value is smaller than exclusive minimum");
    const JsonSerialization::Variant *pExclusiveMaximum = valueFromMap(schemaVariantMap, "exclusiveMaximum", JsonSerialization::Variant::Type::Number, false);
    if (pExclusiveMaximum && pExclusiveMaximum->toNumber() <= value)
        throw std::runtime_error("Numeric value is greater than exclusive maximum");
    const JsonSerialization::Variant *pMultipleOf = valueFromMap(schemaVariantMap, "multipleOf", JsonSerialization::Variant::Type::Number, false);
    if (pMultipleOf)
    {
        double multipleOf = pMultipleOf->toNumber();
        if (!isInteger(multipleOf) || multipleOf <= 0)
            throw std::runtime_error("Multiple of has to be an positive number");
        if (! isInteger(value / multipleOf))
            throw std::runtime_error("Multiple of division must be an integer");

    }
}
void JsonSerializationInternal::SchemaValidator::compareInteger(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    compareNumber(schemaVariantMap, jsonVariant);
    if (! isInteger(jsonVariant.toNumber()))
        throw std::runtime_error("Expected integer value");
}
void JsonSerializationInternal::SchemaValidator::checkBoolean(const JsonSerialization::Variant& jsonVariant) const
{
    if (jsonVariant.type() != JsonSerialization::Variant::Type::Bool)
        throw std::runtime_error("Expected boolean value");
}
void JsonSerializationInternal::SchemaValidator::checkNull(const JsonSerialization::Variant& jsonVariant) const
{
    if (jsonVariant.type() != JsonSerialization::Variant::Type::Null)
        throw std::runtime_error("Expected null value");
}
