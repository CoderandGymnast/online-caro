#include "MatchManager.h"


// theIntance needs to define since it is static
MatchManager MatchManager::theInstance;

MatchManager::MatchManager() {
	init();
}

// initialize
void MatchManager::init() {

}

// get global instance
MatchManager& MatchManager::getInstance() {
	return theInstance;
}

void MatchManager::createRoom() {

}

void MatchManager::manageScore() {

}