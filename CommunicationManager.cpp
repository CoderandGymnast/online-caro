#include "CommunicationManager.h"

// theIntance needs to define since it is static
CommunicationManager CommunicationManager::theInstance;

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