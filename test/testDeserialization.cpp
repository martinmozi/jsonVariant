#include <gtest/gtest.h>
#include "jsonVariant.hpp"

namespace
{
    std::string veggieSchema{R"(
    {
        "$id": "https://example.com/arrays.schema.json",
        "$schema": "http://json-schema.org/draft-07/schema#",
        "description": "A representation of a person, company, organization, or place",
        "type": "object",
        "properties": {
            "fruits": {
                "type": "array",
                "items": {
                    "type": "string"
                }
            },
            "vegetables": {
                "type": "array",
                "items": {
                    "$ref": "#/definitions/veggie"
                }
            }
        },
        "definitions": {
            "veggie": {
                "type": "object",
                "required": ["veggieName", "veggieLike"],
                "properties": {
                    "veggieName": {
                        "type": "string",
                        "description": "The name of the vegetable."
                    },
                    "veggieLike": {
                        "type": "boolean",
                        "description": "Do I like this vegetable?"
                    }
                }
            }
        }
    })"};

    std::string vegieJson {R"(
    {
        "fruits": ["apple", "orange", "pear"],
        "vegetables": [{
                "veggieName": "potato",
                "veggieLike": true
            },
            {
                "veggieName": "broccoli",
                "veggieLike": false
            }
        ]
    })"};
}

TEST(UnserializeVeggie, unserializeVeggie) {
    JsonSerialization::Variant veggieVariant;
    std::string errorStr;
    EXPECT_TRUE(JsonSerialization::Variant::fromJson(vegieJson, veggieSchema, veggieVariant, &errorStr));
}


 std::string teamJson(R"(
    {
        "id": 7,
        "coach": "Samuel Motivator",
        "assistant": null,
        "address": {
            "city": "Poprad",
            "country": "Slovakia"
        },
        "players": [{
                "name": "Stephen",
                "averageScoring": 16.4
            },
            {
                "name": "Geoffrey",
                "averageScoring": 12.7
            },
            {
                "name": "Anthony",
                "averageScoring": 14.8
            }
        ],
        "identificators": [1, 2, 3, 4]
    }
    )");

    std::string temaSchema(R"(
    {
        "type": "object",
        "properties": {
            "id": { "type": "integer" },
            "coach": { "type": "string", "minLength": 8 },
            "address": {
                "type": "object",
                "properties": {
                    "city": { "type": "string" },
                    "country": { "type": "string" }
                },
                "required": [ "city", "country" ]
            },
            "players": {
            "type": "array",
            "items": [
                {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string" },
                        "averageScoring": { "type": "number" }
                    },
                    "required": [ "name", "averageScoring" ]
                }
            ]
            },
            "identificators": {"type": "array", "items": {"type": "number"}}
        },
        "required": ["id", "coach", "address", "players", "identificators" ]
    }
    )");

    TEST(UnserializeTeam, unserializeTeam) {
    JsonSerialization::Variant teamVariant;
    std::string errorStr;
    EXPECT_TRUE(JsonSerialization::Variant::fromJson(teamJson, temaSchema, teamVariant, &errorStr));
}
