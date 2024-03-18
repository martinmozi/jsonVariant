#include "internal/jsonParser.h"
#include "internal/schemaValidator.h"
#include <stdexcept>

void JsonSerializationInternal::JsonParser::fromJson(const std::string& jsonStr, const std::string& jsonSchema, JsonSerialization::Variant& jsonVariant) const {
     JsonSerialization::Variant schemaVariant, schemaVariantInternal;
     fromJson(jsonSchema, schemaVariant);
     fromJson(jsonStr, jsonVariant);
     JsonSerializationInternal::SchemaValidator().validate(schemaVariant, jsonVariant);
}

void JsonSerializationInternal::JsonParser::fromJson(const std::string& jsonStr, JsonSerialization::Variant& jsonVariant) const {
    std::string _jsonStr = trim(jsonStr);
    const char *pData = _jsonStr.c_str();
    size_t len = _jsonStr.size();
    if (len < 2)
        throw std::runtime_error("No short json");

    jsonVariant = parseObject(pData);
}

JsonSerialization::VariantVector JsonSerializationInternal::JsonParser::parseArray(const char *& pData) const {
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

JsonSerialization::VariantMap JsonSerializationInternal::JsonParser::parseMap(const char *& pData) const {
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

JsonSerialization::Variant JsonSerializationInternal::JsonParser::parseObject(const char *& pData) const 
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

std::string JsonSerializationInternal::JsonParser::parseKey(const char *& pData) const {
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

JsonSerialization::Variant JsonSerializationInternal::JsonParser::parseValue(const char *& pData) const {
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

std::string JsonSerialization::JsonParser::parseString(const char *& pData) const {
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

bool JsonSerialization::JsonParser::parseBoolean(const char *& pData) const {
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

double JsonSerialization::JsonParser::parseNumber(const char *& pData) const {
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

JsonSerialization::Variant JsonSerialization::JsonParser::parseNull(const char *& pData) const {
    static const char *nullStr = "null";
    for (int i = 0; i < 4; i ++)
    {
        if (nullStr[i] != *pData)
            throw std::runtime_error("Unable to parse null value");

        pData ++;
    }

    return JsonSerialization::Variant(nullptr);
}

void JsonSerialization::JsonParser::gotoValue(const char *& pData) const {
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

std::string JsonSerialization::JsonParser::trim(const std::string& jsonStr) const {
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

        if (!isIgnorable(c) || record)
        {
            p[j] = c;
            j++;
        }
    }

    return outStr;
}

bool JsonSerialization::JsonParser::isIgnorable(char d) const {
    return (d == ' ' || d == '\n' || d == '\t' || d == '\r');
}
