#include "AccountManager.h"

// theIntance needs to define since it is static
AccountManager AccountManager::theInstance;

AccountManager::AccountManager() {
	init();
}

// initialize
void AccountManager::init() {

}

// get global instance
AccountManager& AccountManager::getInstance() {
	return theInstance;
}

void AccountManager::getOnlineAccounts() {

}