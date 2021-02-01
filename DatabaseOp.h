#pragma once

// things to do:

// 1. download xampp server: a development server that already contains mysql server and its admin to 
// watch the database
// link tutorial: https://www.youtube.com/watch?v=3B-CnezwEeo&list=PL4cUxeGkcC9gksOX3Kd9KPo-O68ncT05o&index=2

// 2. start xampp server and start Apache, mysql so the program can start connect to the database
// open mysql admin button --> phpmyadmin --> create a database named: "caro_winsock_server" 
// see more info in the init function()

// 3. when you start running and it gives error ".dll not found", just go to the folder /dependencies/lib ...
// ... copy the that .dll file and place it next to the .exe file in the Debug folder

// 4. click into that database
// then import the .sql file in the dependencies folder

#include <mysql.h>
#include <string>
#include "TableDatas.h"
#include "UserInfo.h"


//singleton
class DatabaseOp {
private:
	static DatabaseOp theInstance;
	MYSQL* connect;

	DatabaseOp();
	void init();
	void executeQuery(std::string query, std::string& errorMsg, TableDatas& datas);
	void testShowAllUsers();

public:
	static DatabaseOp& getInstance();
	void closeConnect();

	// --------- DatabaseOp API instruction in DatabaseOp.cpp file - inside init() function --------------

	// == 0 success or else failed + errorMessage
	int createAccount(std::string username, std::string password, std::string& errorMsg);

	// == 0 success + get table datas (user data)
	// or else failed + errorMessage
	int logIn(std::string username, std::string password, std::string& errorMsg, TableDatas& datas);

	// == 0 update success 
	// or else failed + errorMessage
	int updateUserScore(int userId, int userScore, std::string& errorMsg);

	// get suitable users from the database according to user's score
	void getRankList(int userId, std::string& errorMsg ,TableDatas& datas);
};


