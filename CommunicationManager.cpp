#include "CommunicationManager.h"

CommunicationManager::CommunicationManager() {
	init();
}

// initialize
void CommunicationManager::init() {
	
}

// get global instance
CommunicationManager& CommunicationManager::getInstance() {
	return theInstance;
}