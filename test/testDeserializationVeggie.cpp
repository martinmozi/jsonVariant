#define CATCH_CONFIG_MAIN  // This defines the main() function for Catch2
#include <catch2/catch_all.hpp>
#include "../include/jsonVariant.h"

namespace {
    std::string veggieSchema{ R"(
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
    })" };

    std::string vegieJson{ R"(
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
    })" };
}

TEST_CASE("Unserialize Veggie JSON", "[unserializeVeggie]") {
    JsonSerialization::Variant veggieVariant;
    std::string errorStr;

    bool success = JsonSerialization::Variant::fromJson(vegieJson, veggieSchema, veggieVariant, &errorStr);

    REQUIRE(success);  // Catch2 equivalent of EXPECT_TRUE
    REQUIRE(errorStr.empty());  // Ensure no error message was produced
}
