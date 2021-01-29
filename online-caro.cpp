// online-caro.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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
#include <thread>
#include <ctime>

#pragma comment (lib, "Ws2_32.lib")

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define MAX_ROOMS 1
#define MAX_USERNAME_SIZE 255
#define RESPONSE_SIZE 2048

using namespace std;

/* data structure definition: */
struct Competitor {
	char* username;
	SOCKET socket;
};

struct Room {
	int status; // 0: gaming, 1: out.
	int turn; // {0; 1}
	Competitor *competitors;
	int* map[3][3];
	int* move[2];
};

struct UserData {
	int status; // 0: no operation. 1: challenging. 2: challenged. 3: gaming.
	char* username;
	SOCKET socket;
	Room* room;
	char* operation[2];
};

/* function declaration: */
void registerRoom(Room* room);
unsigned __stdcall processRequestThread(void* arg);
void initRoom(Competitor* competitors, Room* room);
int initUserData(SOCKET socket, UserData* userData);
int registerUserData(UserData* userData);
void worker();
void initLists();
void log(string message);
void toClient(char* m, SOCKET socket);
void processFullSlots(SOCKET socket);

Room* rooms;
UserData* userDatas;
int loggerCounter = 0;

int main(int argc, TCHAR* argv[]) {	

	initLists();
	thread deamon(worker);

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

		UserData* userData = (UserData*)malloc(sizeof(UserData));
		int res = initUserData(connSock, userData);
		if (!res) {
			processFullSlots(connSock);
			continue;
		}

		/*
		* Create a thread: 1 thread/client.
		* "start_address" & "arglist".
		* (?) initflag: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=msvc-160
		*/
		_beginthreadex(0, 0, processRequestThread, (void*)userData, 0, 0);
	}

	closesocket(listenSock);

	WSACleanup();

	return 0;
}

/*
* process in case slots are full.
*/
void processFullSlots(SOCKET socket) {
	char* m = (char*)"error: slots are full";
	log(m);
	closesocket(socket);
}

/*
* Send message to the specified client.
* @param     m          [IN] message.
* @param     socket     [IN] socket related to the client.
*/
void toClient(char* m, SOCKET socket) {
	int res = send(socket, m, strlen(m), 0);
	if (res < 0) log("could not send. Socket closed" + socket);
}

/*
* Create & register user data.
* @param     socket     [IN] socket related to user data.
* @param     userData   [OUT] initiated user data.
* 
* @return               only return 0 in case slots are full.
*/
int initUserData(SOCKET socket, UserData* userData) {
	userData->status = 0;
	userData->socket = socket;
	return registerUserData(userData);
}

/*
* Add the specified user data to the tracked list.
* @param     userData    [OUT] The specified room.
* 
* @return                only return 0 in case slots are full.
*/
int registerUserData(UserData* userData) {
	for (int i = 0; i < 2*MAX_ROOMS; i++) {
		if (userDatas[i].status != 0 && 1 && 2) {
			userDatas[i] = *userData;
			log("register user ID: " + to_string(i));
			return 1;
		}
	}
	return 0;
}

unsigned __stdcall processRequestThread(void* args) {

	char buff[BUFF_SIZE]; // "buff" is a pointer.
	int ret;
	UserData* userData = (UserData*)args;
	SOCKET connSock = (SOCKET)userData->socket;

	bool counter = false; // TEMP.

	while (1) {
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret < 0) printf("Socket '%d' closed\n", connSock);
		else {
			buff[ret] = 0; // NOTE: '\0' (Null terminator, NUL) is a control character with the value 0.
			cout << "from client: " << buff << endl;
			
			if (!counter) { // TEMP.
				userData->username = (char*)malloc(MAX_USERNAME_SIZE * sizeof(char));
				strcpy(userData->username, buff);
				counter = true;
			}

			cout << "username: " << userData->username << endl;
			cout << "status: " << userData->status;

			char* response = (char*)malloc(RESPONSE_SIZE * sizeof(char));
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

void processRequest() {

}

/*
* Add the specified room to the tracked list.
* @param     room    [OUT] The specified room.
*/
void registerRoom(Room* room) {
	for (int i = 0; i < MAX_ROOMS; i++) {
		if (rooms[i].status != 0 & 1) { // NOTE: rooms[i] is an actual value. &rooms[i] is address of rooms[i] in memory space (RAM).
			rooms[i] = *room; // NOTE: Assign value pointed by "room" pointer to rooms[i].
			return;
		}
	}
}

/*
* Create & register room 
* @param     players     [IN] room players.
* @param     sockets     [IN] sockets related to room players.
* @param     room        [OUT] initiated room.
*/
void initRoom(Competitor* competitors, Room* room) {
	room->competitors = competitors;
	registerRoom(room);
}

/*
* Check rooms & user data lists to execute operations.
*/
void worker() {
	while (1) {
		Sleep(100);
		for (int i = 0; i < 2 * MAX_ROOMS; i++) {
		}
	}
}

void initLists() {
	userDatas = (UserData*)malloc(2 * MAX_ROOMS * sizeof(UserData));
	rooms = (Room*)malloc(MAX_ROOMS * sizeof(Room));
}

void log(string message) {
	time_t now = time(0);
	char* dt = ctime(&now);
	cout << ++loggerCounter << ". " << dt << message  << endl;
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
