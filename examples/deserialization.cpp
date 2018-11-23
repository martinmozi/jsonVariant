#include "jsonVariant.hpp"
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
		]
	}
	)");

#ifdef __RAPID_JSON_BACKEND
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
			}
		},
		"required": ["id", "coach", "address", "players" ]
	}
	)");
#endif
}

int main()
{
	// assume that you have Team structure serialized in json string (with or without json validation schema)
	std::string errorStr;
	JsonSerialization::Variant variant;
#ifdef __RAPID_JSON_BACKEND
	if (!JsonSerialization::Variant::fromJson(jsonString, validationSchema, variant, &errorStr))
#else
	if (!JsonSerialization::Variant::fromJson(jsonString, variant, &errorStr))
#endif
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

	const JsonSerialization::VariantMap & addressMap = objectMap.at("address").toMap();
	team.address.city = addressMap.at("city").toString();
	team.address.country = addressMap.at("country").toString();

	const JsonSerialization::VariantVector & playersVector = objectMap.at("players").toVector();
	for (const JsonSerialization::Variant & v : playersVector)
	{
		Team::Player player;
		const JsonSerialization::VariantMap & playersMap = v.toMap();
		player.name = playersMap.at("name").toString();
		player.averageScoring = playersMap.at("averageScoring").toDouble();
		team.players.push_back(player);
	}

	// when you are using json schema you can trust the data types and directly convert variants to types according schema
	return 1;
}
