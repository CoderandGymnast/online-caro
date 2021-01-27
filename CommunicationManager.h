#pragma once

//singleton

class CommunicationManager {
private:
	static CommunicationManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	CommunicationManager();

public:
	void init();
	static CommunicationManager& getInstance();
};

// theIntance needs to define since it is static
CommunicationManager CommunicationManager::theInstance;