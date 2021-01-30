#pragma once

//singleton

class AccountManager {
private:
	static AccountManager theInstance;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	AccountManager();
	void init();

public:
	static AccountManager& getInstance();
	void getOnlineAccounts();
};
