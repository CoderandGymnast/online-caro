#pragma once

//singleton

class MatchManager {
private:
	static MatchManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	MatchManager();

public:
	void init();
	static MatchManager& getInstance();
	void createRoom();
	void manageScore();
};
