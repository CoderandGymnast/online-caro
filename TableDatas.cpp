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
	cout << endl << "----------table datas (testing purpose)-------------" << endl;

	for (int i = 0; i < this->m_datas.size(); i++) {
		for (int j = 0; j < this->m_datas[i].size(); j++) {
			cout << this->m_datas[i][j] << " ";
		}

		cout << endl; // endl every each row
	}
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

