#include "internal/schemaValidator.h"

void JsonSerializationInternal::SchemaValidator::validate(const JsonSerialization::Variant& schemaVariant, const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the validate function
}
const JsonSerialization::Variant* JsonSerializationInternal::SchemaValidator::valueFromMap(const JsonSerialization::VariantMap& schemaVariantMap, const char* key, JsonSerialization::Variant::Type type, bool required, bool* isRef) const
{
    // Implementation of the valueFromMap function
}
void JsonSerializationInternal::SchemaValidator::compare(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    // Implementation of the compare function
}
void JsonSerializationInternal::SchemaValidator::compareMap(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    // Implementation of the compareMap function
}
void JsonSerializationInternal::SchemaValidator::compareVector(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    // Implementation of the compareVector function
}
void JsonSerializationInternal::SchemaValidator::compareString(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the compareString function
}
void JsonSerializationInternal::SchemaValidator::compareNumber(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the compareNumber function
}
void JsonSerializationInternal::SchemaValidator::compareInteger(const JsonSerialization::VariantMap& schemaVariantMap, const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the compareInteger function
}
void JsonSerializationInternal::SchemaValidator::checkBoolean(const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the checkBoolean function
}
void JsonSerializationInternal::SchemaValidator::checkNull(const JsonSerialization::Variant& jsonVariant) const
{
    // Implementation of the checkNull function
}
const JsonSerialization::VariantMap& JsonSerializationInternal::SchemaValidator::fromRef(const std::string& refPath, const JsonSerialization::VariantMap& wholeSchemaVariantMap) const
{
    // Implementation of the fromRef function
}
std::vector<std::string> JsonSerializationInternal::SchemaValidator::tokenize(const std::string& str, char delim) const
{
    // Implementation of the tokenize function
}