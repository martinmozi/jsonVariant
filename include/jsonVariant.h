#ifndef __JSON_VARIANT_H
#define __JSON_VARIANT_H

#include <string>
#include <vector>
#include <map>

namespace JsonSerialization
{
    class Variant;
    typedef std::map<std::string, Variant> VariantMap;
    typedef std::vector<Variant> VariantVector;
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

            type_ = JsonSerialization::Variant::Type::Vector;
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

        template<typename T> void value(T& val) const;
        template<typename T> T value() const;
        template<typename T> const T& valueByRef() const;
        template<typename T> std::vector<T> valueVector() const;
        template<typename T> void valueVector(std::vector<T>& val) const;

        std::string toJson() const;
        std::string toJson(bool pretty) const;

        static bool fromJson(const std::string& jsonStr, Variant& jsonVariant, std::string* errorStr = nullptr);
        static bool fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant, std::string* errorStr = nullptr);
    private:
        void clear();
        void copyAll(const Variant& value);
        void moveAll(Variant &value) noexcept;
        std::string _toJson(int &intend) const;
    };
}

#endif
