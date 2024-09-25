#include "jsonVariant.h"
#include <regex>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <ostream>
#include <set>

// Platform-specific newline string
#ifdef _WIN32
    const std::string endLineStr = "\r\n";
#else
    const std::string endLineStr = "\n";
#endif

namespace JsonSerializationInternal 
{
    namespace
    {
        inline bool isIgnorable(char d) {
            return d == ' ' || d == '\n' || d == '\t' || d == '\r';
        }

        inline bool isInteger(double d)
        {
            return d == (int) d;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class SchemaValidator
    {
    public:
        static void validate(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& jsonVariant)
        {
            if (schemaVariant.type() != JsonSerialization::Type::Map)
                throw std::runtime_error("Bad schema type");

            const auto& schemaVariantMap = schemaVariant.toMap();
            compare(schemaVariantMap, jsonVariant, schemaVariantMap);
        }

    private:
        static const JsonSerialization::Variant* valueFromMap(const JsonSerialization::VariantMap& schemaVariantMap, const char* key, JsonSerialization::Type type, bool required, bool *isRef = nullptr)
        {
            const auto it = schemaVariantMap.find(key);
            if (it == schemaVariantMap.end())
            {
                if (required)
                {
                    const auto iter = schemaVariantMap.find("$ref");
                    if (iter == schemaVariantMap.end())
                    {
                        throw std::runtime_error("Missing type in schema");
                    }
                    else
                    {
                        if (iter->second.type() != JsonSerialization::Type::String)
                            throw std::runtime_error("Expected string for $ref in schema");
                        if (isRef != nullptr)
                            *isRef = true;
                        
                        return &iter->second;
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

        static std::vector<std::string> tokenize(const std::string &str, char delim)
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

        static const JsonSerialization::VariantMap& fromRef(const std::string &refPath,  const JsonSerialization::VariantMap& wholeSchemaVariantMap)
        {
            std::vector<std::string> pathVector = tokenize(refPath, '/');
            const JsonSerialization::VariantMap * pVariantMap = &wholeSchemaVariantMap;
            for (const std::string & s : pathVector)
            {
                const auto it = pVariantMap->find(s);
                if (it == pVariantMap->end())
                    throw std::runtime_error("Unable find ref according path");
                if (it->second.type() != JsonSerialization::Type::Map)
                    throw std::runtime_error("Ref link is not valid");
                pVariantMap = &it->second.toMap();
            }
            return *pVariantMap;
        }

        static void compare(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap)
        {
            // todo enum and const - not implemented
            bool isRef = false;
            const std::string & typeStr = valueFromMap(schemaVariantMap, "type", JsonSerialization::Type::String, true, &isRef)->toString();
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

        static void compareMap(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap)
        {
            std::set<std::string> required;
            const JsonSerialization::VariantMap &propertiesVariantMap = valueFromMap(schemaVariantMap, "properties", JsonSerialization::Type::Map, true)->toMap();
            const JsonSerialization::Variant *pRequiredVariant = valueFromMap(schemaVariantMap, "required", JsonSerialization::Type::Vector, false);
            if (pRequiredVariant)
            {
                const JsonSerialization::VariantVector& requiredVariantVector = pRequiredVariant->toVector();
                for (const auto &v : requiredVariantVector)
                    required.insert(v.toString());
            }
            if (jsonVariant.type() != JsonSerialization::Type::Map)
                throw std::runtime_error("Map required");
            const JsonSerialization::VariantMap &jsonVariantMap = jsonVariant.toMap();
            for (const auto& it : propertiesVariantMap)
            {
                if (it.second.type() != JsonSerialization::Type::Map)
                    throw std::runtime_error(std::string("Missing map for key: ") + it.first);
                auto iter = jsonVariantMap.find(it.first);
                if (iter == jsonVariantMap.end() && required.find(it.first) != required.end())
                    throw std::runtime_error(std::string("Missing key in map: ") + it.first);
                compare(it.second.toMap(), iter->second, wholeSchemaVariantMap);
            }
            const JsonSerialization::Variant *pMinProperties = valueFromMap(schemaVariantMap, "minProperties", JsonSerialization::Type::Number, false);
            if (pMinProperties && pMinProperties->toInt() > (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is smaller as defined in minProperties");
            const JsonSerialization::Variant *pMaxProperties = valueFromMap(schemaVariantMap, "maxProperties", JsonSerialization::Type::Number, false);
            if (pMaxProperties && pMaxProperties->toInt() < (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is greater as defined in maxProperties");
            const JsonSerialization::Variant *pDependentRequired = valueFromMap(schemaVariantMap, "dependentRequired", JsonSerialization::Type::Map, false);
            if (pDependentRequired)
                throw std::runtime_error("not supported dependentRequired");
        }

        static void compareVector(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap)
        {
            const auto it = schemaVariantMap.find("items");
            if (it == schemaVariantMap.end())
                throw std::runtime_error("Expected items for schema vector");
            const JsonSerialization::VariantMap* pSchemaVariantMap = nullptr;
            const JsonSerialization::VariantVector* pSchemaVariantVector = nullptr;
            if (it->second.type() == JsonSerialization::Type::Map)
            {
                pSchemaVariantMap = &it->second.toMap();
            }
            else if (it->second.type() == JsonSerialization::Type::Vector)
            {
                const auto & schemaVariantVector = it->second.toVector();
                if (schemaVariantVector.empty())
                    throw std::runtime_error("Expected non empty schema vector");
                if (schemaVariantVector.size() == 1)
                {
                    const auto & schVariant = schemaVariantVector[0];
                    if (schVariant.type() != JsonSerialization::Type::Map)
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
            if (jsonVariant.type() != JsonSerialization::Type::Vector)
            {
                throw std::runtime_error("Expected vector for items");
            }

            const auto & variantVector = jsonVariant.toVector();
            if (pSchemaVariantMap != nullptr)
            {
                for (const auto& v : variantVector)
                    compare(*pSchemaVariantMap, v, wholeSchemaVariantMap);
            }
            else if (pSchemaVariantVector != nullptr)
            {
                if (variantVector.size() != pSchemaVariantVector->size())
                {
                    throw std::runtime_error("Different size for heterogenous schema vector and checked vector");
                }

                for (size_t i = 0; i < variantVector.size(); i++)
                {
                    const auto& schVariant = pSchemaVariantVector->at(i);
                    if (schVariant.type() != JsonSerialization::Type::Map)
                        throw std::runtime_error("Expected map in json schema vector");
                    compare(schVariant.toMap(), variantVector[i], wholeSchemaVariantMap);
                }    
            }
            const JsonSerialization::Variant *pMinItems = valueFromMap(schemaVariantMap, "minItems", JsonSerialization::Type::Number, false);
            if (pMinItems && pMinItems->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");
            const JsonSerialization::Variant *pMaxItems = valueFromMap(schemaVariantMap, "maxItems", JsonSerialization::Type::Number, false);
            if (pMaxItems && pMaxItems->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");
            const JsonSerialization::Variant *pMinContains = valueFromMap(schemaVariantMap, "minContains", JsonSerialization::Type::Number, false);
            if (pMinContains && pMinContains->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");
            const JsonSerialization::Variant *pMaxContains = valueFromMap(schemaVariantMap, "maxContains", JsonSerialization::Type::Number, false);
            if (pMaxContains && pMaxContains->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");
            const JsonSerialization::Variant *pUniqueItems = valueFromMap(schemaVariantMap, "uniqueItems", JsonSerialization::Type::Number, false);
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

        static void compareString(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant)
        {
            if (jsonVariant.type() != JsonSerialization::Type::String)
                throw std::runtime_error("Expected string value");
            const std::string& value = jsonVariant.toString();
            const JsonSerialization::Variant *pMinLength = valueFromMap(schemaVariantMap, "minLength", JsonSerialization::Type::Number, false);
            if (pMinLength && pMinLength->toInt() > (int)value.size())
                throw std::runtime_error(std::string("Too short string: ") + value);
            const JsonSerialization::Variant *pMaxLength = valueFromMap(schemaVariantMap, "maxLength", JsonSerialization::Type::Number, false);
            if (pMaxLength && pMaxLength->toInt() < (int)value.size())
                throw std::runtime_error(std::string("Too long string: ") + value);
            const JsonSerialization::Variant *pPattern = valueFromMap(schemaVariantMap, "pattern", JsonSerialization::Type::String, false);
            if (pPattern)
            {
                std::regex expr(pPattern->toString());
                std::smatch sm;
                if (! std::regex_match(value, sm, expr))
                    throw std::runtime_error(std::string("String doesn't match the pattern: ") + pPattern->toString());
            }
            const JsonSerialization::Variant *pFormat = valueFromMap(schemaVariantMap, "format", JsonSerialization::Type::String, false);
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

        static void compareNumber(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant)
        {
            if (jsonVariant.type() != JsonSerialization::Type::Number)
                throw std::runtime_error("Expected numeric value");
            double value = jsonVariant.toNumber();
            const JsonSerialization::Variant *pMinimum = valueFromMap(schemaVariantMap, "minimum", JsonSerialization::Type::Number, false);
            if (pMinimum && pMinimum->toNumber() > value)
                throw std::runtime_error("Numeric value is smaller than minimum");
            const JsonSerialization::Variant *pMaximum = valueFromMap(schemaVariantMap, "maximum", JsonSerialization::Type::Number, false);
            if (pMaximum && pMaximum->toNumber() < value)
                throw std::runtime_error("Numeric value is greater than maximum");
            const JsonSerialization::Variant *pExclusiveMinimum = valueFromMap(schemaVariantMap, "exclusiveMinimum", JsonSerialization::Type::Number, false);
            if (pExclusiveMinimum && pExclusiveMinimum->toNumber() >= value)
                throw std::runtime_error("Numeric value is smaller than exclusive minimum");
            const JsonSerialization::Variant *pExclusiveMaximum = valueFromMap(schemaVariantMap, "exclusiveMaximum", JsonSerialization::Type::Number, false);
            if (pExclusiveMaximum && pExclusiveMaximum->toNumber() <= value)
                throw std::runtime_error("Numeric value is greater than exclusive maximum");
            const JsonSerialization::Variant *pMultipleOf = valueFromMap(schemaVariantMap, "multipleOf", JsonSerialization::Type::Number, false);
            if (pMultipleOf)
            {
                double multipleOf = pMultipleOf->toNumber();
                if (!isInteger(multipleOf) || multipleOf <= 0)
                    throw std::runtime_error("Multiple of has to be an positive number");
                if (! isInteger(value / multipleOf))
                    throw std::runtime_error("Multiple of division must be an integer");

            }
        }

        static void compareInteger(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant)
        {
            compareNumber(schemaVariantMap, jsonVariant);
            if (! isInteger(jsonVariant.toNumber()))
                throw std::runtime_error("Expected integer value");
        }

        static void checkBoolean(const JsonSerialization::Variant& jsonVariant) 
        {
            if (jsonVariant.type() != JsonSerialization::Type::Bool)
                throw std::runtime_error("Expected boolean value");
        }

        static void checkNull(const JsonSerialization::Variant& jsonVariant) 
        {
            if (jsonVariant.type() != JsonSerialization::Type::Null)
                throw std::runtime_error("Expected null value");
        }
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class JsonParser
    {
    public:
        static void fromJson(const std::string& jsonStr, JsonSerialization::Variant& jsonVariant) {
            std::string _jsonStr = trim(jsonStr);
            const char *pData = _jsonStr.c_str();
            size_t len = _jsonStr.size();
            if (len < 2)
                throw std::runtime_error("No short json");

            jsonVariant = parseObject(pData);
        }

        static void fromJson(const std::string& jsonStr, const std::string& jsonSchema, JsonSerialization::Variant& jsonVariant) 
        {
            JsonSerialization::Variant schemaVariant, schemaVariantInternal;
            fromJson(jsonSchema, schemaVariant);
            fromJson(jsonStr, jsonVariant);
            SchemaValidator::validate(schemaVariant, jsonVariant);
        }

    private:
        static JsonSerialization::VariantVector parseArray(const char *& pData) {
            ++pData;
            JsonSerialization::VariantVector variantVector;
            while (pData != NULL)
            {
                variantVector.emplace_back(parseValue(pData));
                if (*pData == ']')
                {
                    ++pData;
                    return variantVector;
                }

                ++pData;
            }

            throw std::runtime_error("Unfinished vector");
        }

        static JsonSerialization::VariantMap parseMap(const char *& pData) {
            ++pData;
            JsonSerialization::VariantMap variantMap;
            while (pData != NULL)
            {
                bool isEmpty = false;
                if (*pData == '\"')
                {
                    std::string key = parseKey(pData);
                    gotoValue(pData);
                    variantMap.insert(std::make_pair(key, parseValue(pData)));
                }
                else
                {
                    isEmpty = true;
                }

                if (*pData == '}')
                {
                    ++pData;
                    return variantMap;
                }

                ++pData;
                if (isEmpty) 
                {
                    throw std::runtime_error("Wrong character in json");
                }
            }

            throw std::runtime_error("Unfinished map");
        }

        static JsonSerialization::Variant parseObject(const char *& pData) 
        {
            while (pData != NULL)
            {
                char c = *pData;
                if (c == '{')
                    return JsonSerialization::Variant(parseMap(pData));
                else if (c == '[')
                    return JsonSerialization::Variant(parseArray(pData));
                else
                    throw std::runtime_error("Invalid json - first char");

                ++pData;
            }

            throw std::runtime_error("Missing end of the object");
        }

        static std::string parseKey(const char *& pData) {
            std::string key;
            while (pData != NULL)
            {
                ++pData;
                char c = *pData;
                if (c == '\"') 
                    return key;                

                key.push_back(c);
            }

            throw std::runtime_error("Missing end of the key");
        }

        static JsonSerialization::Variant parseValue(const char *& pData)
        {
            JsonSerialization::Variant variant;
            char c = *pData;
            if (c == '{')
                variant = JsonSerialization::Variant(parseMap(pData));
            else if (c == '[')
                variant = JsonSerialization::Variant(parseArray(pData));
            else if (c == '\"')
                variant = JsonSerialization::Variant(parseString(pData));
            else if (c == 't' || c == 'f')
                variant = JsonSerialization::Variant(parseBoolean(pData));
            else if (c == 'n')
                variant = JsonSerialization::Variant(parseNull(pData));
            else if (isdigit(c) || c == '.' || c == '-')
            {
                double d = parseNumber(pData);
                variant = (d == (int)d) ? JsonSerialization::Variant((int)d) : JsonSerialization::Variant(d);
            }
            else
                throw std::runtime_error("Unknown character when parsing value");

            c = *pData;
            if (c == ',' || c == '}' || c == ']')
                return variant;

            throw std::runtime_error("Missing delimiter");
        }

        static std::string parseString(const char *& pData)
        {
            std::string potentionalString;
            while (pData != NULL)
            {
                ++pData;
                char c = *pData;
                if (c == '\\')  // potentional escaping
                {
                    ++pData;
                    if (pData == nullptr) {
                        throw std::runtime_error("Incorrect escaping in string value reading at the end");
                    }

                    c = *pData;
                    switch (c) {
                    case '"':  // Escape double quotes
                    case '\\': // Escape backslashes
                    case 'n':  // Escape newlines
                    case 'r':  // Escape carriage return
                    case 't':  // Escape tabs
                    case 'b':  // Escape backspace
                    case 'f':  // Escape form feed
                        potentionalString.push_back(*(pData - 1));
                        potentionalString.push_back(c);
                        break;
                    default:
                        throw std::runtime_error("Incorrect escaping in string value reading");
                    }

                    continue;
                }

                if (c == '\"')
                {
                    ++pData;
                    return potentionalString;
                }

                potentionalString.push_back(c);
            }

            throw std::runtime_error("Not finished string value reading");
        }

        static bool parseBoolean(const char *& pData)
        {
            bool t, f;
            t = f = true;
            static const char *trueStr = "true";
            static const char *falseStr = "false";
            for (int i = 0; i < 4; i++)
            {
                char c = *pData;
                ++pData;
                if (trueStr[i] != c)
                    t = false;
                else if (falseStr[i] != c)
                    f = false;
            }
        
            if (t)
                return true;

            if (f && *pData == 'e')
            {
                pData++;
                return false;
            }

            throw std::runtime_error("Unable to parse boolean value");
        }

        static double parseNumber(const char *& pData)
        {
            std::string potentionalNumber;
            while (pData != NULL)
            {
                char c = *pData;
                if (isdigit(c) || c == '.' || c == '-')
                {
                    potentionalNumber += c;
                }
                else
                {
                    try
                    {
                        return std::stod(potentionalNumber);
                    }
                    catch (const std::invalid_argument&) {
                        throw std::runtime_error("Invalid argument when number converting");
                    }
                    catch (const std::out_of_range&) {
                        throw std::runtime_error("Out of range value when number converting");
                    }
                }

                ++pData;
            }

            throw std::runtime_error("Not finished number value reading");
        }

        static JsonSerialization::Variant parseNull(const char *& pData)
        {
            static const char *nullStr = "null";
            for (int i = 0; i < 4; i ++)
            {
                if (nullStr[i] != *pData)
                    throw std::runtime_error("Unable to parse null value");

                pData ++;
            }

            return JsonSerialization::Variant(nullptr);
        }

        static void gotoValue(const char *& pData)
        {
            if(pData != NULL)
            {
                ++pData;
                if (*pData == ':')
                {
                    pData++;
                    return;
                }
            }

            throw std::runtime_error("Expected value delimiter");
        }

        static std::string trim(const std::string& jsonStr)
        {
            const char *pData = jsonStr.c_str();
            size_t len = jsonStr.size();
            std::string outStr(len, 0);
            int j = 0;
            char *p = (char*)outStr.data();
            bool record = false;
            for (size_t i = 0; i < len ;i++)
            {
                char c = pData[i];
                if (c == '\"' && i > 0 && pData[i - 1] != '\\')
                    record = !record;

                if (!isIgnorable(c) || record)
                {
                    p[j] = c;
                    j++;
                }
            }

            return outStr;
        }
    };
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////


void JsonSerialization::Variant::clear()
{
    switch (type_)
    {
    case JsonSerialization::Type::Number:
    case JsonSerialization::Type::Bool:
        break;

    case JsonSerialization::Type::String:
    {
        std::string *pString = (std::string *)pData_.pData;
        delete pString;
    }
    break;

    case JsonSerialization::Type::Vector:
    {
        JsonSerialization::VariantVector *pJsonVariantVector = (JsonSerialization::VariantVector *)pData_.pData;
        for (auto& jsonVariant : *pJsonVariantVector)
            jsonVariant.clear();

        delete pJsonVariantVector;
    }
    break;

    case JsonSerialization::Type::Map:
    {
        JsonSerialization::VariantMap *pJsonVariantMap = (JsonSerialization::VariantMap *)pData_.pData;
        for (auto &it : *pJsonVariantMap)
            it.second.clear();

        delete pJsonVariantMap;
    }
    break;

    default:
        break;
    }

    pData_.pData = nullptr;
    type_ = JsonSerialization::Type::Empty;
}

void JsonSerialization::Variant::copyAll(const JsonSerialization::Variant &value)
{
    type_ = value.type_;
    switch (type_)
    {
    case JsonSerialization::Type::Null:
        pData_.pData = nullptr;
        break;

    case JsonSerialization::Type::Number:
        pData_.numberValue = value.pData_.numberValue;
        break;

    case JsonSerialization::Type::Bool:
        pData_.boolValue = value.pData_.boolValue;
        break;

    case JsonSerialization::Type::String:
        pData_.pData = new std::string(*(std::string *)value.pData_.pData);
        break;

    case JsonSerialization::Type::Vector:
        pData_.pData = new JsonSerialization::VariantVector(*((VariantVector *)value.pData_.pData));
        break;

    case JsonSerialization::Type::Map:
        pData_.pData = new JsonSerialization::VariantMap(*((VariantMap *)value.pData_.pData));
        break;

    default:
        break;
    }
}

void JsonSerialization::Variant::moveAll(JsonSerialization::Variant &value) noexcept
{
    pData_ = value.pData_;
    type_ = value.type_;
    value.pData_.pData = nullptr;
    value.type_ = JsonSerialization::Type::Empty;
}

std::string JsonSerialization::Variant::_toJson(int &intend) const
{
    switch (type_)
    {
    case JsonSerialization::Type::Null:
        return "null";

    case JsonSerialization::Type::Number:
        return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t)pData_.numberValue) : std::to_string(pData_.numberValue);

    case JsonSerialization::Type::Bool:
        return pData_.boolValue ? "true" : "false";

    case JsonSerialization::Type::String:
        return std::string("\"") + *((std::string *)pData_.pData) + "\"";

    case JsonSerialization::Type::Vector:
    {
        JsonSerialization::VariantVector *pJsonVariantVector = (JsonSerialization::VariantVector *)pData_.pData;
        if (pJsonVariantVector->empty())
        {
            return "[]";
        }
        else
        {
            intend += 4;
            std::string resultStr("[" + endLineStr);
            for (auto &jsonVariant : *pJsonVariantVector)
            {
                if (intend > 0)
                    resultStr += std::string(intend, ' ');

                resultStr += jsonVariant._toJson(intend) + "," + endLineStr;
            }

            intend -= 4;
            resultStr.pop_back();
            resultStr.pop_back();
            resultStr += endLineStr + std::string(intend, ' ') + "]";
            return resultStr;
        }
    }
    break;

    case JsonSerialization::Type::Map:
    {
        JsonSerialization::VariantMap *pJsonVariantMap = (JsonSerialization::VariantMap *)pData_.pData;
        if (pJsonVariantMap->empty())
        {
            return "{}";
        }
        else
        {
            std::string resultStr("{");
            intend += 4;
            for (auto &it : *pJsonVariantMap)
            {
                resultStr += endLineStr;
                if (intend > 0)
                    resultStr += std::string(intend, ' ');

                resultStr += "\"" + it.first + "\": " + it.second._toJson(intend) + ",";
            }

            intend -= 4;
            resultStr.pop_back();
            resultStr += endLineStr + std::string(intend, ' ') + "}";
            return resultStr;
        }
    }
    break;

    default:
        return "";
    }

    return "";
}

JsonSerialization::Variant::Variant()
{
    type_ = Type::Empty;
    pData_.pData = nullptr;
}

JsonSerialization::Variant::Variant(std::nullptr_t)
{
    type_ = Type::Null;
    pData_.pData = nullptr;
}

JsonSerialization::Variant::Variant(int value)
    : Variant((double)value)
{
}

JsonSerialization::Variant::Variant(double value)
{
    type_ = Type::Number;
    pData_.numberValue = value;
}

JsonSerialization::Variant::Variant(bool value)
{
    type_ = Type::Bool;
    pData_.boolValue = value;
}

JsonSerialization::Variant::Variant(const char *value)
    : Variant(std::move(std::string(value)))
{
}

JsonSerialization::Variant::Variant(const std::string &value)
{
    type_ = Type::String;
    pData_.pData = new std::string(value);
}

JsonSerialization::Variant::Variant(std::string &&value) noexcept
{
    type_ = Type::String;
    pData_.pData = new std::string(std::move(value));
}

JsonSerialization::Variant::Variant(const JsonSerialization::VariantVector &value)
{
    type_ = Type::Vector;
    pData_.pData = new VariantVector(value);
}

JsonSerialization::Variant::Variant(JsonSerialization::VariantVector &&value) noexcept
{
    type_ = Type::Vector;
    pData_.pData = new VariantVector(std::move(value));
}

JsonSerialization::Variant::Variant(const JsonSerialization::VariantMap &value)
{
    type_ = Type::Map;
    pData_.pData = new VariantMap(value);
}

JsonSerialization::Variant::Variant(JsonSerialization::VariantMap &&value) noexcept
{
    type_ = Type::Map;
    pData_.pData = new JsonSerialization::VariantMap(std::move(value));
}

JsonSerialization::Variant::Variant(const JsonSerialization::Variant &value)
{
    copyAll(value);
}

JsonSerialization::Variant::Variant(JsonSerialization::Variant &&value) noexcept
{
    moveAll(value);
}

JsonSerialization::Variant& JsonSerialization::Variant::operator=(const JsonSerialization::Variant &value)
{
    copyAll(value);
    return *this;
}

bool JsonSerialization::Variant::operator==(const JsonSerialization::Variant &r) const
{
    if (type_ != r.type_)
        return false;

    switch (type_)
    {
    case JsonSerialization::Type::Empty:
    case JsonSerialization::Type::Null:
    case JsonSerialization::Type::Number:
    case JsonSerialization::Type::Bool:
        return (pData_.pData == r.pData_.pData);

    case JsonSerialization::Type::String:
        return *((const std::string *)pData_.pData) == *((const std::string *)r.pData_.pData);

    case JsonSerialization::Type::Vector:
        return *((const JsonSerialization::VariantVector *)pData_.pData) == *((const JsonSerialization::VariantVector *)r.pData_.pData);

    case JsonSerialization::Type::Map:
        return *((const JsonSerialization::VariantMap *)pData_.pData) == *((const JsonSerialization::VariantMap *)r.pData_.pData);

    default:
        return false;
    }
}

JsonSerialization::Variant &JsonSerialization::Variant::operator=(JsonSerialization::Variant &&value) noexcept
{
    if (this != &value)
    {
        clear();
        moveAll(value);
    }

    return *this;
}

JsonSerialization::Variant::~Variant()
{
    clear();
}

JsonSerialization::Type JsonSerialization::Variant::type() const
{
    return type_;
}

bool JsonSerialization::Variant::isEmpty() const
{
    return (type_ == JsonSerialization::Type::Empty);
}

bool JsonSerialization::Variant::isNull() const
{
    return (type_ == JsonSerialization::Type::Null);
}

int JsonSerialization::Variant::toInt() const
{
    if (type_ == JsonSerialization::Type::Number)
        return (int)pData_.numberValue;

    throw std::runtime_error("Not integer in variant");
}

double JsonSerialization::Variant::toNumber() const
{
    if (type_ == JsonSerialization::Type::Number)
        return pData_.numberValue;

    throw std::runtime_error("Not number in variant");
}

bool JsonSerialization::Variant::toBool() const
{
    if (type_ == JsonSerialization::Type::Bool)
        return pData_.boolValue;

    throw std::runtime_error("Not bool in variant");
}

const std::string& JsonSerialization::Variant::toString() const
{
    if (type_ == JsonSerialization::Type::String)
        return *(std::string *)pData_.pData;

    throw std::runtime_error("Not string in variant");
}

const JsonSerialization::VariantVector& JsonSerialization::Variant::toVector() const
{
    if (type_ == JsonSerialization::Type::Vector)
        return *(JsonSerialization::VariantVector *)pData_.pData;

    throw std::runtime_error("Not vector in variant");
}

const JsonSerialization::VariantMap& JsonSerialization::Variant::toMap() const
{
    if (type_ == JsonSerialization::Type::Map)
        return *(JsonSerialization::VariantMap *)pData_.pData;

    throw std::runtime_error("Not map in variant");
}

std::string JsonSerialization::Variant::toJson() const
{
    switch (type_)
    {
    case JsonSerialization::Type::Null:
        return "null";

    case JsonSerialization::Type::Number:
        return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t)pData_.numberValue) : std::to_string(pData_.numberValue);

    case JsonSerialization::Type::Bool:
        return pData_.boolValue ? "true" : "false";

    case JsonSerialization::Type::String:
        return std::string("\"") + *((std::string *)pData_.pData) + "\"";

    case JsonSerialization::Type::Vector:
    {
        JsonSerialization::VariantVector *pJsonVariantVector = (JsonSerialization::VariantVector*)pData_.pData;
        if (pJsonVariantVector->empty())
        {
            return "[]";
        }
        else
        {
            std::string resultStr("[");
            for (auto &jsonVariant : *pJsonVariantVector)
                resultStr += jsonVariant.toJson() + ",";

            resultStr[resultStr.size() - 1] = ']';
            return resultStr;
        }
    }
    break;

    case JsonSerialization::Type::Map:
    {
        JsonSerialization::VariantMap *pJsonVariantMap = (JsonSerialization::VariantMap*)pData_.pData;
        if (pJsonVariantMap->empty())
        {
            return "{}";
        }
        else
        {
            std::string resultStr("{");
            for (auto &it : *pJsonVariantMap)
                resultStr += "\"" + it.first + "\":" + it.second.toJson() + ",";

            resultStr[resultStr.size() - 1] = '}';
            return resultStr;
        }
    }
    break;

    default:
        return "";
    }

    return "";
}

std::string JsonSerialization::Variant::toJson(bool pretty) const
{
    int intend = 0;
    return pretty ? _toJson(intend) : toJson();
}

bool JsonSerialization::Variant::fromJson(const std::string &jsonStr, JsonSerialization::Variant &jsonVariant, std::string *errorStr /*= nullptr*/)
{
    try
    {
        JsonSerializationInternal::JsonParser::fromJson(jsonStr, jsonVariant);
    }
    catch (const std::exception &e)
    {
        if (errorStr)
            *errorStr = e.what();

        return false;
    }

    return true;
}

bool JsonSerialization::Variant::fromJson(const std::string &jsonStr, const std::string &jsonSchema, JsonSerialization::Variant &jsonVariant, std::string *errorStr /*= nullptr*/)
{
    try
    {
        JsonSerializationInternal::JsonParser::fromJson(jsonStr, jsonSchema, jsonVariant);
    }
    catch (const std::exception &e)
    {
        if (errorStr)
            *errorStr = e.what();

        return false;
    }

    return true;
}

void JsonSerialization::Variant::value(int &val) const
{
    val = toInt();
}
void JsonSerialization::Variant::value(double &val) const
{
    val = toNumber();
}
void JsonSerialization::Variant::value(bool &val) const
{
    val = toBool();
}
void JsonSerialization::Variant::value(std::string &val) const
{
    val = toString();
}
