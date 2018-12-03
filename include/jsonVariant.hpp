#ifndef __JSON_VARIANT_HPP
#define __JSON_VARIANT_HPP

#include <assert.h>
#include <map>
#include <string>
#include <vector>

#ifdef __RAPID_JSON_BACKEND
	#include <rapidjson/document.h>
	#include <rapidjson/schema.h>
#elif __JSON_CPP_BACKEND
	#include <json.hpp>
#else
	#error Define backend for json parsing
#endif

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
			NotDefined,
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
			type_ = Type::NotDefined;
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
			value.type_ = Type::NotDefined;
		}

#ifdef __RAPID_JSON_BACKEND
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
				for (auto it = value.Begin(); it != value.End(); it++)
					variantVector.push_back(_fromJson(*it));

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
#else
		static Variant _fromJson(const nlohmann::json & value)
		{
			if (value.is_null())
			{
				return Variant(nullptr);
			}
			else if (value.is_number_integer())
			{
				return Variant(value.get<int64_t>());
			}
			else if (value.is_number_float())
			{
				return Variant(value.get<double>());
			}
			else if (value.is_boolean())
			{
				return Variant(value.get<bool>());
			}
			else if (value.is_string())
			{
				return Variant(value.get<std::string>());
			}
			else if (value.is_array())
			{
				VariantVector variantVector;
				for (auto it = value.begin(); it != value.end(); it++)
					variantVector.push_back(_fromJson(*it));

				return Variant(variantVector);
			}
			else if (value.is_object())
			{
				VariantMap variantMap;
				for (auto it = value.begin(); it != value.end(); it++)
					variantMap.insert(std::make_pair(it.key(), _fromJson(it.value())));

				return Variant(variantMap);
			}

			return Variant();
		}
#endif

	public:
		Variant() 
		{ 
			type_ = Type::NotDefined;
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

		Variant(const VariantVector& value)
		{
			type_ = Type::Vector;
			pData_.pData = new VariantVector(value);
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
#ifdef __RAPID_JSON_BACKEND
			rapidjson::Document document;
			auto& doc = document.Parse(jsonStr.c_str());
			if (doc.HasParseError())
			{
				if (errorStr)
					*errorStr = std::string("Parse error with code: ") + std::to_string(doc.GetParseError()) + " and offset: " + std::to_string(doc.GetErrorOffset());

				return false;
			}
#else
			nlohmann::json doc;
			try
			{
				doc = nlohmann::json::parse(jsonStr);
			}
			catch (nlohmann::json::parse_error& e)
			{
				if (errorStr)
					*errorStr = std::string("Parse error with code: ") + e.what();

				return false;
			}
#endif
			jsonVariant = _fromJson(doc);
			return true;
		}

#ifdef __RAPID_JSON_BACKEND
		static bool fromJson(const std::string& jsonStr, const std::string& jsonSchema, Variant& jsonVariant, std::string* errorStr = nullptr)
		{
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
			return true;
		}
#endif
	};
}

#endif
