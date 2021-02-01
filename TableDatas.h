#pragma once

#include <vector>
#include <string>
#include "UserInfo.h"

// ----------- save datas after SELECT query --------------
class TableDatas {

public:
	std::vector<std::vector<std::string>> m_datas;

	bool isDataEmpty();
	void addRow();
	void addDataInCurrentLastRow(std::string data);
	void showTableDatas();
	void clearDatas();
	void removeAnElement(int index);
	void removeElements(int fromIndex, int toIndex);
	// only use this for a table data that has only one row (like logIn())
	UserInfo extractUserFromTableDatas();
	string extractChallengeListMessage();
};
