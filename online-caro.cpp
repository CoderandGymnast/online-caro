// online-caro.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/* TCPServer.cpp : Defines the entry point for the console application. */

/** Doc: https://users.soict.hust.edu.vn/tungbt/it4060q/Lec03.IOMode.pdf */

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#undef UNICODE

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h> 
#include <string>
#include <iostream>
#include <process.h> /* Contains multi-thread APIs. */
#include "DatabaseOp.h"

#pragma comment (lib, "Ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

#define USERNAME_OPERATION '1'
#define PASSWORD_OPERATION '2'
#define LOGOUT_OPERATION '3'

#define USERNAME_SUCCESS "10"
#define USERNAME_NOT_FOUND "11"
#define ACCOUNT_IS_BLOCKED "12"
#define ACCOUNT_IS_LOGGED_IN "13"
#define PASSWORD_SUCCESS "20"
#define PASSWORD_INCORRECT "21"
#define LOGOUT_SUCCESS "30"

#define STATUS_DEFAULT 0
#define STATUS_LOGGED_IN 1
#define STATUS_BLOCKED 2

#define RESPONSE_SIZE 3
#define DATA_MAX_SIZE 1024

struct Account {
	char* username;
	char* password;
	int status;
	Account* next;
	int counter;
};

bool isValidRequest(char* args, int* totalChars, int* totalDigits);
void splitCharsAndDigits(char* args, char* chars, char* digits);
void writeAccountsToBuffer();
void writeAccountsToBuffer();
void printBuffer();
void convertRecordToEntity(char record[], Account* account);
bool processRequest(char* request, char* response, char* state);
void processUsername(char* data, char* response, char* state);
void processPassword(char* data, char* response, char* state);
void processLogout(char* response, char* state);
bool processBlockAccount(char* state);
unsigned __stdcall processRequestThread(void* arg);

Account* accountDB;
#define DB_PATH "./account.txt"

int main(int argc, TCHAR* argv[]) {

	// database object
	DatabaseOp& m_database = DatabaseOp::getInstance();

	writeAccountsToBuffer();
	printBuffer();

	u_short SERVER_PORT = atoi(argv[1]);

	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);

	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	SOCKET listenSock;

	/*
	* Params: Address family, Socket type, Protocol.
	* AF_INET: The Internet Protocol version 4 (IPv4) address family.
	* Reference: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	*/
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);


	printf("Server TCP/IP network byte order (big-endian) port: '%d'\n", serverAddr.sin_port);
	printf("Server address family: '%d'\n", serverAddr.sin_family);
	/** Associates a local address with a socket. */
	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Error! Cannot bind this address.\n");
		_getch();
		return 0;
	}

	/** The listen function places a socket in a state in which it is listening for an incoming connection. */
	if (listen(listenSock, 10)) {
		printf("Error! Cannot listen.\n");
		_getch();
		return 0;
	}

	printf("Server is running...\n");

	sockaddr_in clientAddr;
	char buff[BUFF_SIZE];
	int ret, clientAddrLen = sizeof(clientAddr);

	while (1) {
		SOCKET connSock;

		/** The accept function permits an incoming connection attempt on a socket. */
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		printf("Connected socket: %d\n", connSock);

		/*
		* Create a thread: 1 thread/client.
		* "start_address" & "arglist".
		* (?) initflag: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=msvc-160
		*/
		_beginthreadex(0, 0, processRequestThread, (void*)connSock, 0, 0);
	}

	closesocket(listenSock);

	WSACleanup();

	// close database connection
	m_database.closeConnect();

	return 0;
}

/*
* Validate request from client; Count total characters and total digits.
* @param     request     [IN] client request.
* @param     totalChars     [OUT] Total characters
* @param     totalDigits     [OUT] Total digits.
* @return     bool     only return true in case request is valid.
*/
bool isValidRequest(char* request, int* totalChars, int* totalDigits) {

	char* args = request;

	for (int i = 0; i < strlen(args); i++) {
		if ((int('A') <= int(toupper(args[i])) && int(toupper(args[i])) <= int('Z'))) *totalChars = *totalChars + 1;
		else if ((int('0') <= int(args[i]) && int(args[i]) <= int('9'))) *totalDigits = *totalDigits + 1;
		else return false;
	}

	return true;
}

/*
* Split the arguments into an array of characters and an array of digits.
* @param     args    [IN] the string arguments.
* @param     chars     [OUT] the array of characters.
* @param     chars     [OUT] the array of digits.
*/
void splitCharsAndDigits(char* args, char* chars, char* digits) {
	int charsCounter = 0, numsCounter = 0;
	for (int i = 0; i < strlen(args); i++) {
		if ((int('A') <= int(toupper(args[i])) && int(toupper(args[i])) <= int('Z'))) {
			chars[charsCounter] = args[i];
			charsCounter++;
		}
		else if ((int('0') <= int(args[i]) && int(args[i]) <= int('9'))) {
			digits[numsCounter] = args[i];
			numsCounter++;
		}
	}
}

/*
* Convert record read from the file "account.txt" to the standard entity.
* @param     record     [IN] The record read from the file "account.txt".
* @param     account     [OUT] The standard entity.
*/
void convertRecordToEntity(char record[], Account* account) {

	const int FIELD_NUMBER = 3;

	const char delimiter[2] = " ";
	char** buffer = (char**)malloc(FIELD_NUMBER * sizeof(char*));
	int index = 0;
	buffer[index] = strtok(record, delimiter);
	while (buffer[index++] != NULL) {
		buffer[index] = strtok(NULL, delimiter);
	}
	account->username = (char*)malloc(DATA_MAX_SIZE * sizeof(char));
	account->password = (char*)malloc(DATA_MAX_SIZE * sizeof(char));

	strcpy(account->username, buffer[0]);
	strcpy(account->password, buffer[1]);
	account->status = int(buffer[2][0]) - 48; /* Convert from character to integer. */
	account->next = NULL;
	account->counter = 0; /* Default value of all counters. */
}

/*
* Print all accounts stored on the buffer to the console.
*/
void printBuffer() {

	Account* node = accountDB;

	printf("\nDATABASE:\n\n");
	while (node != NULL) {
		printf("Username: %s\n", node->username);
		printf("Password: %s\n", node->password);
		printf("Status: %d\n\n", node->status);
		node = node->next;
	}
}

/*
* Read accounts from the file "account.txt" and write them to a linked-list buffer.
*/
void writeAccountsToBuffer() {

	accountDB = (Account*)malloc(sizeof(Account));

	const int MAX_RECORD_LENGTH = 1024;
	char record[MAX_RECORD_LENGTH];
	Account* node = (Account*)malloc(sizeof(Account));

	FILE* file;
	file = fopen(DB_PATH, "r");
	if (!file) printf("[Error]: Could not access the database (this is a test database using file / disable this later - in main() line 238).\n");

	bool isFirstRecord = true;
	Account* account;

	while (fgets(record, MAX_RECORD_LENGTH, file) != NULL) {
		account = (Account*)malloc(sizeof(Account));
		convertRecordToEntity(record, account);

		if (isFirstRecord) {
			node = account;
			isFirstRecord = false;
			accountDB = node;
		}
		else {
			node->next = account;
			node = node->next;
		}
	}

	fclose(file);
}

/*
* Get the operation code from the request.
* @param     request     The client's request.
*/
char getOperationCode(char* request) {
	return request[0];
}

/*
* Get data from the client's request.
* @param     request    [IN] The client's request.
* @param     data    [OUT] The data detached from the client's request.
*/
void getData(char* request, char* data) {
	for (int i = 0; i < strlen(request); i++) {
		data[i] = request[i + 1];
	}
}

/*
* Process the request from the client.
* @param     request   [IN] The client's request.
* @param     response    [OUT] The response from this server.
*/
bool processRequest(char* request, char* response, char* state) {

	char code = getOperationCode(request);
	char* data = (char*)malloc(sizeof(char));
	getData(request, data);

	switch (code) {
	case USERNAME_OPERATION:
		processUsername(data, response, state);
		return true;
	case PASSWORD_OPERATION:
		processPassword(data, response, state);
		return true;
	case LOGOUT_OPERATION:
		processLogout(response, state);
		return true;
	default:
		printf("[WARNING]: Client out\n");
		processLogout(response, state);
		return false;
	}
}

/*
* Process the username request from the client.
* @param     data     [IN] The data received from the client.
* @param     response    [OUT] The response from this server.
*/
void processUsername(char* data, char* response, char* state) {
	printf("Received username: %s\n", data);
	strcpy(state, data);
	Account* node = accountDB;
	while (node != NULL) {
		if (strcmp(node->username, data) == 0) {
			if (node->status == STATUS_BLOCKED) {
				printf("[ERROR]: Account ID '%s' is blocked\n", state);
				strcpy(response, (char*)ACCOUNT_IS_BLOCKED);
				return;
			}
			else if (node->status == STATUS_LOGGED_IN) {
				printf("[ERROR]: Account ID '%s' is already logged in\m", state);
				strcpy(response, (char*)ACCOUNT_IS_LOGGED_IN);
				return;
			}
			printf("[RESPONSE]: Username success - ID '%s'\n", state);
			strcpy(response, (char*)USERNAME_SUCCESS);
			return;
		}
		node = node->next;
	}
	printf("[RESPONSE]: Username not found - ID '%s'\n", state);
	strcpy(response, (char*)USERNAME_NOT_FOUND);
}

/*
* Process the password request from the client.
* @param     data     [IN] The data received from the client.
* @param     response     [OUT] The response from this server.
*/
void processPassword(char* data, char* response, char* state) {
	printf("Received password '%s' from ID '%s'\n", data, state);
	Account* node = accountDB;
	while (node != NULL) {
		if (strcmp(node->username, state) == 0) {
			if (strcmp(node->password, data) == 0) {
				node->status = STATUS_LOGGED_IN;
				strcpy(response, (char*)PASSWORD_SUCCESS);
				printf("[RESPONSE]: Password success - ID '%s'\n\n", state);
			}
			else {
				printf("[RESPONSE]: Password incorrect - ID '%s'\n", state);
				if (processBlockAccount(state)) strcpy(response, (char*)ACCOUNT_IS_BLOCKED);
				else strcpy(response, (char*)PASSWORD_INCORRECT);
			}
			return;
		}
		node = node->next;
	}
}

/*
* Process the 'Block' state of the current account.
*/
bool processBlockAccount(char* state) {
	Account* node = accountDB;
	while (node != NULL) {
		if (strcmp(node->username, state) == 0) {
			if (++node->counter == 3) {
				node->status = STATUS_BLOCKED;
				printf("[WARNING]: Account ID '%s' is blocked\n", state);
				return true;
			}
			else {
				printf("[WARNING]: Incorrect entered password counter '%d' - ID '%s'\n", node->counter, state);
				break;
			}
		}
		node = node->next;
	}
	return false;
}

/*
* Process the log out request from client.
* @param     response    [OUT] The response from this server.
*/
void processLogout(char* response, char* state) {
	Account* node = accountDB;
	while (node != NULL) {
		if (strcmp(node->username, state) == 0) {
			node->status = STATUS_DEFAULT;
		}
		node = node->next;
	}
	printf("[RESPONSE]: Logout success - ID '%s'\n\n", state);
	free(state);
	state = (char*)malloc(DATA_MAX_SIZE * sizeof(char));
	strcpy(response, (char*)LOGOUT_SUCCESS);
}

unsigned __stdcall processRequestThread(void* arg) {

	char buff[BUFF_SIZE];
	int ret;
	SOCKET connSock = (SOCKET)arg;
	bool processStatus;
	char* state = (char*)malloc(DATA_MAX_SIZE * sizeof(char));

	while (1) {
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret < 0) printf("Socket '%d' closed\n", connSock);
		else {
			buff[ret] = 0; /* '\0': Null terminator (NUL) is a control character with the value 0. */
			char* response = (char*)malloc(RESPONSE_SIZE * sizeof(char));
			processStatus = processRequest(buff, response, state);
			if (!processStatus) break;
			ret = send(connSock, response, ret, 0);
			if (ret < 0) {
				if (ret < 0) printf("Socket '%d' closed\n", connSock);
			}
		}
	}

	closesocket(connSock);
	printf("Connection closed - Socket '%d' \n", connSock);
	return 0;
}









/*
* [ERRORS]:
* 1. warning C6269: Possibly incorrect order of operations: dereference ignored.
* Solution: Change "*x++" to "*x = *x + 1".
*
* 2. file: minkernel\crts\ucrt\inc\corecrt_internal_strtox.h Line:1772 Expression: _p != nullptr
* Reason: Missing the second argument which is the port number.
*/

/** [NOTES]:
 * The relative path of the "account.txt" file must be relative to the location of the application file.
 * Should use "Debugger" to determine the error. Should use "argv[1] = (TCHAR *) "5500";" to prevent "Expression: _p != nullptr" error.
 * Must use "x[]" (const char*) as input of "strtok (String token)" function. Could not re-assign to a const char variable but could re-assign each position in the array.
 * Null-terminated string is a character string stored as an array containing the characters and terminated with a null character ('\0', call NUL in ASCII).
 * Incorrect memory allocation for pointers causes a server crash.
 * Work with null pointer causes a server crash, i.e., print, assign, "String copy" function.
 * Print the pointed address of a pointer would result "CDCDCDCD".
 */




// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
