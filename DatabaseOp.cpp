#include "DatabaseOp.h"
#include <iostream>

using namespace std;

// this config is default for mysql server on xampp on the first start
#define SERVER "localhost"
#define USER "root"
#define PASSWORD ""
#define DATABASE "caro_winsock_server"
#define MYSQL_PORT 3306 // on your xampp program and look at mysql port

// theIntance needs to define since it is static
DatabaseOp DatabaseOp::theInstance;

DatabaseOp::DatabaseOp() {
	cout << "databaseOp object initing" << endl; // testing if the program really call this function
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

		// do something rather than return -----
		// code ...
		// -------------------------------------
		return;
	}

	// test this function if it create a test table on your database successfully
	this->testShowTables();
}

void DatabaseOp::executeQuery(MYSQL* connect, string query) {

	MYSQL_RES* res_set; //Result set object to store output table from the query
	MYSQL_ROW row; // row variable to process each row from the result set.

	mysql_query(connect, query.c_str());  // execute the query, returns Zero for success. Nonzero if an error occurred. details at https://dev.mysql.com/doc/refman/5.7/en/mysql-query.html

	res_set = mysql_store_result(connect); //reads the entire result of a query, allocates a MYSQL_RES structure, details at: https://dev.mysql.com/doc/refman/5.7/en/mysql-store-result.html
	int num_rows = mysql_num_rows(res_set); // get number of rows in output table/ result set
	int num_col = mysql_num_fields(res_set); // get number of columns
	int i = 0;

	while (((row = mysql_fetch_row(res_set)) != NULL)) // fetch each row one by one from the result set
	{
		i = 0;
		while (i < num_col) { // print every row
			cout << row[i] << " ";  //cells are separated by space
			i++;
		}
		cout << endl; // print a new line after printing a row
	}
}

void DatabaseOp::testShowTables() {
	cout << "showing all tables in database " << DATABASE << endl;
	this->executeQuery(this->connect, "SELECT * FROM test_table");
}

void DatabaseOp::getOnlineAccounts() {

}

void DatabaseOp::closeConnect() {
	mysql_close(this->connect);
}