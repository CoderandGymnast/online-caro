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
#define BUFF_SIZE 1024
#define MAX_ROOMS 2
#define MAX_USERNAME_SIZE 255
#define RESPONSE_SIZE 2048
#define CODE_SIZE 2
#define ROOM_CAPACITY 2

#define BAD_REQUEST "400: Bad Request"
#define OK "200: OK"
#define NOT_FOUND "404: Not Found"

#define STATUS_ATTACHED 0
#define STATUS_LOGGED_IN 1
#define STATUS_CHALLENGING 2
#define STATUS_CHALLENGED 3
#define STATUS_GAMING 4

#define STATUS_OPERATION_CLOSED 0
#define STATUS_OPERATION_OPENED 1
#define STATUS_OPERATION_ACCEPTED 2
#define STATUS_OPERATION_DENIED 3

#define TURN_CHALLENGER 0
#define TURN_COMPETITOR 1

#define STATUS_ROOM_GAMING 0
#define STATUS_ROOM_OUT 1

#define CHALLENGING "30"
#define ACCEPTED "31"

using namespace std;

/* data structure definition: */
struct Competitor {
	char* username;
	SOCKET socket;
};

struct Room {
	int status; // 0: gaming, 1: out.
	int turn; // {0; 1}
	Competitor *challenger;
	Competitor* competitor;
	int* map[3][3];
	int* move[2];
};

struct UserData {
	int status; // 0: , 1: logged in 1: challenging, 2: challenged, 3: gaming.
	char* username;
	SOCKET socket;
	Room* room;
	int operationStatus; // 0: done / nothing, 1: open.
	char* meta;
};

/* function declaration: */
unsigned __stdcall processRequestThread(void* arg);
int attachUserData(SOCKET socket, int* slot);
void worker();
void initLists();
void log(string m);
void toClient(char* m, SOCKET socket);
void processFullSlots(SOCKET socket);
void processRequest(char* m, int i, char* res);
void getCode(char* m, char* code, char* meta);
void processChallengingRequest(char* meta, int i, char* res);
void processRequestNotFound(char* res);
void processChallengingStatus(int i);
void processChallengedStatus(int i);
void processGamingStatus(int i);
int find(char* username);
int isLoggedIn(int i);
void processAcceptingRequest(int i, char* res);
int attachRoom();

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

		int* slot = (int*) malloc(sizeof(int));
		int res = attachUserData(connSock, slot);
		if (!res) {
			processFullSlots(connSock);
			continue;
		}

		/*
		* Create a thread: 1 thread/client.
		* "start_address" & "arglist".
		* (?) initflag: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=msvc-160
		*/
		_beginthreadex(0, 0, processRequestThread, (void*)slot, 0, 0);
	}

	closesocket(listenSock);

	WSACleanup();

	return 0;
}

unsigned __stdcall processRequestThread(void* args) {

	char buff[BUFF_SIZE];
	int ret;
	int* slot = (int*)args;
	UserData* userData = &(userDatas[*slot]);
	SOCKET connSock = (SOCKET)userData->socket;
	char* res = (char*) malloc(RESPONSE_SIZE * sizeof(char));

	bool counter = false; // TEMP.

	while (1) {
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret < 0) printf("Socket '%d' closed\n", connSock);
		else {
			buff[ret] = 0; // NOTE: '\0' (Null terminator, NUL) is a control character with the value 0.
			printf("client socket '%d': '%s'\n", connSock, buff);

			if (userData->status) { // NOTE: Must logged in for further features.
				processRequest(buff, *slot, res);
			}
			
			if (!counter) { // TEMP.
				userData->username = (char*)malloc(MAX_USERNAME_SIZE * sizeof(char));
				strcpy(userData->username, buff);
				userData->status = STATUS_LOGGED_IN;
				string username = userData->username;
				log("account '" + username + "' logged in");
				counter = true;
				strcpy(res, OK);
			}

			log("response: '" + (string)res + "' - size: '" + to_string(strlen(res)) + "'");
			ret = send(connSock, res, strlen(res), 0); // NOTE: could not send too large data.
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
* Add the specified user data to the tracked list.
* @param     userData    [OUT] The specified room.
*
* @return                only return 0 in case slots are full.
*/
int attachUserData(SOCKET socket, int* slot) {
	for (int i = 0; i < ROOM_CAPACITY * MAX_ROOMS; i++) {
		if (!(STATUS_ATTACHED <= userDatas[i].status && userDatas[i].status <= STATUS_GAMING)) {
			userDatas[i].status = STATUS_ATTACHED;
			userDatas[i].socket = socket;
			userDatas[i].operationStatus = STATUS_OPERATION_CLOSED;
			*slot = i;
			log("ID: '" + to_string(i) + "' attached");
			return 1;
		}
	}
	return 0;
}

/*
* Process client request.
* @param     m        [IN] request message.
* @param	 i		  [IN] slot index.
* @param     res      [OUT] response.
*/
void processRequest(char* m, int i, char* res) {

	char* code = (char*)malloc(CODE_SIZE * sizeof(char));
	char* meta = (char*)malloc((strlen(m) - CODE_SIZE) * sizeof(char));
	getCode(m, code, meta);

	if (!strcmp(code, CHALLENGING))	 {
		processChallengingRequest(meta, i, res);
	}
	else if (!strcmp(code, ACCEPTED)) {
		processAcceptingRequest(i, res);
	}
	else {
		processRequestNotFound(res);
	} // TODO: process logout request (free user data).
}

/*
* Process client request.
* @param     res      [OUT] response.
*/
void processRequestNotFound(char* res) {
	strcpy(res, NOT_FOUND);
}

/*
* Proces client challenging request.
* @param     m        [IN] request message.
* @param	 i        [IN] slot index.
* @param     res      [OUT] response.
*/
void processChallengingRequest(char* meta, int i, char* res) {

	UserData* userData = &(userDatas[i]);
	int status = userData->status;
	if (status != STATUS_LOGGED_IN) {
		strcpy(res, BAD_REQUEST);
		log((string)res);
		log("error: could not 'challenging' - user data status '" + to_string(status) + "'");
		return;
	}

	userData->status = STATUS_CHALLENGING;
	userData->operationStatus = 1;
	userData->meta = (char*)malloc(strlen(meta) * sizeof(char));
	strcpy(userData->meta, meta);
	strcpy(res, OK);
	log("'" + (string)userData->username + "' challenging '" + (string)meta + "'");
}

void processAcceptingRequest(int i, char* res) {

	UserData* userData = &(userDatas[i]);
	int status = userData->status;
	if (status != STATUS_CHALLENGED) {
		strcpy(res, BAD_REQUEST);
		log("error: could not 'accepting' - user data status '" + to_string(status) + "'");
		return;
	}

	userData->operationStatus = STATUS_OPERATION_ACCEPTED;
	strcpy(res, OK);
	log("'" + (string)userData->username + "' accepts challenge '" + (string)userData->meta + "'");
}

/*
* Get code from client request.
* @param     m    [IN] request message.
* @param     code [OUT] request code.
*/
void getCode(char* m, char* code, char* meta) {
	int mSize = strlen(m);
	strncpy(code , m, CODE_SIZE);
	strncpy(meta, &(*(m + CODE_SIZE)), mSize - CODE_SIZE);
	code[CODE_SIZE] = 0;
	meta[mSize - CODE_SIZE] = 0;
	log("code: " + (string) code + " - size: " + to_string(sizeof(code)/sizeof(char)));
	log("meta: " + (string)meta + " - size: " + to_string(sizeof(meta) / sizeof(char)));
}

/*
* Check rooms & user data lists to execute operations.
*/
void worker() {
	while (1) {
		for (int i = 0; i < 2 * MAX_ROOMS; i++) {
			if (!userDatas[i].operationStatus) continue;
			if (userDatas[i].status == STATUS_CHALLENGING) {
				processChallengingStatus(i);
			}
			else if (userDatas[i].status == STATUS_CHALLENGED ) {
				processChallengedStatus(i);
			}
			else if (userDatas[i].status == 4) {
				processGamingStatus(i);
			}
		}
	}
} 

/*
* working on challenging user data.
* @param     i    [IN]     the index of user data on the list.
*/
void processChallengingStatus(int i) {

	log("[FIX BUG]: server down");

	UserData* challenger = &(userDatas[i]);

	int res = find(challenger->meta);
	if (res == -1) {
		challenger->status = STATUS_LOGGED_IN; // NOTE: reset user data's status to be able to challenge others.
		log("error: not found '" + (string)challenger->meta + "'");
	}
	else {
		UserData* competitor = &(userDatas[res]);
		if (competitor->status == STATUS_LOGGED_IN) {
			log("competitor: '" + (string)competitor->username + "'");
			competitor->status = STATUS_CHALLENGED;
			competitor->operationStatus = STATUS_OPERATION_OPENED;
			competitor->meta = (char*)malloc(strlen(challenger->username) * sizeof(char));
			strcpy(competitor->meta, challenger->username);
		}
		else { // TODO: Could not challenge itsself because status is changed to STATUS_CHALLENGING.
			challenger->status = 1; // NOTE: reset user data's status to be able to challenge others.
			log("error: could not challenged '" + (string)competitor->username + "'");
		}
	}

	challenger->operationStatus = STATUS_OPERATION_CLOSED;
}

/*
* working on challenged user data.
* @param     i    [IN]     the index of user data on the list.
*/
void processChallengedStatus(int i) {

	UserData* competitor = &(userDatas[i]);
	if (competitor->operationStatus == STATUS_OPERATION_OPENED) {
		printf("[DEBUG]: send '30%s' to client with Socket '%d'\n", competitor->meta, competitor->socket);
	}
	else if (competitor->operationStatus == STATUS_OPERATION_ACCEPTED) {
		int res = attachRoom();
		if (res == -1) {
			log("error: rooms are full");
			printf("[DEBUG]: send '33 - rooms are full' to client with Socket '%d'\n", competitor->socket);
			// TODO: Handle full rooms.
		}
		else {
			Room* room = &(rooms[i]);
			room->status = STATUS_ROOM_GAMING;
			room->turn = TURN_CHALLENGER;
			Competitor* roomCompetitor = (Competitor*)malloc(sizeof(Competitor));
			roomCompetitor->username = (char*)malloc(strlen(competitor->username) * sizeof(char));
			strcmp(roomCompetitor->username, competitor->username);
			roomCompetitor->socket = competitor->socket;
			Competitor* roomChallenger = (Competitor*)malloc(sizeof(Competitor));
			int challengerI = find(competitor->meta);
			if (challengerI == -1) {
				log("error: not found challenger '" + (string)competitor->meta + "'");
				// TODO: Handle more cases (challenger is offline, challenger not challenging anymore,...)
			}
			else {

				UserData* challenger = &(userDatas[challengerI]);
				roomChallenger->username = challenger->username;
				roomChallenger->socket = challenger->socket;
				room->challenger = roomChallenger;
				room->competitor = roomCompetitor;

				competitor->status = STATUS_GAMING;
				challenger->status = STATUS_GAMING;
			}

		}
	}
	else if (competitor->operationStatus == STATUS_OPERATION_DENIED) {

	}
	else {
		log("error: nonsense");
	}
	competitor->operationStatus = STATUS_OPERATION_CLOSED;
}

int attachRoom() {
	for (int i = 0; i < MAX_ROOMS; i++) {
		if (!(0 <= rooms[i].status && rooms[i].status <= 1)) {
			return i;
		}
	}
	return -1;
}

/*
* working on gaming user data.
* @param     i    [IN]     the index of user data on the list.
*/
void processGamingStatus(int i) {

}

/*
* Find user data index (only find logged-in user data).
* @param     username    [IN]     username of user data.
* 
* @return                         -1: not found.
*/
int find(char* username) {
	for (int i = 0; i < 2 * MAX_ROOMS; i++) {
		if (!isLoggedIn(i)) continue;
		if (!strcmp(userDatas[i].username, username)) return i;
	}
	return -1;
}

/*
* check logged in status.
* @param     i    [IN]     the index of user data on the list.
* 
* @return                  0: not logged in, 1: logged in.
*/
int isLoggedIn(int i) {
	return (STATUS_LOGGED_IN <= userDatas[i].status && userDatas[i].status <= STATUS_GAMING) ? 1 : 0;
}

void initLists() {
	userDatas = (UserData*)malloc(2 * MAX_ROOMS * sizeof(UserData));
	rooms = (Room*)malloc(MAX_ROOMS * sizeof(Room));
}

void log(string m) {
	time_t now = time(0);
	char* dt = ctime(&now);
	cout << ++loggerCounter << ". " << dt << m << endl;
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
