#include <gtest/gtest.h>
#include "jsonVariant.hpp"

namespace 
{
    std::string jsonSchema = R"(
    {
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$id": "https://example.com/complex-example.schema.json",
        "title": "ComplexExample",
        "type": "object",
        "properties": {
            "username": {
                "type": "string",
                "minLength": 3,
                "maxLength": 30
            },
            "age": {
                "type": "integer",
                "minimum": 18
            },
            "email": {
                "type": "string",
                "format": "email"
            },
            "homepage": {
                "type": "string",
                "format": "uri"
            },
            "membership": {
                "type": "string",
                "enum": ["basic", "premium", "admin"]
            },
            "preferences": {
                "type": "object",
                "properties": {
                    "newsletter": {
                        "type": "boolean"
                    },
                    "interests": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        },
                        "minItems": 1,
                        "uniqueItems": true
                    }
                },
                "required": ["newsletter"]
            },
            "address": {
                "type": "object",
                "properties": {
                    "street": { "type": "string" },
                    "city": { "type": "string" },
                    "postalCode": {
                        "type": "string",
                        "pattern": "^[0-9]{5}(-[0-9]{4})?$"
                    }
                },
                "required": ["street", "city", "postalCode"]
            },
            "creationDate": {
                "type": "string",
                "format": "date-time"
            }
        },
        "required": ["username", "email", "age", "membership", "preferences", "address"],
        "additionalProperties": false
    })";

    std::string jsonString = R"(
    {
        "username": "johndoe123",
        "age": 25,
        "email": "john.doe@example.com",
        "homepage": "https://johndoe.com",
        "membership": "premium",
        "preferences": {
            "newsletter": true,
            "interests": ["technology", "gaming"]
        },
        "address": {
            "street": "1234 Elm Street",
            "city": "Somewhere",
            "postalCode": "12345"
        },
        "creationDate": "2021-07-01T12:00:00Z"
    })";

    TEST(TestDraft7Schema, testDraft7Schema) 
    {
        JsonSerialization::Variant variant;
        std::string errorStr;
        EXPECT_TRUE(JsonSerialization::Variant::fromJson(jsonString, jsonSchema, variant, &errorStr));
    }
}
