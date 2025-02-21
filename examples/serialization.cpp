#include "../include/jsonVariant.h"

#include "team.h"

int main()
{
    // assume that the structure was somehow initialized
    Team team;
    team.id = 7;
    team.coach = "Samuel Motivator";
    team.address = { "Poprad", "Slovakia" };
    team.players = 
    {
        { "Stephen", 16.4 },
        { "Geoffrey", 12.7 },
        { "Anthony", 14.8 }
    };
    
    team.identificators = { 1, 2, 3, 4 };

    JsonSerialization::VariantVector playersVector;
    for (const Team::Player & player : team.players)
    {
        playersVector.push_back(std::move(
        JsonSerialization::VariantMap
        {
            { "name", player.name },
            { "averageScoring", player.averageScoring }
        }));
    }

    JsonSerialization::VariantMap variantMap
    {
        {"id",  team.id},
        {"coach", team.coach},
        {"assistant", nullptr},
        {"address", std::move(JsonSerialization::VariantMap
            {
                {"city", team.address.city},
                {"country", team.address.country},
            })
        },
        {"players", playersVector},
        {"identificators", team.identificators}
    };

    // convert final variant to json string
    printf("%s", JsonSerialization::Variant(variantMap).toJson().c_str());
    
    return 1;
}
