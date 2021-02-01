#include "DatabaseOp.h"
#include <typeinfo>
#include <iostream>

using namespace std;

// this config is default for mysql server on xampp on the first start
#define SERVER "localhost"
#define USER "root"
#define PASSWORD ""
#define DATABASE "caro_winsock_server"
#define MYSQL_PORT 3306				// on your xampp program and look at mysql port
#define USER_LIST_TABLE "user_list" // the only table the project is connecting to

// in a rank list, only get 4 higher and 4 lower than the current level  
#define RANK_LIST_LEVEL_LIMIT 4     

// theIntance needs to define since it is static
DatabaseOp DatabaseOp::theInstance;

DatabaseOp::DatabaseOp() {
	init();
}

// get global instance
DatabaseOp& DatabaseOp::getInstance() {
	return theInstance;
}
 
// initialize database connection
void DatabaseOp::init() {
	this->connect = mysql_init(NULL);  // initialize object

	if (!this->connect) {		// check if connect is properly initialized
		cout << "mysql initialization failed" << endl;
		return;
	}

	this->connect = mysql_real_connect(this->connect, SERVER, USER, PASSWORD, DATABASE, MYSQL_PORT,NULL, 0);

	if (this->connect) // check if the connection was successful, 
	{
		cout << "Connection Succeeded\n";
	}
	else
	{
		cout << "Connection Failed\n";
		return;
	}

	// ------------------- how to use DatabaseOp API --------------------------
	string test_errorMsg;
	TableDatas test_m_tableDatas;
	UserInfo test_user = UserInfo(1, "hoang1", 10);

	testShowAllUsers();

	/*
	if (createAccount("hoang4", "123456", errorMsg) != 0) {
		cout << "errorMsg: " << errorMsg << endl;
	}

	*/

	/*
	if (logIn("hoang2", "123456", test_errorMsg, test_m_tableDatas) != 0) {
		cout << "errorMsg: " << test_errorMsg << endl;
	}
	else {
		cout << "log in successful" << endl;
		test_m_tableDatas.showTableDatas();
	}
	*/

	/*
	if (updateUserScore(test_user.id, test_user.score + 3, test_errorMsg) != 0) {
		cout << test_errorMsg << endl;
	}
	else {
		cout << "update user success" << endl;
	}
	*/

	/*
	this->getRankList(35, test_errorMsg, test_m_tableDatas);
	if (test_errorMsg != "") {
		cout << test_errorMsg << endl;
	}
	else {
		test_m_tableDatas.showTableDatas();
	}
	*/
}

void DatabaseOp::testShowAllUsers() {
	cout << "testing showing users list table " << DATABASE << endl;
	TableDatas test_m_datas;
	string test_errorMsg;
	this->executeQuery("SELECT * FROM user_list", test_errorMsg, test_m_datas);

	test_m_datas.showTableDatas();
}

void DatabaseOp::closeConnect() {
	mysql_close(this->connect);
}

int DatabaseOp::createAccount(string username, string password, std::string& errorMsg) {
	string createQuery = "INSERT INTO `" + (string)USER_LIST_TABLE + "` (`username`, `password`) VALUES " 
						  + (string)"(" + (string)"\"" + username + "\"" 
						  + ","
						  + "\"" + password + "\"" + ")";

	TableDatas emptyObj;
	this->executeQuery(createQuery, errorMsg, emptyObj);

	if (errorMsg != "") {
		return 1;
	}

	return 0;
}

int DatabaseOp::logIn(string username, string password, std::string& errorMsg, TableDatas& datas) {

	string logInQuery = "SELECT `id`, `username`, `score` FROM " + (string)"`" + USER_LIST_TABLE + "`" 
						+ " WHERE `username` = " + "\"" + username + "\""
						+ " AND `password` = " + "\"" + password + "\"";

	this->executeQuery(logInQuery, errorMsg, datas);

	if (errorMsg != "" && datas.isDataEmpty()) {		// user not exist
		errorMsg = "user doesn't exist";
		return 1;
	}

	return 0;
}

void DatabaseOp::getRankList(int userId, string& errorMsg,TableDatas& datas) {
	string getRankListQuery = "SELECT `id`, `username`, `score`" + 
							  (string)" FROM `user_list` " + 
							  "ORDER BY `score` DESC";

	this->executeQuery(getRankListQuery, errorMsg, datas);

	// push the unnecessary datas out of rank list according to rank limit
	int currentRowIndex = -1;

	// finding current row index in the table datas that match the current user id
	for (int i = 0; i < datas.m_datas.size(); i++) {
		if (stoi(datas.m_datas[i][0]) == userId) {
			currentRowIndex = i;
		}
	}

	// currentRowIndex should never be -1
	if (currentRowIndex == -1) {
		errorMsg = "something went wrong";
		return;
	}

	// rank limit get higher levels --> pop other unecessary datas
	if (currentRowIndex - (int)RANK_LIST_LEVEL_LIMIT <= 0) {
		// do nothing
	}
	else {
		datas.removeElements(0, currentRowIndex - (int)RANK_LIST_LEVEL_LIMIT);
	}

	// rank limit get lower levels --> pop other unnecessary datas
	// since the datas has been changed --> recalculate currentRowIndex
	for (int i = 0; i < datas.m_datas.size(); i++) {
		if (stoi(datas.m_datas[i][0]) == userId) {
			currentRowIndex = i;
		}
	}

	if (currentRowIndex + (int)RANK_LIST_LEVEL_LIMIT >= datas.m_datas.size() - 1) {
		// do nothing
	}
	else {
		datas.removeElements(currentRowIndex + (int)RANK_LIST_LEVEL_LIMIT + 1, datas.m_datas.size());
	}

	// since the datas has been changed --> recalculate currentRowIndex
	for (int i = 0; i < datas.m_datas.size(); i++) {
		if (stoi(datas.m_datas[i][0]) == userId) {
			currentRowIndex = i;
		}
	}

	datas.removeAnElement(currentRowIndex);
}

int DatabaseOp::updateUserScore(int userId, int userScore, string& errorMsg) { 

	string updateQuery = "UPDATE `" + (string)USER_LIST_TABLE + "`" + 
						 "SET `score`= " + to_string(userScore) + 
						 " WHERE `id` = " + to_string(userId);
	
	TableDatas m_datas;

	this->executeQuery(updateQuery, errorMsg, m_datas);

	if (errorMsg != "") {
		return 1;
	}

	return 0;
}

void DatabaseOp::executeQuery(string query, std::string& errorMsg, TableDatas& datas) {
	// initalize errors
	errorMsg = "";
	datas.clearDatas();

	MYSQL_RES* res_set; //Result set object to store output table from the query
	MYSQL_ROW row;		// row variable to process each row from the result set.
	
	mysql_query(this->connect, query.c_str());  // execute the query, returns Zero for success. Nonzero if an error occurred. details at https://dev.mysql.com/doc/refman/5.7/en/mysql-query.html

	if (mysql_errno(this->connect) != 0) {
		cerr << "[SQL_ERROR_TESTING]:" << mysql_error(this->connect) << endl;
		
		errorMsg = mysql_error(this->connect);

		if (errorMsg.find("Duplicate") != string::npos &&
			errorMsg.find("username") != string::npos) {

			errorMsg = "username already exist";
		}

		return;
	}

	res_set = mysql_store_result(this->connect); // reads the entire result of a query, allocates a MYSQL_RES structure, details at: https://dev.mysql.com/doc/refman/5.7/en/mysql-store-result.html
	
	if (res_set == NULL) {		// res_set == NULL when it's a CREATE QUERY or DELETE query
								// -> there will be no data when data create successfully
		return;
	}

	int num_rows = mysql_num_rows(res_set);
	int num_col = mysql_num_fields(res_set);
	int i = 0;

	if (num_rows == 0) {
		// there is no data in the QUERY
		return;
	}

	while (((row = mysql_fetch_row(res_set)) != NULL)) // fetch each row one by one from the result set
	{
		datas.addRow();

		i = 0;
		while (i < num_col) { // every cell in a row
			// cout << row[i] << " ";
			datas.addDataInCurrentLastRow(row[i]);
			i++;
		}
		cout << endl;
	}
}