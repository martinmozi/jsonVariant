#ifndef __TEAM_H
#define __TEAM_H

struct Team
{
	struct Player
	{
        std::string name;
        double averageScoring;
		Player() : averageScoring(0.0) {}
        Player(const std::string& _name, double _averageScoring) : name(_name), averageScoring(_averageScoring) {}
	};
	struct Address
	{
        std::string city;
        std::string country;
	};
	
	int id;
	std::string coach;
	std::string assistant;
	Address address;
	std::vector<Player> players;
	std::vector<int> identificators;
};

#endif
