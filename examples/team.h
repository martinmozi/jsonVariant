#ifndef __TEAM_H
#define __TEAM_H

struct Team
{
	struct Player
	{
        std::string name;
        double averageScoring;
	};
	struct Address
	{
        std::string city;
        std::string country;
	};
	
	int64_t id;
	std::string coach;
	std::string assistant;
	Address address;
	std::vector<Player> players;
	std::vector<int64_t> identificators;
};

#endif
