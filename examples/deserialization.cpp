#include "../include/jsonVariant.hpp"
#include "team.h"

namespace
{
    std::string jsonString(R"(
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

    std::string validationSchema(R"(
    {
        "type": "object",
        "properties": {
            "id": { "type": "integer" },
            "coach": { "type": "string" },
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
}

int main()
{
    // assume that you have Team structure serialized in json string (with or without json validation schema)
    std::string errorStr;
    JsonSerialization::Variant variant;
    if (!JsonSerialization::Variant::fromJson(jsonString, validationSchema, variant, &errorStr))
    {
        printf("Unable to parse json with error: %s", errorStr.c_str());
        return -1;
    }

    Team team;
    const JsonSerialization::VariantMap & objectMap = variant.toMap();
    objectMap.value("id", team.id, (int64_t)-1); // retrieve with default value
    objectMap("coach").value(team.coach); // in this approach "coach" must exist
    if (objectMap.contains("assistant") && !objectMap.isNull("assistant"))
        objectMap("assistant").value(team.assistant);

    const auto & addressMap = objectMap("address").toMap();
    addressMap("city").value(team.address.city);
    addressMap("country").value(team.address.country);

    const auto & playersVector = objectMap("players").toVector();
    for (const JsonSerialization::Variant & v : playersVector)
    {
        Team::Player player;
        const JsonSerialization::VariantMap & playersMap = v.toMap();
        playersMap("name").value(player.name);
        playersMap("averageScoring").value(player.averageScoring);
        team.players.push_back(player);
    }

    objectMap("identificators").valueVector(team.identificators);
    return 1;
}
