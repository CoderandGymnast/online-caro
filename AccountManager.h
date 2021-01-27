#pragma once

//singleton

class AccountManager {
private:
	static AccountManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	AccountManager();

public:
	void init();
	static AccountManager& getInstance();
	void getOnlineAccounts();
};

// theIntance needs to define since it is static
AccountManager AccountManager::theInstance;
