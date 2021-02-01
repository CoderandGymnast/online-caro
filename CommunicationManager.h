#pragma once

//singleton

class CommunicationManager {
private:
	static CommunicationManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	CommunicationManager();
	void init();

public:
	static CommunicationManager& getInstance();
};

