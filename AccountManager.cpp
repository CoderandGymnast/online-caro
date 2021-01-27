#include "AccountManager.h"

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