#include "TableDatas.h"
#include <iostream>

using namespace std;

bool TableDatas::isDataEmpty() {
	return this->m_datas.size() == 0;
}

void TableDatas::addRow() {
	std::vector<std::string> tableRow;
	this->m_datas.push_back(tableRow);
}

void TableDatas::addDataInCurrentLastRow(std::string data) {
	this->m_datas[this->m_datas.size() - 1].push_back(data);
}

void TableDatas::showTableDatas() {
	//cout << endl << "----------table datas (testing purpose)-------------" << endl;

	for (int i = 0; i < this->m_datas.size(); i++) {
		for (int j = 0; j < this->m_datas[i].size(); j++) {
			cout << this->m_datas[i][j] << " ";
		}

		cout << endl; // endl every each row
	}
}

string TableDatas::extractChallengeListMessage(int* onlineSchemaIDs, int size) {

	// NOTE: not include self.

	string m = "\n";

	for (int i = 0; i < this->m_datas.size(); i++) {
		if (!isInList(stoi(this->m_datas[i][0]), onlineSchemaIDs, size)) m += "(offline) ";
		else m += "(online) ";
		for (int j = 1; j < this->m_datas[i].size(); j++) {
			m += m_datas[i][j] + " ";
		}

		m += "\n";
	}

	return m;
}

bool TableDatas::isInList(int id, int* onlineSchemaIDs, int size) {
	for (int i = 0; i < size; i++) {
		if (id == onlineSchemaIDs[i]) return true;
	}
	return false;
}


void TableDatas::removeElements(int fromIndex, int toIndex) {
	this->m_datas.erase(this->m_datas.begin() + fromIndex, this->m_datas.begin() + toIndex);
}

void TableDatas::removeAnElement(int index) {
	this->m_datas.erase(this->m_datas.begin() + index);
}

void TableDatas::clearDatas() {
	this->m_datas.clear();
}

UserInfo TableDatas::extractUserFromTableDatas() {

	int id = stoi(this->m_datas[0][0]);
	string username = this->m_datas[0][1];
	int score = stoi(this->m_datas[0][2]);

	return UserInfo(id, username, score);
}
