#pragma once

// things to do:

// 1. download xampp server: a development server that already contains mysql server and its admin to 
// watch the database
// link tutorial: https://www.youtube.com/watch?v=3B-CnezwEeo&list=PL4cUxeGkcC9gksOX3Kd9KPo-O68ncT05o&index=2

// start xampp server and start Apache, mysql so the program can start connect to the database
// open mysql admin button --> phpmyadmin --> create a database named: "caro_winsock_server" 
// see more info in the init function()

// when you start running and it gives error ".dll not found", just go to dependencies/lib ...
// ... copy the that .dll file and place it next to the .exe file in the Debug folder

#include <mysql.h>
#include <string>

//singleton
class DatabaseOp {
private:
	static DatabaseOp theInstance;
	MYSQL* connect;

	// private constructor prevent creating object from outside this class
	// because creating object outside try to access the private constructor --> error
	DatabaseOp();
	void init();
	void executeQuery(MYSQL* connect, std::string query);
	
	// show tables just for testing purpose, please create a table on phpMyAdmin first, 
	// or just download and import a test table into your local database here: ...
	// .. the test import is in the dependencies folder
	void testShowTables();

public:
	static DatabaseOp& getInstance();
	void getOnlineAccounts();
	void closeConnect();
};


