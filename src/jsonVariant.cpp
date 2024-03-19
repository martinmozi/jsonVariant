#include "jsonVariant.h"
#include "internal/jsonParser.h"
#include <cstdint>
#include <limits>
#include <ostream>

// Platform-specific newline string
#ifdef _WIN32
    const std::string endLineStr = "\r\n";
#else
    const std::string endLineStr = "\n";
#endif


void JsonSerialization::Variant::clear()
{
    switch (type_)
    {
    case JsonSerialization::Variant::Type::Number:
    case JsonSerialization::Variant::Type::Bool:
        break;

    case JsonSerialization::Variant::Type::String:
    {
        std::string *pString = (std::string *)pData_.pData;
        delete pString;
    }
    break;

    case JsonSerialization::Variant::Type::Vector:
    {
        JsonSerialization::VariantVector *pJsonVariantVector = (JsonSerialization::VariantVector *)pData_.pData;
        for (auto& jsonVariant : *pJsonVariantVector)
            jsonVariant.clear();

        delete pJsonVariantVector;
    }
    break;

    case JsonSerialization::Variant::Type::Map:
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
    type_ = JsonSerialization::Variant::Type::Empty;
}

void JsonSerialization::Variant::copyAll(const JsonSerialization::Variant &value)
{
    type_ = value.type_;
    switch (type_)
    {
    case JsonSerialization::Variant::Type::Null:
        pData_.pData = nullptr;
        break;

    case JsonSerialization::Variant::Type::Number:
        pData_.numberValue = value.pData_.numberValue;
        break;

    case JsonSerialization::Variant::Type::Bool:
        pData_.boolValue = value.pData_.boolValue;
        break;

    case JsonSerialization::Variant::Type::String:
        pData_.pData = new std::string(*(std::string *)value.pData_.pData);
        break;

    case JsonSerialization::Variant::Type::Vector:
        pData_.pData = new JsonSerialization::VariantVector(*((VariantVector *)value.pData_.pData));
        break;

    case JsonSerialization::Variant::Type::Map:
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
    value.type_ = JsonSerialization::Variant::Type::Empty;
}

std::string JsonSerialization::Variant::_toJson(int &intend) const
{
    switch (type_)
    {
    case JsonSerialization::Variant::Type::Null:
        return "null";

    case JsonSerialization::Variant::Type::Number:
        return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t)pData_.numberValue) : std::to_string(pData_.numberValue);

    case JsonSerialization::Variant::Type::Bool:
        return pData_.boolValue ? "true" : "false";

    case JsonSerialization::Variant::Type::String:
        return std::string("\"") + *((std::string *)pData_.pData) + "\"";

    case JsonSerialization::Variant::Type::Vector:
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

    case JsonSerialization::Variant::Type::Map:
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
    case JsonSerialization::Variant::Type::Empty:
    case JsonSerialization::Variant::Type::Null:
    case JsonSerialization::Variant::Type::Number:
    case JsonSerialization::Variant::Type::Bool:
        return (pData_.pData == r.pData_.pData);

    case JsonSerialization::Variant::Type::String:
        return *((const std::string *)pData_.pData) == *((const std::string *)r.pData_.pData);

    case JsonSerialization::Variant::Type::Vector:
        return *((const JsonSerialization::VariantVector *)pData_.pData) == *((const JsonSerialization::VariantVector *)r.pData_.pData);

    case JsonSerialization::Variant::Type::Map:
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

JsonSerialization::Variant::Type JsonSerialization::Variant::type() const
{
    return type_;
}

bool JsonSerialization::Variant::isEmpty() const
{
    return (type_ == JsonSerialization::Variant::Type::Empty);
}

bool JsonSerialization::Variant::isNull() const
{
    return (type_ == JsonSerialization::Variant::Type::Null);
}

int JsonSerialization::Variant::toInt() const
{
    if (type_ == JsonSerialization::Variant::Type::Number)
        return (int)pData_.numberValue;

    throw std::runtime_error("Not integer in variant");
}

double JsonSerialization::Variant::toNumber() const
{
    if (type_ == JsonSerialization::Variant::Type::Number)
        return pData_.numberValue;

    throw std::runtime_error("Not number in variant");
}

bool JsonSerialization::Variant::toBool() const
{
    if (type_ == JsonSerialization::Variant::Type::Bool)
        return pData_.boolValue;

    throw std::runtime_error("Not bool in variant");
}

const std::string& JsonSerialization::Variant::toString() const
{
    if (type_ == JsonSerialization::Variant::Type::String)
        return *(std::string *)pData_.pData;

    throw std::runtime_error("Not string in variant");
}

const JsonSerialization::VariantVector& JsonSerialization::Variant::toVector() const
{
    if (type_ == JsonSerialization::Variant::Type::Vector)
        return *(JsonSerialization::VariantVector *)pData_.pData;

    throw std::runtime_error("Not vector in variant");
}

const JsonSerialization::VariantMap& JsonSerialization::Variant::toMap() const
{
    if (type_ == JsonSerialization::Variant::Type::Map)
        return *(JsonSerialization::VariantMap *)pData_.pData;

    throw std::runtime_error("Not map in variant");
}

std::string JsonSerialization::Variant::toJson() const
{
    switch (type_)
    {
    case JsonSerialization::Variant::Type::Null:
        return "null";

    case JsonSerialization::Variant::Type::Number:
        return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t)pData_.numberValue) : std::to_string(pData_.numberValue);

    case JsonSerialization::Variant::Type::Bool:
        return pData_.boolValue ? "true" : "false";

    case JsonSerialization::Variant::Type::String:
        return std::string("\"") + *((std::string *)pData_.pData) + "\"";

    case JsonSerialization::Variant::Type::Vector:
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

    case JsonSerialization::Variant::Type::Map:
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
        JsonSerializationInternal::JsonParser().fromJson(jsonStr, jsonVariant);
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
        JsonSerializationInternal::JsonParser().fromJson(jsonStr, jsonSchema, jsonVariant);
    }
    catch (const std::exception &e)
    {
        if (errorStr)
            *errorStr = e.what();

        return false;
    }

    return true;
}