#ifndef __JSON_VARIANT_H
#define __JSON_VARIANT_H

#include <string>
#include <vector>
#include <map>

namespace JsonSerialization
{
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

    public:
        Variant();
        Variant(std::nullptr_t);
        Variant(int value);
        Variant(double value);
        Variant(bool value);
        Variant(const char* value);
        Variant(const std::string& value);
        Variant(std::string&& value) noexcept;
        Variant(const VariantVector& value);
        Variant(VariantVector&& value) noexcept;
        Variant(const VariantMap& value);
        Variant(VariantMap&& value) noexcept;
        Variant(const Variant& value);
        Variant(Variant&& value) noexcept;
        template <typename T> Variant(const std::vector<T> &value)
        {
            JsonSerialization::VariantVector *pVector = new JsonSerialization::VariantVector;
            for (const auto &v : value)
                pVector->push_back(v);

            type_ = JsonSerialization::Type::Vector;
            pData_.pData = pVector;
        }

        ~Variant();

        Variant& operator=(const Variant& value);
        Variant& operator=(Variant&& value) noexcept;
        bool operator==(const Variant& r) const;

        Type type() const;
        bool isEmpty() const;
        bool isNull() const;
        int toInt() const;
        double toNumber() const;
        bool toBool() const;
        const std::string& toString() const;
        const VariantVector& toVector() const;
        const VariantMap& toMap() const;

        template<typename T> T value() const
        {
            T val;
            _value(val);
            return val;
        }

        template<typename T> void value(T& val) const
        {
            _value(val);
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

        std::string toJson(bool pretty = false) const;
        static bool fromJson(const std::string& jsonStr, Variant& jsonVariant, std::string* errorStr = nullptr);
        static bool fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant, std::string* errorStr = nullptr);
        
    private:
        void clear();
        void copyAll(const Variant& value);
        void moveAll(Variant &&value) noexcept;
        std::string _toJson() const;
        std::string _toJson(int& intend) const;
        void _value(int& val) const;
        void _value(double& val) const;
        void _value(bool& val) const;
        void _value(std::string& val) const;
    };
}

#endif
