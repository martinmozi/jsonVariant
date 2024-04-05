#ifndef __TEAM_H
#define __TEAM_H

struct Team
{
	struct Player
	{
        std::string name;
        double averageScoring;
		Player() : averageScoring(0.0) {}
		Player(std::string n, double a) : name(n), averageScoring(a) {}
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
	Team() : id(0) {}
	Team(int i, std::string c, Address a, std::vector<Player> p) : id(i), coach(c), address(a), players(p) {}
};

#endif
