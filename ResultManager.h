#pragma once

//singleton

class ResultManager {
private:
	static ResultManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	ResultManager();

public:
	void init();
	static ResultManager& getInstance();
	void getRankList();
};

// theIntance needs to define since it is static
ResultManager ResultManager::theInstance;
#pragma once
