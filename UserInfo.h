#pragma once

#include <string>
using namespace std;

// structure of a User object inside a database

class UserInfo {
public:
	UserInfo(int id, string username, int score);

	int id;
	string username;
	int score;
};
