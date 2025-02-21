#define CATCH_CONFIG_MAIN  // This defines the main() function for Catch2
#include <catch2/catch_all.hpp>
#include "../include/jsonVariant.h"

namespace {
    std::string teamJson{ R"(
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
    )" };

    std::string teamSchema{ R"(
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
                "items": {
                    "type": "object",
                    "properties": {
                        "name": { "type": "string" },
                        "averageScoring": { "type": "number" }
                    },
                    "required": [ "name", "averageScoring" ]
                }
            },
            "identificators": {"type": "array", "items": {"type": "number"}}
        },
        "required": ["id", "coach", "address", "players", "identificators"]
    }
    )" };
}

TEST_CASE("Unserialize Team JSON", "[unserializeTeam]") {
    JsonSerialization::Variant teamVariant;
    std::string errorStr;

    bool success = JsonSerialization::Variant::fromJson(teamJson, teamSchema, teamVariant, &errorStr);

    REQUIRE(success);
    REQUIRE(errorStr.empty());
}