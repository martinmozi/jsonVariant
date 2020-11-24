jsonVariant is a generic C++ header-only library for deserializing JSON strings to the recursive variants and serializing recursive variants to JSON strings

# Installation

Copy the header file into your project or use cmake for installing header to your system folders

# Prerequisities

No addtional dependencies except C++ standard libraries.

# Build & Install

* `cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON .` - Replace _Release_ with _Debug_ for a build with debug symbols.
* `cmake . && make install` - use sudo user for installing to system directories on Linux

# Usage

Read json string to recursive variant structure. For advanced json schema validation and parsing with generic approach see [examples](https://github.com/martinmozi/jsonVariant/tree/master/examples/deserialization.cpp)

```c++
#include "jsonVariant.hpp"

namespace
{
    struct Team
    {
        struct Player
        {
            std::string name;
            double averageScoring;
        };
        
        int64_t id;
        std::string coach;
        std::string assistant;
        std::vector<Player> players;
    };
    
    std::string jsonString(R"(
    {
        "id": 2,
        "coach": "Samuel Motivator",
        "assistant": null,
        "players": [{
                "name": "Stephen",
                "averageScoring": 16.4
            },
            {
                "name": "Anthony",
                "averageScoring": 14.8
            }
        ]
    })");
}

int main()
{
    std::string errorStr;
    JsonSerialization::Variant variant;
    if (!JsonSerialization::Variant::fromJson(jsonString, variant, &errorStr))
    {
        printf("Unable to parse json with error: %s", errorStr.c_str());
        return -1;
    }
    
    Team team;
    const JsonSerialization::VariantMap & objectMap = variant.toMap();
    team.id = objectMap.at("id").toInt64();
    team.coach = objectMap.at("coach").toString();
    if (!objectMap.at("assistant").isNull())
        team.coach = objectMap.at("assistant").toString();
    
    const JsonSerialization::VariantVector & playersVector = objectMap.at("players").toVector();
    for (const JsonSerialization::Variant & v : playersVector)
    {
        Team::Player player;
        const JsonSerialization::VariantMap & playersMap = v.toMap();
        player.name = playersMap.at("name").toString();
        player.averageScoring = playersMap.at("averageScoring").toDouble();
        team.players.push_back(player);
    }
    
    return 0;
}
```

Create recursive structure and than flush it as json string

```c++

#include "jsonVariant.hpp"

namespace
{
    struct Team
    {
        struct Player
        {
            std::string name;
            double averageScoring;
        };
        
        int64_t id;
        std::string coach;
        std::string assistant;
        std::vector<Player> players;
    };
}

int main()
{
    Team team;
    team.id = 2;
    team.coach = "Samuel Motivator";
    team.players =
    {
        { "Stephen", 16.4 },
        { "Anthony", 14.8 }
    };
    
    JsonSerialization::VariantVector playersVector;
    for (const Team::Player & player : team.players)
    {
        playersVector.push_back(std::move(JsonSerialization::VariantMap
        {
            { "name", player.name },
            { "averageScoring", player.averageScoring }
        }));
    }
    
    JsonSerialization::VariantMap variantMap
    {
        { "id", team.id },
        { "coach", team.coach },
        { "assistant", nullptr },
        { "players", std::move(playersVector) }
    };
    
    JsonSerialization::Variant finalVariant(variantMap);
    printf("%s", finalVariant.toJson().c_str());
    return 1;
}

```
