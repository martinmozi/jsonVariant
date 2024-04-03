#ifndef __JSON_VARIANT_HPP
#define __JSON_VARIANT_HPP

#include <set>
#include <map>
#include <vector>
#include <stdexcept>
#include <string>
#include <regex>
#include <limits>
#include <cstdint>

// Platform-specific newline string
#ifdef _WIN32
const std::string endLineStr = "\r\n";
#else
const std::string endLineStr = "\n";
#endif

namespace JsonSerializationInternal
{
    inline bool isIgnorable(char d) {
        return d == ' ' || d == '\n' || d == '\t' || d == '\r';
    }

    inline bool isInteger(double d)
    {
        return d == (int)d;
    } 
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace JsonSerialization {
    enum class Type : char
    {
        Empty,
        Null,
        Number,
        Bool,
        String,
        Vector,
        Map
    };

    class Variant;
    template <typename T1, typename T2> struct _VariantMap : std::map<T1, T2>
    {
        using std::map<T1, T2>::map; // "inherit" the constructors.
        bool contains(const char* key) const
        {
            return (this->find(key) != this->end());
        }

        bool isNull(const char* key) const
        {
            const auto it = this->find(key);
            if (it == this->end())
                return false;

            return it->second.isNull();
        }

        template<typename T> T value(const char* key, T defaultValue) const
        {
            const auto it = this->find(key);
            if (it != this->end()) {
                T v;
                it->second.value(v);
                return v;
            }

            return defaultValue;
        }

        template<typename T> void value(const char* key, T& val, T defaultValue) const
        {
            val = this->value(key, defaultValue);
        }

        const Variant& operator()(const char* key) const
        {
            return this->at(key);
        }
    };

    typedef _VariantMap<std::string, Variant> VariantMap;
    typedef std::vector<Variant> VariantVector;

    class Variant
    {
    private:
        union PDATA
        {
            bool boolValue;
            double numberValue;
            void* pData;
        };

        PDATA pData_;
        Type type_;

    private:
        void clear()
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
                VariantVector *pJsonVariantVector = (VariantVector *)pData_.pData;
                for (auto& jsonVariant : *pJsonVariantVector)
                    jsonVariant.clear();

                delete pJsonVariantVector;
            }
            break;

            case JsonSerialization::Type::Map:
            {
                VariantMap *pJsonVariantMap = (VariantMap *)pData_.pData;
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

        void copyAll(const Variant &value)
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
                pData_.pData = new VariantVector(*((VariantVector *)value.pData_.pData));
                break;

            case JsonSerialization::Type::Map:
                pData_.pData = new VariantMap(*((VariantMap *)value.pData_.pData));
                break;

            default:
                break;
            }
        }

        void moveAll(Variant &value) noexcept
        {
            pData_ = value.pData_;
            type_ = value.type_;
            value.pData_.pData = nullptr;
            value.type_ = JsonSerialization::Type::Empty;
        }

        std::string toJsonPrivate(int &intend) const
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
                VariantVector *pJsonVariantVector = (VariantVector *)pData_.pData;
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

                        resultStr += jsonVariant.toJsonPrivate(intend) + "," + endLineStr;
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
                VariantMap *pJsonVariantMap = (VariantMap *)pData_.pData;
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

                        resultStr += "\"" + it.first + "\": " + it.second.toJsonPrivate(intend) + ",";
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

        //////////////////////////////////////////////////////////////////////////////
        ////////////////////////// schema validator methods //////////////////////////
        static void schemaValidator_validate(const Variant& schemaVariant, const Variant& jsonVariant)
        {
            if (schemaVariant.type() != JsonSerialization::Type::Map)
                throw std::runtime_error("Bad schema type");
            const auto& schemaVariantMap = schemaVariant.toMap();
            schemaValidator_compare(schemaVariantMap, jsonVariant, schemaVariantMap);
        }

        static const Variant* schemaValidator_valueFromMap(const VariantMap& schemaVariantMap, const char* key, JsonSerialization::Type type, bool required, bool *isRef = nullptr)
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
                        if (it->second.type() != JsonSerialization::Type::String)
                            throw std::runtime_error("Expected string for $ref in schema");
                        if (isRef != nullptr)
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

        static std::vector<std::string> schemaValidator_tokenize(const std::string &str, char delim)
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

        static const VariantMap& schemaValidator_fromRef(const std::string &refPath,  const VariantMap& wholeSchemaVariantMap)
        {
            std::vector<std::string> pathVector = schemaValidator_tokenize(refPath, '/');
            const VariantMap * pVariantMap = &wholeSchemaVariantMap;
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

        static void schemaValidator_compare(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap)
        {
            // todo enum and const - not implemented
            bool isRef = false;
            const std::string & typeStr = schemaValidator_valueFromMap(schemaVariantMap, "type", JsonSerialization::Type::String, true, &isRef)->toString();
            if (isRef)
            {
                schemaValidator_compare(schemaValidator_fromRef(typeStr, wholeSchemaVariantMap), jsonVariant, wholeSchemaVariantMap);
            }
            else
            {
                if (typeStr == "object")
                {
                    schemaValidator_compareMap(schemaVariantMap, jsonVariant, wholeSchemaVariantMap);
                }
                else if (typeStr == "array")
                {
                    schemaValidator_compareVector(schemaVariantMap, jsonVariant, wholeSchemaVariantMap);
                }
                else if (typeStr == "integer")
                {
                    schemaValidator_compareInteger(schemaVariantMap, jsonVariant);
                }
                else if (typeStr == "number")
                {
                    schemaValidator_compareNumber(schemaVariantMap, jsonVariant);
                }
                else if (typeStr == "null")
                {
                    schemaValidator_checkNull(jsonVariant);
                }
                else if (typeStr == "boolean")
                {
                    schemaValidator_checkBoolean(jsonVariant);
                }
                else if (typeStr == "string")
                {
                    schemaValidator_compareString(schemaVariantMap, jsonVariant);
                }
                else
                {
                    throw std::runtime_error("Unsupported type in json schema");
                }
            }
        }

        static void schemaValidator_compareMap(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap)
        {
            std::set<std::string> required;
            const VariantMap &propertiesVariantMap = schemaValidator_valueFromMap(schemaVariantMap, "properties", JsonSerialization::Type::Map, true)->toMap();
            const Variant *pRequiredVariant = schemaValidator_valueFromMap(schemaVariantMap, "required", JsonSerialization::Type::Vector, false);
            if (pRequiredVariant)
            {
                const VariantVector& requiredVariantVector = pRequiredVariant->toVector();
                for (const auto &v : requiredVariantVector)
                    required.insert(v.toString());
            }
            if (jsonVariant.type() != JsonSerialization::Type::Map)
                throw std::runtime_error("Map required");
            const VariantMap &jsonVariantMap = jsonVariant.toMap();
            for (const auto& it : propertiesVariantMap)
            {
                if (it.second.type() != JsonSerialization::Type::Map)
                    throw std::runtime_error(std::string("Missing map for key: ") + it.first);
                auto iter = jsonVariantMap.find(it.first);
                if (iter == jsonVariantMap.end() && required.find(it.first) != required.end())
                    throw std::runtime_error(std::string("Missing key in map: ") + it.first);
                schemaValidator_compare(it.second.toMap(), iter->second, wholeSchemaVariantMap);
            }
            const Variant *pMinProperties = schemaValidator_valueFromMap(schemaVariantMap, "minProperties", JsonSerialization::Type::Number, false);
            if (pMinProperties && pMinProperties->toInt() > (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is smaller as defined in minProperties");
            const Variant *pMaxProperties = schemaValidator_valueFromMap(schemaVariantMap, "maxProperties", JsonSerialization::Type::Number, false);
            if (pMaxProperties && pMaxProperties->toInt() < (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is greater as defined in maxProperties");
            const Variant *pDependentRequired = schemaValidator_valueFromMap(schemaVariantMap, "dependentRequired", JsonSerialization::Type::Map, false);
            if (pDependentRequired)
                throw std::runtime_error("not supported dependentRequired");
        }

        static void schemaValidator_compareVector(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap)
        {
            const auto it = schemaVariantMap.find("items");
            if (it == schemaVariantMap.end())
                throw std::runtime_error("Expected items for schema vector");
            const VariantMap* pSchemaVariantMap = nullptr;
            const VariantVector* pSchemaVariantVector = nullptr;
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
                    schemaValidator_compare(*pSchemaVariantMap, v, wholeSchemaVariantMap);
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
                    schemaValidator_compare(schVariant.toMap(), variantVector[i], wholeSchemaVariantMap);
                }    
            }
            const Variant *pMinItems = schemaValidator_valueFromMap(schemaVariantMap, "minItems", JsonSerialization::Type::Number, false);
            if (pMinItems && pMinItems->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");
            const Variant *pMaxItems = schemaValidator_valueFromMap(schemaVariantMap, "maxItems", JsonSerialization::Type::Number, false);
            if (pMaxItems && pMaxItems->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");
            const Variant *pMinContains = schemaValidator_valueFromMap(schemaVariantMap, "minContains", JsonSerialization::Type::Number, false);
            if (pMinContains && pMinContains->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");
            const Variant *pMaxContains = schemaValidator_valueFromMap(schemaVariantMap, "maxContains", JsonSerialization::Type::Number, false);
            if (pMaxContains && pMaxContains->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");
            const Variant *pUniqueItems = schemaValidator_valueFromMap(schemaVariantMap, "uniqueItems", JsonSerialization::Type::Number, false);
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

        static void schemaValidator_compareString(const VariantMap& schemaVariantMap, const Variant& jsonVariant)
        {
            if (jsonVariant.type() != JsonSerialization::Type::String)
                throw std::runtime_error("Expected string value");
            const std::string& value = jsonVariant.toString();
            const Variant *pMinLength = schemaValidator_valueFromMap(schemaVariantMap, "minLength", JsonSerialization::Type::Number, false);
            if (pMinLength && pMinLength->toInt() > (int)value.size())
                throw std::runtime_error(std::string("Too short string: ") + value);
            const Variant *pMaxLength = schemaValidator_valueFromMap(schemaVariantMap, "maxLength", JsonSerialization::Type::Number, false);
            if (pMaxLength && pMaxLength->toInt() < (int)value.size())
                throw std::runtime_error(std::string("Too long string: ") + value);
            const Variant *pPattern = schemaValidator_valueFromMap(schemaVariantMap, "pattern", JsonSerialization::Type::String, false);
            if (pPattern)
            {
                std::regex expr(pPattern->toString());
                std::smatch sm;
                if (! std::regex_match(value, sm, expr))
                    throw std::runtime_error(std::string("String doesn't match the pattern: ") + pPattern->toString());
            }
            const Variant *pFormat = schemaValidator_valueFromMap(schemaVariantMap, "format", JsonSerialization::Type::String, false);
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

        static void schemaValidator_compareNumber(const VariantMap& schemaVariantMap, const Variant& jsonVariant)
        {
            if (jsonVariant.type() != JsonSerialization::Type::Number)
                throw std::runtime_error("Expected numeric value");
            double value = jsonVariant.toNumber();
            const Variant *pMinimum = schemaValidator_valueFromMap(schemaVariantMap, "minimum", JsonSerialization::Type::Number, false);
            if (pMinimum && pMinimum->toNumber() > value)
                throw std::runtime_error("Numeric value is smaller than minimum");
            const Variant *pMaximum = schemaValidator_valueFromMap(schemaVariantMap, "maximum", JsonSerialization::Type::Number, false);
            if (pMaximum && pMaximum->toNumber() < value)
                throw std::runtime_error("Numeric value is greater than maximum");
            const Variant *pExclusiveMinimum = schemaValidator_valueFromMap(schemaVariantMap, "exclusiveMinimum", JsonSerialization::Type::Number, false);
            if (pExclusiveMinimum && pExclusiveMinimum->toNumber() >= value)
                throw std::runtime_error("Numeric value is smaller than exclusive minimum");
            const Variant *pExclusiveMaximum = schemaValidator_valueFromMap(schemaVariantMap, "exclusiveMaximum", JsonSerialization::Type::Number, false);
            if (pExclusiveMaximum && pExclusiveMaximum->toNumber() <= value)
                throw std::runtime_error("Numeric value is greater than exclusive maximum");
            const Variant *pMultipleOf = schemaValidator_valueFromMap(schemaVariantMap, "multipleOf", JsonSerialization::Type::Number, false);
            if (pMultipleOf)
            {
                double multipleOf = pMultipleOf->toNumber();
                if (! JsonSerializationInternal::isInteger(multipleOf) || multipleOf <= 0)
                    throw std::runtime_error("Multiple of has to be an positive number");
                if (! JsonSerializationInternal::isInteger(value / multipleOf))
                    throw std::runtime_error("Multiple of division must be an integer");
            }
        }

        static void schemaValidator_compareInteger(const VariantMap& schemaVariantMap, const Variant& jsonVariant)
        {
            schemaValidator_compareNumber(schemaVariantMap, jsonVariant);
            if (! JsonSerializationInternal::isInteger(jsonVariant.toNumber()))
                throw std::runtime_error("Expected integer value");
        }

        static void schemaValidator_checkBoolean(const Variant& jsonVariant) 
        {
            if (jsonVariant.type() != JsonSerialization::Type::Bool)
                throw std::runtime_error("Expected boolean value");
        }

        static void schemaValidator_checkNull(const Variant& jsonVariant) 
        {
            if (jsonVariant.type() != JsonSerialization::Type::Null)
                throw std::runtime_error("Expected null value");
        }

        //////////////////////////////////////////////////////////////////////////////
        ///////////////////////// parser methods /////////////////////////////////////

        static void jsonParser_fromJson(const std::string& jsonStr, Variant& jsonVariant) {
            std::string _jsonStr = jsonParser_trim(jsonStr);
            const char *pData = _jsonStr.c_str();
            size_t len = _jsonStr.size();
            if (len < 2)
                throw std::runtime_error("No short json");

            jsonVariant = jsonParser_parseObject(pData);
        }

        static void jsonParser_fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant) 
        {
            Variant schemaVariant, schemaVariantInternal;
            jsonParser_fromJson(jsonSchema, schemaVariant);
            jsonParser_fromJson(jsonStr, jsonVariant);
            schemaValidator_validate(schemaVariant, jsonVariant);
        }

    private:
        static VariantVector jsonParser_parseArray(const char *& pData) {
            ++pData;
            VariantVector variantVector;
            while (pData != NULL)
            {
                variantVector.emplace_back(jsonParser_parseValue(pData));
                if (*pData == ']')
                {
                    ++pData;
                    return variantVector;
                }

                ++pData;
            }

            throw std::runtime_error("Unfinished vector");
        }

        static VariantMap jsonParser_parseMap(const char *& pData) {
            ++pData;
            VariantMap variantMap;
            while (pData != NULL)
            {
                bool isEmpty = false;
                if (*pData == '\"')
                {
                    std::string key = jsonParser_parseKey(pData);
                    jsonParser_gotoValue(pData);
                    variantMap.insert(std::make_pair(key, jsonParser_parseValue(pData)));
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

        static Variant jsonParser_parseObject(const char *& pData) 
        {
            while (pData != NULL)
            {
                char c = *pData;
                if (c == '{')
                    return Variant(jsonParser_parseMap(pData));
                else if (c == '[')
                    return Variant(jsonParser_parseArray(pData));
                else
                    throw std::runtime_error("Invalid json - first char");

                ++pData;
            }

            throw std::runtime_error("Missing end of the object");
        }

        static std::string jsonParser_parseKey(const char *& pData) {
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

        static Variant jsonParser_parseValue(const char *& pData)
        {
            Variant variant;
            char c = *pData;
            if (c == '{')
                variant = Variant(jsonParser_parseMap(pData));
            else if (c == '[')
                variant = Variant(jsonParser_parseArray(pData));
            else if (c == '\"')
                variant = Variant(jsonParser_parseString(pData));
            else if (c == 't' || c == 'f')
                variant = Variant(jsonParser_parseBoolean(pData));
            else if (c == 'n')
                variant = Variant(jsonParser_parseNull(pData));
            else if (isdigit(c) || c == '.' || c == '-')
            {
                double d = jsonParser_parseNumber(pData);
                variant = (d == (int)d) ? Variant((int)d) : Variant(d);
            }
            else
                throw std::runtime_error("Unknown character when parsing value");

            c = *pData;
            if (c == ',' || c == '}' || c == ']')
                return variant;

            throw std::runtime_error("Missing delimiter");
        }

        static std::string jsonParser_parseString(const char *& pData)
        {
            std::string potentionalString;
            while (pData != NULL)
            {
                ++pData;
                char c = *pData;
                if (c == '\"')
                {
                    ++pData;
                    return potentionalString;
                }

                potentionalString.push_back(c);
            }

            throw std::runtime_error("Not finished string value reading");
        }

        static bool jsonParser_parseBoolean(const char *& pData)
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

        static double jsonParser_parseNumber(const char *& pData)
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

        static Variant jsonParser_parseNull(const char *& pData)
        {
            static const char *nullStr = "null";
            for (int i = 0; i < 4; i ++)
            {
                if (nullStr[i] != *pData)
                    throw std::runtime_error("Unable to parse null value");

                pData ++;
            }

            return Variant(nullptr);
        }

        static void jsonParser_gotoValue(const char *& pData)
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

        static std::string jsonParser_trim(const std::string& jsonStr)
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
                if (c== '\"')
                    record = !record;

                if (!JsonSerializationInternal::isIgnorable(c) || record)
                {
                    p[j] = c;
                    j++;
                }
            }

            return outStr;
        }

    public:
        Variant()
        {
            type_ = Type::Empty;
            pData_.pData = nullptr;
        }

        Variant(std::nullptr_t)
        {
            type_ = Type::Null;
            pData_.pData = nullptr;
        }

        Variant(int value)
            : Variant((double)value)
        {
        }

        Variant(double value)
        {
            type_ = Type::Number;
            pData_.numberValue = value;
        }

        Variant(bool value)
        {
            type_ = Type::Bool;
            pData_.boolValue = value;
        }

        Variant(const char *value)
            : Variant(std::move(std::string(value)))
        {
        }

        Variant(const std::string &value)
        {
            type_ = Type::String;
            pData_.pData = new std::string(value);
        }

        Variant(std::string &&value) noexcept
        {
            type_ = Type::String;
            pData_.pData = new std::string(std::move(value));
        }

        Variant(const VariantVector &value)
        {
            type_ = Type::Vector;
            pData_.pData = new VariantVector(value);
        }

        Variant(VariantVector &&value) noexcept
        {
            type_ = Type::Vector;
            pData_.pData = new VariantVector(std::move(value));
        }

        Variant(const VariantMap &value)
        {
            type_ = Type::Map;
            pData_.pData = new VariantMap(value);
        }

        Variant(VariantMap &&value) noexcept
        {
            type_ = Type::Map;
            pData_.pData = new VariantMap(std::move(value));
        }

        Variant(const Variant &value)
        {
            copyAll(value);
        }

        Variant(Variant &&value) noexcept
        {
            moveAll(value);
        }

        template<typename T> Variant(const std::vector<T>& value)
        {
            VariantVector *pVector = new VariantVector;
            for (const auto& v : value)
                pVector->push_back(v);

            type_ = Type::Vector;
            pData_.pData = pVector;
        }

        Variant& operator=(const Variant &value)
        {
            copyAll(value);
            return *this;
        }

        bool operator==(const Variant &r) const
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
                return *((const VariantVector *)pData_.pData) == *((const VariantVector *)r.pData_.pData);

            case JsonSerialization::Type::Map:
                return *((const VariantMap *)pData_.pData) == *((const VariantMap *)r.pData_.pData);

            default:
                return false;
            }
        }

        Variant &operator=(Variant &&value) noexcept
        {
            if (this != &value)
            {
                clear();
                moveAll(value);
            }

            return *this;
        }

        ~Variant()
        {
            clear();
        }

        JsonSerialization::Type type() const
        {
            return type_;
        }

        bool isEmpty() const
        {
            return (type_ == JsonSerialization::Type::Empty);
        }

        bool isNull() const
        {
            return (type_ == JsonSerialization::Type::Null);
        }

        int toInt() const
        {
            if (type_ == JsonSerialization::Type::Number)
                return (int)pData_.numberValue;

            throw std::runtime_error("Not integer in variant");
        }

        double toNumber() const
        {
            if (type_ == JsonSerialization::Type::Number)
                return pData_.numberValue;

            throw std::runtime_error("Not number in variant");
        }

        bool toBool() const
        {
            if (type_ == JsonSerialization::Type::Bool)
                return pData_.boolValue;

            throw std::runtime_error("Not bool in variant");
        }

        const std::string& toString() const
        {
            if (type_ == JsonSerialization::Type::String)
                return *(std::string *)pData_.pData;

            throw std::runtime_error("Not string in variant");
        }

        const VariantVector& toVector() const
        {
            if (type_ == JsonSerialization::Type::Vector)
                return *(VariantVector *)pData_.pData;

            throw std::runtime_error("Not vector in variant");
        }

        const VariantMap& toMap() const
        {
            if (type_ == JsonSerialization::Type::Map)
                return *(VariantMap *)pData_.pData;

            throw std::runtime_error("Not map in variant");
        }

        void value(int &value) const
        {
            value = toInt();
        }

        void value(double &value) const
        {
            value = toNumber();
        }

        void value(bool &value) const
        {
            value = toBool();
        }

        void value(std::string &value) const
        {
            value = toString();
        }

        template<typename T> void valueVector(std::vector<T> &value) const
        {
            const VariantVector &variantVector = toVector();
            value.clear();
            for (const auto &v : variantVector)
            {
                T t;
                v.value(t);
                value.push_back(t);
            }
        }

        std::string toJson() const
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
                VariantVector *pJsonVariantVector = (VariantVector*)pData_.pData;
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
                VariantMap *pJsonVariantMap = (VariantMap*)pData_.pData;
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

        std::string toJson(bool pretty) const
        {
            int intend = 0;
            return pretty ? toJsonPrivate(intend) : toJson();
        }

        static bool fromJson(const std::string &jsonStr, Variant &jsonVariant, std::string *errorStr /*= nullptr*/)
        {
            try
            {
                jsonParser_fromJson(jsonStr, jsonVariant);
            }
            catch (const std::exception &e)
            {
                if (errorStr)
                    *errorStr = e.what();

                return false;
            }

            return true;
        }

        static bool fromJson(const std::string &jsonStr, const std::string &jsonSchema, Variant &jsonVariant, std::string *errorStr /*= nullptr*/)
        {
            try
            {
                jsonParser_fromJson(jsonStr, jsonSchema, jsonVariant);
            }
            catch (const std::exception &e)
            {
                if (errorStr)
                    *errorStr = e.what();

                return false;
            }

            return true;
        } 
    };
}

#endif

    