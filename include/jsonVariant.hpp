#ifndef __JSON_VARIANT_HPP
#define __JSON_VARIANT_HPP

#include <set>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include <regex>
#include <limits>

#ifdef _WIN32
    std::string endLineStr("\r\n");
#else
    std::string endLineStr("\n");
#endif

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

    namespace JsonSerializationInternal
    {
        class JsonParser
        {
        public:
            JsonParser() = default;
            void fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant) const;
            void fromJson(const std::string& jsonStr, Variant& jsonVariant) const;

        private:
            VariantVector parseArray(const char *& pData) const;
            VariantMap parseMap(const char *& pData) const;
            Variant parseObject(const char *& pData) const;
            std::string parseKey(const char *& pData) const;
            Variant parseValue(const char *& pData) const;
            std::string parseString(const char *& pData) const;
            bool parseBoolean(const char *& pData) const;
            double parseNumber(const char *& pData) const;
            Variant parseNull(const char *& pData) const;
            void gotoValue(const char *& pData) const;
            std::string trim(const std::string& jsonStr) const;
            bool isIgnorable(char d) const;
        };
    }

    class Variant
    {
    public:
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

    private:
        typedef union 
        {
            bool boolValue;
            double numberValue;
            void* pData;
        } PDATA;

        PDATA pData_;
        Type type_;

    private:
        void clear()
        {
            switch (type_)
            {
            case Type::Number:
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

            case Type::Number:
                pData_.numberValue = value.pData_.numberValue;
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

        std::string _toJson(int& intend) const
        {
            switch (type_)
            {
            case Type::Null:
                return "null";

            case Type::Number:
                return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t) pData_.numberValue) : std::to_string(pData_.numberValue);

            case Type::Bool:
                return pData_.boolValue ? "true" : "false";

            case Type::String:
                return std::string("\"") + *((std::string*)pData_.pData) + "\"";

            case Type::Vector:
            {
                VariantVector* pJsonVariantVector = (VariantVector*)pData_.pData;
                if (pJsonVariantVector->empty())
                {
                    return "[]";
                }
                else
                {
                    intend += 4;
                    std::string resultStr("[" + endLineStr);
                    for (Variant& jsonVariant : *pJsonVariantVector)
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

            case Type::Map:
            {
                VariantMap* pJsonVariantMap = (VariantMap*)pData_.pData;
                if (pJsonVariantMap->empty())
                {
                    return "{}";
                }
                else
                {
                    std::string resultStr("{");
                    intend += 4;
                    for (auto& it : *pJsonVariantMap)
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
        :   Variant((double)value) 
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

        Variant(const char* value) 
        :   Variant(std::move(std::string(value))) 
        {
        }

        Variant(const std::string& value)
        {
            type_ = Type::String;
            pData_.pData = new std::string(value);
        }

        Variant(std::string&& value) noexcept
        {
            type_ = Type::String;
            pData_.pData = new std::string(std::move(value));
        }

        Variant(const VariantVector& value)
        {
            type_ = Type::Vector;
            pData_.pData = new VariantVector(value);
        }

        Variant(VariantVector&& value) noexcept
        {
            type_ = Type::Vector;
            pData_.pData = new VariantVector(std::move(value));
        }

        Variant(const VariantMap& value)
        {
            type_ = Type::Map;
            pData_.pData = new VariantMap(value);
        }

        Variant(VariantMap && value) noexcept
        {
            type_ = Type::Map;
            pData_.pData = new VariantMap(std::move(value));
        }

        Variant(const Variant& value) 
        { 
            copyAll(value);
        }

        Variant(Variant&& value) noexcept
        { 
            moveAll(value);
        }

        Variant& operator=(const Variant& value)
        {
            copyAll(value);
            return *this;
        }

        bool operator==(const Variant& r) const
        {
            if (type_ != r.type_)
                return false;

            switch (type_)
            {
            case Type::Empty:
            case Type::Null:
            case Type::Number:
            case Type::Bool:
                return (pData_.pData == r.pData_.pData);

            case Type::String:
                return *((const std::string*)pData_.pData) == *((const std::string*)r.pData_.pData);

            case Type::Vector:
                return *((const VariantVector*)pData_.pData) == *((const VariantVector*)r.pData_.pData);

            case Type::Map:
                return *((const VariantMap*)pData_.pData) == *((const VariantMap*)r.pData_.pData);
   
            default:
                return false;
            }
        }

        template<typename T> Variant(const std::vector<T>& value)
        {
            VariantVector *pVector = new VariantVector;
            for (const auto& v : value)
                pVector->push_back(v);

            type_ = Type::Vector;
            pData_.pData = pVector;
        }

        Variant& operator=(Variant&& value)noexcept
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

        int toInt() const
        {
            if (type_ == Type::Number)
                return (int)pData_.numberValue;

            throw std::runtime_error("Not integer in variant");
        }

        double toNumber() const
        {
            if (type_ == Type::Number)
                return pData_.numberValue;

            throw std::runtime_error("Not number in variant");
        }

        bool toBool() const
        {
            if (type_ == Type::Bool)
                return pData_.boolValue;

            throw std::runtime_error("Not bool in variant");
        }

        const std::string& toString() const
        {
            if (type_ == Type::String)
                return *(std::string*)pData_.pData;

            throw std::runtime_error("Not string in variant");
        }

        const VariantVector& toVector() const
        {
            if (type_ == Type::Vector)
                return *(VariantVector*)pData_.pData;

            throw std::runtime_error("Not vector in variant");
        }

        const VariantMap& toMap() const
        {
            if (type_ == Type::Map)
                return *(VariantMap*)pData_.pData;

            throw std::runtime_error("Not map in variant");
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

            case Type::Number:
                return std::abs(pData_.numberValue - (int64_t)pData_.numberValue) < std::numeric_limits<double>::epsilon() ? std::to_string((int64_t) pData_.numberValue) : std::to_string(pData_.numberValue);

            case Type::Bool:
                return pData_.boolValue ? "true" : "false";

            case Type::String:
                return std::string("\"") + *((std::string*)pData_.pData) + "\"";

            case Type::Vector:
            {
                VariantVector* pJsonVariantVector = (VariantVector*)pData_.pData;
                if (pJsonVariantVector->empty())
                {
                    return "[]";
                }
                else
                {
                    std::string resultStr("[");
                    for (Variant& jsonVariant : *pJsonVariantVector)
                        resultStr += jsonVariant.toJson() + ",";

                    resultStr[resultStr.size() -1] = ']';
                    return resultStr;
                }
            }
            break;

            case Type::Map:
            {
                VariantMap* pJsonVariantMap = (VariantMap*)pData_.pData;
                if (pJsonVariantMap->empty())
                {
                    return "{}";
                }
                else
                {
                    std::string resultStr("{");
                    for (auto& it : *pJsonVariantMap)
                        resultStr += "\"" + it.first + "\":" + it.second.toJson() + ",";

                    resultStr[resultStr.size() -1] = '}';
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
            return pretty ? _toJson(intend) : toJson();
        }

        static bool fromJson(const std::string& jsonStr, Variant& jsonVariant, std::string* errorStr = nullptr)
        {
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
    template<> void Variant::value<double>(double& t) const { t = toNumber(); }
    template<> double Variant::value<double>() const { return toNumber(); }
    template<> void Variant::value<int>(int& t) const { t = (int)toInt(); }
    template<> int Variant::value<int>() const { return (int)toInt(); }
    template<> void Variant::value<VariantVector>(VariantVector& t) const { t = toVector(); }
    template<> const VariantVector & Variant::valueByRef<VariantVector>() const { return toVector(); }
    template<> void Variant::value<VariantMap>(VariantMap& t) const { t = toMap(); }
    template<> const VariantMap & Variant::valueByRef<VariantMap>() const { return toMap(); }

// #################################################################################################################
// #################################################################################################################

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
            void validate(const Variant& schemaVariant, const Variant& jsonVariant) const;

        private:
            const Variant* valueFromMap(const VariantMap& schemaVariantMap, const char* key, Variant::Type type, bool required, bool* isRef = nullptr) const;
            void compare(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const;
            void compareMap(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const;
            void compareVector(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const;
            void compareString(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const;
            void compareNumber(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const;
            void compareInteger(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const;
            void checkBoolean(const Variant& jsonVariant) const;
            void checkNull(const Variant& jsonVariant) const;
            const VariantMap& fromRef(const std::string &refPath,  const VariantMap& wholeSchemaVariantMap) const;
            std::vector<std::string> tokenize(const std::string &str, char delim) const;
        };

        inline bool JsonParser::isIgnorable(char d) const
        {
            return (d == ' ' || d == '\n' || d == '\t' || d == '\r');
        }

        void JsonParser::gotoValue(const char *& pData) const
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

        std::string JsonParser::parseKey(const char *& pData) const
        {
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

        std::string JsonParser::parseString(const char *& pData) const
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

        bool JsonParser::parseBoolean(const char *& pData) const
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

        double JsonParser::parseNumber(const char *& pData) const
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

        Variant JsonParser::parseNull(const char *& pData) const
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

        Variant JsonParser::parseValue(const char *& pData) const
        {
            Variant variant;
            char c = *pData;
            if (c == '{')
                variant = Variant(parseMap(pData));
            else if (c == '[')
                variant = Variant(parseArray(pData));
            else if (c == '\"')
                variant = Variant(parseString(pData));
            else if (c == 't' || c == 'f')
                variant = Variant(parseBoolean(pData));
            else if (c == 'n')
                variant = Variant(parseNull(pData));
            else if (isdigit(c) || c == '.' || c == '-')
            {
                double d = parseNumber(pData);
                variant = (d == (int)d) ? Variant((int)d) : Variant(d);
            }
            else
                throw std::runtime_error("Unknown character when parsing value");

            c = *pData;
            if (c == ',' || c == '}' || c == ']')
                return variant;

            throw std::runtime_error("Missing delimiter");
        }

        VariantVector JsonParser::parseArray(const char *& pData) const
        {
            ++pData;
            VariantVector variantVector;
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

        VariantMap JsonParser::parseMap(const char *& pData) const
        {
            ++pData;
            VariantMap variantMap;
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

        Variant JsonParser::parseObject(const char *& pData) const
        {
            while (pData != NULL)
            {
                char c = *pData;
                if (c == '{')
                    return Variant(parseMap(pData));
                else if (c == '[')
                    return Variant(parseArray(pData));
                else
                    throw std::runtime_error("Invalid json - first char");

                ++pData;
            }

            throw std::runtime_error("Missing end of the object");
        }

        void JsonParser::fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant) const
        {
            Variant schemaVariant, schemaVariantInternal;
            fromJson(jsonSchema, schemaVariant);
            fromJson(jsonStr, jsonVariant);
            SchemaValidator().validate(schemaVariant, jsonVariant);
        }

        std::string JsonParser::trim(const std::string& jsonStr) const
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

                if (!isIgnorable(c) || record)
                {
                    p[j] = c;
                    j++;
                }
            }

            return outStr;
        }

        void JsonParser::fromJson(const std::string& jsonStr, Variant& jsonVariant) const
        {
            std::string _jsonStr = trim(jsonStr);
            const char *pData = _jsonStr.c_str();
            size_t len = _jsonStr.size();
            if (len < 2)
                throw std::runtime_error("No short json");

            jsonVariant = parseObject(pData);
        }

        // #######################################################################################################################################################
        // #######################################################################################################################################################

        void SchemaValidator::validate(const Variant& schemaVariant, const Variant& jsonVariant) const
        {
            if (schemaVariant.type() != Variant::Type::Map)
                throw std::runtime_error("Bad schema type");

            const auto& schemaVariantMap = schemaVariant.toMap();
            compare(schemaVariantMap, jsonVariant, schemaVariantMap);
        }

        const Variant* SchemaValidator::valueFromMap(const VariantMap& schemaVariantMap, const char* key, Variant::Type type, bool required, bool *isRef) const
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
                        if (it->second.type() != Variant::Type::String)
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

        std::vector<std::string> SchemaValidator::tokenize(const std::string &str, char delim) const
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

        const VariantMap& SchemaValidator::fromRef(const std::string &refPath,  const VariantMap& wholeSchemaVariantMap) const
        {
            std::vector<std::string> pathVector = tokenize(refPath, '/');
            const VariantMap * pVariantMap = &wholeSchemaVariantMap;
            for (const std::string & s : pathVector)
            {
                const auto it = pVariantMap->find(s);
                if (it == pVariantMap->end())
                    throw std::runtime_error("Unable find ref according path");

                if (it->second.type() != Variant::Type::Map)
                    throw std::runtime_error("Ref link is not valid");

                pVariantMap = &it->second.toMap();

            }

            return *pVariantMap;
        }

        void SchemaValidator::compare(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const
        {
            // todo enum and const - not implemented
            bool isRef = false;
            const std::string & typeStr = valueFromMap(schemaVariantMap, "type", Variant::Type::String, true, &isRef)->toString();
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

        void SchemaValidator::compareMap(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const
        {
            std::set<std::string> required;
            const VariantMap &propertiesVariantMap = valueFromMap(schemaVariantMap, "properties", Variant::Type::Map, true)->toMap();
            const Variant *pRequiredVariant = valueFromMap(schemaVariantMap, "required", Variant::Type::Vector, false);
            if (pRequiredVariant)
            {
                const VariantVector& requiredVariantVector = pRequiredVariant->toVector();
                for (const auto &v : requiredVariantVector)
                    required.insert(v.toString());
            }
            if (jsonVariant.type() != Variant::Type::Map)
                throw std::runtime_error("Map required");

            const VariantMap &jsonVariantMap = jsonVariant.toMap();
            for (const auto& it : propertiesVariantMap)
            {
                if (it.second.type() != Variant::Type::Map)
                    throw std::runtime_error(std::string("Missing map for key: ") + it.first);

                auto iter = jsonVariantMap.find(it.first);
                if (iter == jsonVariantMap.end() && required.find(it.first) != required.end())
                    throw std::runtime_error(std::string("Missing key in map: ") + it.first);

                compare(it.second.toMap(), iter->second, wholeSchemaVariantMap);
            }

            const Variant *pMinProperties = valueFromMap(schemaVariantMap, "minProperties", Variant::Type::Number, false);
            if (pMinProperties && pMinProperties->toInt() > (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is smaller as defined in minProperties");

            const Variant *pMaxProperties = valueFromMap(schemaVariantMap, "maxProperties", Variant::Type::Number, false);
            if (pMaxProperties && pMaxProperties->toInt() < (int)jsonVariantMap.size())
                throw std::runtime_error("Size of map is greater as defined in maxProperties");

            const Variant *pDependentRequired = valueFromMap(schemaVariantMap, "dependentRequired", Variant::Type::Map, false);
            if (pDependentRequired)
                throw std::runtime_error("not supported dependentRequired");
        }

        void SchemaValidator::compareVector(const VariantMap& schemaVariantMap, const Variant& jsonVariant, const VariantMap& wholeSchemaVariantMap) const
        {
            const auto it = schemaVariantMap.find("items");
            if (it == schemaVariantMap.end())
                throw std::runtime_error("Expected items for schema vector");

            const VariantMap* pSchemaVariantMap = nullptr;
            const VariantVector* pSchemaVariantVector = nullptr;
            if (it->second.type() == Variant::Type::Map)
            {
                pSchemaVariantMap = &it->second.toMap();
            }
            else if (it->second.type() == Variant::Type::Vector)
            {
                const auto & schemaVariantVector = it->second.toVector();
                if (schemaVariantVector.empty())
                    throw std::runtime_error("Expected non empty schema vector");

                if (schemaVariantVector.size() == 1)
                {
                    const auto & schVariant = schemaVariantVector[0];
                    if (schVariant.type() != Variant::Type::Map)
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

            if (jsonVariant.type() != Variant::Type::Vector)
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
                    if (schVariant.type() != Variant::Type::Map)
                        throw std::runtime_error("Expected map in json schema vector");

                    compare(schVariant.toMap(), variantVector[i], wholeSchemaVariantMap);
                }    
            }

            const Variant *pMinItems = valueFromMap(schemaVariantMap, "minItems", Variant::Type::Number, false);
            if (pMinItems && pMinItems->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");

            const Variant *pMaxItems = valueFromMap(schemaVariantMap, "maxItems", Variant::Type::Number, false);
            if (pMaxItems && pMaxItems->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");

            const Variant *pMinContains = valueFromMap(schemaVariantMap, "minContains", Variant::Type::Number, false);
            if (pMinContains && pMinContains->toInt() > (int)variantVector.size())
                throw std::runtime_error("Too short vector");

            const Variant *pMaxContains = valueFromMap(schemaVariantMap, "maxContains", Variant::Type::Number, false);
            if (pMaxContains && pMaxContains->toInt() < (int)variantVector.size())
                throw std::runtime_error("Too long vector");

            const Variant *pUniqueItems = valueFromMap(schemaVariantMap, "uniqueItems", Variant::Type::Number, false);
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

        void SchemaValidator::compareString(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const
        {
            if (jsonVariant.type() != Variant::Type::String)
                throw std::runtime_error("Expected string value");

            const std::string& value = jsonVariant.toString();
            const Variant *pMinLength = valueFromMap(schemaVariantMap, "minLength", Variant::Type::Number, false);
            if (pMinLength && pMinLength->toInt() > (int)value.size())
                throw std::runtime_error(std::string("Too short string: ") + value);

            const Variant *pMaxLength = valueFromMap(schemaVariantMap, "maxLength", Variant::Type::Number, false);
            if (pMaxLength && pMaxLength->toInt() < (int)value.size())
                throw std::runtime_error(std::string("Too long string: ") + value);

            const Variant *pPattern = valueFromMap(schemaVariantMap, "pattern", Variant::Type::String, false);
            if (pPattern)
            {
                std::regex expr(pPattern->toString());
                std::smatch sm;
                if (! std::regex_match(value, sm, expr))
                    throw std::runtime_error(std::string("String doesn't match the pattern: ") + pPattern->toString());
            }

            const Variant *pFormat = valueFromMap(schemaVariantMap, "format", Variant::Type::String, false);
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

        void SchemaValidator::compareNumber(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const
        {
            if (jsonVariant.type() != Variant::Type::Number)
                throw std::runtime_error("Expected numeric value");

            double value = jsonVariant.toNumber();
            const Variant *pMinimum = valueFromMap(schemaVariantMap, "minimum", Variant::Type::Number, false);
            if (pMinimum && pMinimum->toNumber() > value)
                throw std::runtime_error("Numeric value is smaller than minimum");

            const Variant *pMaximum = valueFromMap(schemaVariantMap, "maximum", Variant::Type::Number, false);
            if (pMaximum && pMaximum->toNumber() < value)
                throw std::runtime_error("Numeric value is greater than maximum");

            const Variant *pExclusiveMinimum = valueFromMap(schemaVariantMap, "exclusiveMinimum", Variant::Type::Number, false);
            if (pExclusiveMinimum && pExclusiveMinimum->toNumber() >= value)
                throw std::runtime_error("Numeric value is smaller than exclusive minimum");

            const Variant *pExclusiveMaximum = valueFromMap(schemaVariantMap, "exclusiveMaximum", Variant::Type::Number, false);
            if (pExclusiveMaximum && pExclusiveMaximum->toNumber() <= value)
                throw std::runtime_error("Numeric value is greater than exclusive maximum");

            const Variant *pMultipleOf = valueFromMap(schemaVariantMap, "multipleOf", Variant::Type::Number, false);
            if (pMultipleOf)
            {
                double multipleOf = pMultipleOf->toNumber();
                if (!isInteger(multipleOf) || multipleOf <= 0)
                    throw std::runtime_error("Multiple of has to be an positive number");

                if (! isInteger(value / multipleOf))
                    throw std::runtime_error("Multiple of division must be an integer");
        
            }
        }

        void SchemaValidator::compareInteger(const VariantMap& schemaVariantMap, const Variant& jsonVariant) const
        {
            compareNumber(schemaVariantMap, jsonVariant);
            if (! isInteger(jsonVariant.toNumber()))
                throw std::runtime_error("Expected integer value");
        }

        void SchemaValidator::checkBoolean(const Variant& jsonVariant) const
        {
            if (jsonVariant.type() != Variant::Type::Bool)
                throw std::runtime_error("Expected boolean value");
        }

        void SchemaValidator::checkNull(const Variant& jsonVariant) const
        {
            if (jsonVariant.type() != Variant::Type::Null)
                throw std::runtime_error("Expected null value");
        }
    }
}

#endif
