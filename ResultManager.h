#pragma once

//singleton

class ResultManager {
private:
	static ResultManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	ResultManager();
	void init();

public:
	static ResultManager& getInstance();
	void getRankList();
};