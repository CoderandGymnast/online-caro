#include "ResultManager.h"


// theIntance needs to define since it is static
ResultManager ResultManager::theInstance;

ResultManager::ResultManager() {
	init();
}

// initialize
void ResultManager::init() {

}

// get global instance
ResultManager& ResultManager::getInstance() {
	return theInstance;
}

void ResultManager::getRankList() {

}