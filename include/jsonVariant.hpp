#ifndef __JSON_VARIANT_HPP
#define __JSON_VARIANT_HPP

#include <assert.h>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <rapidjson/document.h>
#include <rapidjson/schema.h>


namespace JsonSerialization
{
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
            if (it != this->end())
                return it->second.template value<T>();

            return defaultValue;
        }

        template<typename T> void value(const char* key, T & val, T defaultValue) const
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

    namespace JsonSerializationInternal
    {
        class JsonParser
        {
        public:
            JsonParser() = default;
            void fromJson(const std::string& jsonStr, const std::string& jsonSchema, JsonSerialization::Variant& jsonVariant) const;
            void fromJson(const std::string& jsonStr, JsonSerialization::Variant& jsonVariant) const;

        private:
            void interpretSchema(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& internalSchemaVariant) const;
            void compareVariantsAccordingSchema(const JsonSerialization::Variant& jsonVariant, const JsonSerialization::Variant& schemaVariant) const;
            JsonSerialization::VariantVector parseArray(const char *& pData, size_t &len) const;
            JsonSerialization::VariantMap parseMap(const char *& pData, size_t &len) const;
            JsonSerialization::Variant parseObject(const char *& pData, size_t &len) const;
            std::string parseKey(const char *& pData, size_t &len) const;
            JsonSerialization::Variant parseValue(const char *& pData, size_t &len) const;
            std::string parseString(const char *& pData, size_t &len) const;
            bool parseBoolean(const char *& pData, size_t &len) const;
            double parseNumber(const char *& pData, size_t &len) const;
            JsonSerialization::Variant parseNull(const char *& pData, size_t &len) const;
            void gotoValue(const char *& pData, size_t &len) const;
            void skipComma(const char *& pData, size_t &len) const;
            bool isDelimiter(char d) const;
            bool isIgnorable(char d) const;
            bool isValidKeyCharacter(char d) const;

        private:
            static std::set<char> delimiters_;
            static std::set<char> ignorable_;
        };
    }

    class Variant
    {
    public:
        enum class Type : char
        {
            Empty,
            Null,
            Int,
            Double,
            Bool,
            String,
            Vector,
            Map
        };

    private:
        typedef union 
        {
            bool boolValue;
            int64_t intValue;
            double doubleValue;
            void* pData;
        } PDATA;

        PDATA pData_;
        Type type_;

    private:
        void clear()
        {
            switch (type_)
            {
            case Type::Int:
            case Type::Double:
            case Type::Bool:
                break;

            case Type::String:
            {
                std::string* pString = (std::string*)pData_.pData;
                delete pString;
            }
            break;

            case Type::Vector:
            {
                VariantVector* pJsonVariantVector = (VariantVector*)pData_.pData;
                for (Variant& jsonVariant : *pJsonVariantVector)
                    jsonVariant.clear();

                delete pJsonVariantVector;
            }
            break;

            case Type::Map:
            {
                VariantMap* pJsonVariantMap = (VariantMap*)pData_.pData;
                for (auto& it : *pJsonVariantMap)
                    it.second.clear();

                delete pJsonVariantMap;
            }
            break;

            default:
                break;
            }

            pData_.pData = nullptr;
            type_ = Type::Empty;
        }

        void copyAll(const Variant& value)
        {
            type_ = value.type_;
            switch (type_)
            {
            case Type::Null:
                pData_.pData = nullptr;
                break;

            case Type::Int:
                pData_.intValue = value.pData_.intValue;
                break;

            case Type::Double:
                pData_.doubleValue = value.pData_.doubleValue;
                break;

            case Type::Bool:
                pData_.boolValue = value.pData_.boolValue;
                break;

            case Type::String:
                pData_.pData = new std::string(*(std::string*)value.pData_.pData);
                break;

            case Type::Vector:
                pData_.pData = new VariantVector(*((VariantVector*)value.pData_.pData));
                break;

            case Type::Map:
                pData_.pData = new VariantMap(*((VariantMap*)value.pData_.pData));	
                break;

            default:
                break;
            }
        }

        void moveAll(Variant& value)
        {
            pData_ = value.pData_;
            type_ = value.type_;
            value.pData_.pData = nullptr;
            value.type_ = Type::Empty;
        }
/*
        static Variant _fromJson(const rapidjson::Value& value)
        {
            if (value.IsNull())
            {
                return Variant(nullptr);
            }
            else if (value.IsInt64())
            {
                return Variant(value.GetInt64());
            }
            else if (value.IsDouble())
            {
                return Variant(value.GetDouble());
            }
            else if (value.IsBool())
            {
                return Variant(value.GetBool());
            }
            else if (value.IsString())
            {
                return Variant(value.GetString());
            }
            else if (value.IsArray())
            {
                VariantVector variantVector;
                if (!value.Empty())
                {
                    for (auto it = value.Begin(); it != value.End(); it++)
                        variantVector.push_back(_fromJson(*it));
                }

                return Variant(variantVector);
            }
            else if (value.IsObject())
            {
                VariantMap variantMap;
                for (auto it = value.MemberBegin(); it != value.MemberEnd(); it++)
                    variantMap.insert(std::make_pair(it->name.GetString(), _fromJson(it->value)));

                return Variant(variantMap);
            }

            return Variant();
        }
*/
        void initVector(const VariantVector& value)
        {
            type_ = Type::Vector;
            pData_.pData = new VariantVector(value);
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
        :   Variant((int64_t)value) 
        {
        }

        Variant(int64_t value)
        {
            type_ = Type::Int;
            pData_.intValue = value;
        }

        Variant(double value)
        {
            type_ = Type::Double;
            pData_.doubleValue = value;
        }

        Variant(bool value)
        {
            type_ = Type::Bool;
            pData_.boolValue = value;
        }

        Variant(const char* value) 
        :   Variant(std::string(value)) 
        {
        }

        Variant(const std::string& value)
        {
            type_ = Type::String;
            pData_.pData = new std::string(value);
        }

        template<typename T> Variant(const std::vector<T>& value)
        {
            VariantVector variantVector;
            for (const auto& v : value)
                variantVector.push_back(v);

            initVector(variantVector);
        }

        template<typename T> Variant(std::vector<T> && value)
        {
            VariantVector variantVector;
            for (const auto& v : value)
                variantVector.push_back(v);

            initVector(variantVector);
        }

        Variant(const VariantVector& value)
        {
            initVector(value);
        }

        Variant(const VariantMap& value)
        {
            type_ = Type::Map;
            pData_.pData = new VariantMap(value);
        }

        Variant(const Variant& value) 
        { 
            copyAll(value);
        }

        Variant(Variant&& value) 
        { 
            moveAll(value);
        }

        Variant& operator=(const Variant& value)
        {
            copyAll(value);
            return *this;
        }

        Variant& operator=(Variant&& value)
        {
            clear();
            moveAll(value);
            return *this;
        }

        ~Variant() 
        { 
            clear(); 
        }

        Type type() const 
        { 
            return type_; 
        }
   
        bool isEmpty() const 
        { 
            return (type_ == Type::Empty); 
        }

        bool isNull() const 
        { 
            return (type_ == Type::Null); 
        }

        int64_t toInt64() const
        {
            if (type_ == Type::Int)
                return pData_.intValue;

            assert(0);
            return 0;
        }

        double toDouble() const
        {
            switch (type_)
            {
            case Type::Double:
                return pData_.doubleValue;
            case Type::Int:
                return (double)pData_.intValue;
            default:
                break;
            }

            assert(0);
            return 0.0;
        }

        bool toBool() const
        {
            if (type_ == Type::Bool)
                return pData_.boolValue;

            assert(0);
            return false;
        }

        const std::string& toString() const
        {
            if (type_ == Type::String)
                return *(std::string*)pData_.pData;

            assert(0);
            static std::string vs;
            return vs;
        }

        const VariantVector& toVector() const
        {
            if (type_ == Type::Vector)
                return *(VariantVector*)pData_.pData;

            assert(0);
            static VariantVector vv;
            return vv;
        }

        const VariantMap& toMap() const
        {
            if (type_ == Type::Map)
                return *(VariantMap*)pData_.pData;

            assert(0);
            static VariantMap vm;
            return vm;
        }

        template<typename T> void value(T &) const 
        {
            static_assert(sizeof(T) == -1, "Only specialized versions of this method are allowed!");
        }

        template<typename T> T value() const 
        { 
            static_assert(sizeof(T) == -1, "Only specialized versions of this method are allowed!");
        }

        template<typename T> const T & valueByRef() const 
        { 
            static_assert(sizeof(T) == -1, "Only specialized versions of this method are allowed!");
        }

        template<typename T> std::vector<T> valueVector() const
        {
            std::vector<T> outVector;
            const VariantVector& variantVector = toVector();
            for (const auto& v : variantVector)
                outVector.push_back(std::move(v.value<T>()));

            return outVector;
        }

        template<typename T> void valueVector(std::vector<T> & val) const
        {
            val = valueVector<T>();
        }

        std::string toJson() const
        {
            switch (type_)
            {
            case Type::Null:
                return "null";

            case Type::Int:
                return std::to_string(pData_.intValue);

            case Type::Double:
                return std::to_string(pData_.doubleValue);

            case Type::Bool:
                return pData_.boolValue ? "true" : "false";

            case Type::String:
                return std::string("\"") + *((std::string*)pData_.pData) + "\"";

            case Type::Vector:
            {
                std::string resultStr("[");
                VariantVector* pJsonVariantVector = (VariantVector*)pData_.pData;
                for (Variant& jsonVariant : *pJsonVariantVector)
                {
                    resultStr += jsonVariant.toJson();
                    resultStr += ",";
                }

                if (!pJsonVariantVector->empty())
                    resultStr.pop_back();

                resultStr += "]";
                return resultStr;
            }
            break;

            case Type::Map:
            {
                VariantMap* pJsonVariantMap = (VariantMap*)pData_.pData;
                if (!pJsonVariantMap->empty())
                {
                    std::string resultStr("{");
                    for (auto& it : *pJsonVariantMap)
                        resultStr += "\"" + it.first + "\":" + it.second.toJson() + ",";

                    resultStr.pop_back();
                    resultStr.push_back('}');
                    return resultStr;
                }
            }
            break;

            default:
                return "";
            }

            return "";
        }

        static bool fromJson(const std::string& jsonStr, Variant& jsonVariant, std::string* errorStr = nullptr)
        {
            /*
            rapidjson::Document document;
            auto& doc = document.Parse(jsonStr.c_str());
            if (doc.HasParseError())
            {
                if (errorStr)
                    *errorStr = std::string("Parse error with code: ") + std::to_string(doc.GetParseError()) + " and offset: " + std::to_string(doc.GetErrorOffset());

                return false;
            }

            jsonVariant = _fromJson(doc);
            */
            try
            {
                JsonSerializationInternal::JsonParser().fromJson(jsonStr, jsonVariant);
            }
            catch(const std::exception& e)
            {
                if (errorStr)
                    *errorStr = e.what();
                
                return false;
            }

            return true;
        }

        static bool fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant, std::string* errorStr = nullptr)
        {
            /*
            rapidjson::Document document;
            auto& doc = document.Parse(jsonStr.c_str());
            if (doc.HasParseError())
            {
                if (errorStr)
                    *errorStr = std::string("Parse error with code: ") + std::to_string(doc.GetParseError()) + " and offset: " + std::to_string(doc.GetErrorOffset());

                return false;
            }

            rapidjson::Document documentSchema;
            if (documentSchema.Parse(jsonSchema.c_str()).HasParseError())
            {
                if (errorStr)
                    *errorStr = "Invalid json schema";

                return false;
            }

            rapidjson::SchemaDocument schemaDocument(documentSchema);
            rapidjson::SchemaValidator validator(schemaDocument);

            if (!document.Accept(validator))
            {
                if (errorStr)
                    *errorStr = "Not valid json according schema";

                return false;
            }

            jsonVariant = _fromJson(doc);
            */

            try
            {
                JsonSerializationInternal::JsonParser().fromJson(jsonStr, jsonSchema, jsonVariant);
            }
            catch(const std::exception& e)
            {
                if (errorStr)
                    *errorStr = e.what();
                
                return false;
            }

            return true;
        }
    };

    template<> void Variant::value<std::string>(std::string& t) const { t = toString(); }
    template<> std::string Variant::value<std::string>() const { return toString(); }
    template<> const std::string& Variant::valueByRef<std::string>() const { return toString(); }
    template<> void Variant::value<bool>(bool& t) const { t = toBool(); }
    template<> bool Variant::value<bool>() const { return toBool(); }
    template<> void Variant::value<double>(double& t) const { t = toDouble(); }
    template<> double Variant::value<double>() const { return toDouble(); }
    template<> void Variant::value<int64_t>(int64_t& t) const { t = toInt64(); }
    template<> int64_t Variant::value<int64_t>() const { return toInt64(); }
    template<> void Variant::value<int>(int& t) const { t = (int)toInt64(); }
    template<> int Variant::value<int>() const { return (int)toInt64(); }
    template<> void Variant::value<VariantVector>(VariantVector& t) const { t = toVector(); }
    template<> const VariantVector & Variant::valueByRef<VariantVector>() const { return toVector(); }
    template<> void Variant::value<VariantMap>(VariantMap& t) const { t = toMap(); }
    template<> const VariantMap & Variant::valueByRef<VariantMap>() const { return toMap(); }
}

// #################################################################################################################
// #################################################################################################################

namespace JsonSerialization::JsonSerializationInternal
{
    std::set<char> JsonParser::delimiters_{'{', '}', '[', ']', ':', ',', '\"'};
    std::set<char> JsonParser::ignorable_{' ', '\t', '\n', '\r'};

    bool JsonParser::isDelimiter(char d) const
    {
        return delimiters_.find(d) != delimiters_.end();
    }

    bool JsonParser::isIgnorable(char d) const
    {
        return ignorable_.find(d) != ignorable_.end();
    }

    bool JsonParser::isValidKeyCharacter(char d) const
    {
        // todo actually allow all
        return true;
    }

    void JsonParser::gotoValue(const char *& pData, size_t &len) const
    {
        bool foundDelimiter = false;
        while (len > 1)
        {
            ++pData;
            --len;
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == ':')
                {
                    if (foundDelimiter)
                        throw std::runtime_error("Several delimiters ':'");

                    foundDelimiter = true;
                }
                else
                {
                    if (!foundDelimiter)
                        throw std::runtime_error("Expected ':'");

                    return;
                }
            }
        }

        throw std::runtime_error("Expected value delimiter");
    }

    std::string JsonParser::parseKey(const char *& pData, size_t &len) const
    {
        std::string key;
        while (len > 1)
        {
            ++pData;
            --len;
            char c = *pData;
            if (!isValidKeyCharacter(c))
                throw std::runtime_error("Invalid key character");
            
            if (c == '\"') 
                return key;                

            key += c;
        }

        throw std::runtime_error("Missing end of the key");
    }

    std::string JsonParser::parseString(const char *& pData, size_t &len) const
    {
        std::string potentionalString;
        while (len > 1)
        {
            ++pData;
            --len;
            char c = *pData;
            if (c == '\"')
            {
                ++pData;
                --len;
                return potentionalString;
            }

            potentionalString += c;
        }

        throw std::runtime_error("Not finished string value reading");
    }

    bool JsonParser::parseBoolean(const char *& pData, size_t &len) const
    {
        if (len > 4)
        {
            if (strncmp(pData, "true", 4) == 0)
            {
                pData += 4;
                len -= 4;
                return true;
            }
            else if (strncmp(pData, "false", 5) == 0)
            {
                pData += 5;
                len -= 5;
                return false;
            }
        }

        throw std::runtime_error("Unable to parse boolean value");
    }

    double JsonParser::parseNumber(const char *& pData, size_t &len) const
    {
        std::string potentionalNumber;
        while (len > 1)
        {
            char c = *pData;
            if (isdigit(c) || c == '.')
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
                    throw std::runtime_error("Inval;id argument when number converting");
                }
                catch (const std::out_of_range&) {
                    throw std::runtime_error("Out of range value when number converting");
                }
            }

            ++pData;
            --len;
        }

        throw std::runtime_error("Not finished number value reading");
    }

    JsonSerialization::Variant JsonParser::parseNull(const char *& pData, size_t &len) const
    {
        if (len > 4 && strncmp(pData, "null", 4) == 0)
        {
            pData += 4;
            len -= 4;
            return JsonSerialization::Variant(nullptr);
        }

        throw std::runtime_error("Unable to parse null value");
    }

    JsonSerialization::Variant JsonParser::parseValue(const char *& pData, size_t &len) const
    {
        while (len > 1)
        {
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == '{')
                    return JsonSerialization::Variant(parseMap(pData, len));
                else if (c == '[')
                    return JsonSerialization::Variant(parseArray(pData, len));
                else if (c == '\"')
                    return JsonSerialization::Variant(parseString(pData, len));
                else if (c == 't' || c == 'f')
                    return JsonSerialization::Variant(parseBoolean(pData, len));
                else if (c == 'n')
                    return JsonSerialization::Variant(parseNull(pData, len));
                else if (isdigit(c) || c == '.')
                    return JsonSerialization::Variant(parseNumber(pData, len));
                else
                    throw std::runtime_error("Unknown character when parsing value");
            }

            ++pData;
            --len;
        }

        throw std::runtime_error("No value to parse");
    }

    void JsonParser::skipComma(const char *& pData, size_t &len) const
    {
        bool hasComma = false;
        while (len > 1)
        {
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == ',')
                {
                    if (hasComma)
                        std::runtime_error("Double comma delimiter");

                    hasComma = true;
                }
                else if (c == ']' || c == '}' || c == '\"')
                {
                    --pData;
                    ++len;
                    return;
                }
                else
                {
                     std::runtime_error("Uknown character - comma expected");
                }
            }
            
            ++pData;
            --len;
        }

        std::runtime_error("Comma problem");
    }

    JsonSerialization::VariantVector JsonParser::parseArray(const char *& pData, size_t &len) const
    {
        JsonSerialization::VariantVector variantVector;
        while (len > 1)
        {
            ++pData;
            --len;
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == ']')
                {
                    return variantVector;
                }

                JsonSerialization::Variant value = parseValue(pData, len);
                variantVector.push_back(value);
                skipComma(pData, len);
            }
        }

        if (pData[0] == ']')
            return variantVector;

        throw std::runtime_error("Unfinished vector");
    }

    JsonSerialization::VariantMap JsonParser::parseMap(const char *& pData, size_t &len) const
    {
        JsonSerialization::VariantMap variantMap;
        while (len > 1)
        {
            ++pData;
            --len;
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == '\"')
                {
                    std::string key = parseKey(pData, len);
                    gotoValue(pData, len);
                    JsonSerialization::Variant value = parseValue(pData, len);
                    variantMap.insert(std::make_pair(key, value));
                    skipComma(pData, len);
                }
                else if (c == '}')
                {
                    return variantMap;
                }
                else if (c == ',')
                {
                    if (variantMap.empty())
                        throw std::runtime_error("Comma on empty object");
                }
                else
                {
                    throw std::runtime_error("Wrong character in json");
                }
            }
        }

        if (pData[0] == '}')
            return variantMap;

        throw std::runtime_error("Unfinished map");
    }

    JsonSerialization::Variant JsonParser::parseObject(const char *& pData, size_t &len) const
    {
        while (len > 0)
        {
            char c = *pData;
            if (!isIgnorable(c))
            {
                if (c == '[')
                    return JsonSerialization::Variant(parseArray(pData, len));
                else if (c == '{')
                    return JsonSerialization::Variant(parseMap(pData, len));
                else
                    throw std::runtime_error("Invalid json - first char");
            }

            ++pData;
            --len;
        }

        throw std::runtime_error("Missing end of the object");
    }

    void JsonParser::fromJson(const std::string& jsonStr, const std::string& jsonSchema, JsonSerialization::Variant& jsonVariant) const
    {
        fromJson(jsonStr, jsonVariant);
        std::string r = jsonVariant.toJson();

        JsonSerialization::Variant schemaVariant, schemaVariantInternal;
        fromJson(jsonSchema, schemaVariant);
        interpretSchema(schemaVariant, schemaVariantInternal);
        fromJson(jsonStr, jsonVariant);
        compareVariantsAccordingSchema(schemaVariantInternal, jsonVariant);
    }

    void JsonParser::fromJson(const std::string& jsonStr, JsonSerialization::Variant& jsonVariant) const
    {
        const char *pData = jsonStr.c_str();
        size_t len = jsonStr.size();
        if (len < 2)
            throw std::runtime_error("No short json");

        jsonVariant = parseObject(pData, len);
    }

    void JsonParser::interpretSchema(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& internalSchemaVariant) const
    {

    }

    void JsonParser::compareVariantsAccordingSchema(const JsonSerialization::Variant& jsonVariant, const JsonSerialization::Variant& schemaVariant) const
    {

    }
}

#endif
