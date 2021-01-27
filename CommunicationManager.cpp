#include "CommunicationManager.h"

CommunicationManager::CommunicationManager() {
	init();
}

// initialize connections between clients and server
void CommunicationManager::init() {
	
}

// get global instance
CommunicationManager& CommunicationManager::getInstance() {
	return theInstance;
}