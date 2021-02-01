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
#define MOVE_SIZE 2
#define MAP_SIZE 3

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

#define STATUS_MOVE_CLOSED 0
#define STATUS_MOVE_OPENED 1

#define CHALLENGING "30"
#define ACCEPTED "31"

#define MOVE "40"

using namespace std;

/* data structure definition: */
struct Competitor {
	char* username;
	SOCKET lisSock;
	SOCKET socket;
};

struct Room {
	int status; // 0: gaming, 1: out.
	int turn; // {0; 1}
	Competitor *challenger;
	Competitor* competitor;
	int** map;
	int moveStatus; // 0: closed, 1: opened.
	int* move;
	int moveCounter;
};

struct UserData {
	int status; // 0: , 1: logged in 1: challenging, 2: challenged, 3: gaming.
	char* username;
	SOCKET lisSock;
	SOCKET socket;
	Room* room;
	int operationStatus; // 0: done / nothing, 1: open.
	char* meta;
};

/* function declaration: */
unsigned __stdcall processRequestThread(void* arg);
int attachUserData(SOCKET lisSock, SOCKET socket, int* slot);
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
void processMovingRequest(char* meta, int i, char* res);
char* toCharArr(string mess);
void printMap(int i);
int** initMap();
int convertMoveToCoordiates(char* move, int* i, int* j);
int charToDigit(char i);
void debug(string m);

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

	int counter = 0;
	SOCKET cliLisSock;
	while (1) {
		SOCKET connSock;

		/** The accept function permits an incoming connection attempt on a socket. */
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		log("connected socket: " + to_string((int)connSock));

		if (++counter == 1) {
			cliLisSock = connSock;
			continue;
		}

		cout << "[client]: fully-connected client - main socket: '" << (int) connSock << "' - listen socket: '" << (int) cliLisSock << "'" << endl;;
		counter = 0;

		int* slot = (int*) malloc(sizeof(int));
		int res = attachUserData(cliLisSock, connSock, slot);
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
	if (res < 0) log("(error): could not send. Socket closed" + socket);
}

/*
* Add the specified user data to the tracked list.
* @param     userData    [OUT] The specified room.
*
* @return                only return 0 in case slots are full.
*/
int attachUserData(SOCKET cliLisSock, SOCKET socket, int* slot) {
	for (int i = 0; i < ROOM_CAPACITY * MAX_ROOMS; i++) {
		if (!(STATUS_ATTACHED <= userDatas[i].status && userDatas[i].status <= STATUS_GAMING)) {
			userDatas[i].status = STATUS_ATTACHED;
			userDatas[i].lisSock = cliLisSock;
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
	else if (!strcmp(code, MOVE)) {
		processMovingRequest(meta, i, res);
	}
	else {
		processRequestNotFound(res);
	} // TODO: process logout request (free user data).
}

void processMovingRequest(char* meta, int i, char* res) {
	UserData* userData = &(userDatas[i]);
	Room* room = userData->room;
	if (room->status == STATUS_ROOM_GAMING && userData->status == STATUS_GAMING) {
		// TODO: track turn.

		int turn = room->turn;
		char* username = userData->username;

		if (
			!((!strcmp(username, room->challenger->username) && (room->turn == TURN_CHALLENGER)) ||
			(!strcmp(username, room->competitor->username) && (room->turn == TURN_COMPETITOR)))
			) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - not your turn");
			strcpy(res, resMess);
			return;
		}

		int* i = (int*)malloc(sizeof(int));
		int* j = (int*)malloc(sizeof(int));
		int convRes = convertMoveToCoordiates(meta, i, j);

		if (!convRes) {
			char* resMess = toCharArr(BAD_REQUEST + (string) " - invalid move '" + meta + "'");
			strcpy(res, resMess);
		}
		else {
			room->move[0] = *i;
			room->move[1] = *j;
			room->moveStatus = STATUS_MOVE_OPENED;
			strcpy(res, OK);
		}
	}
	else {
		res = (char*) BAD_REQUEST;
		log("error: '" + (string)userData->username + "' not in a room");
	}
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
		char* resMess = toCharArr(BAD_REQUEST + (string)" - cannot 'challenging' while 'waiting'");
		strcpy(res, resMess);
		log((string)res);
		log("error: could not 'challenging' - user data status '" + to_string(status) + "'");
		return;
	}

	if (!strcmp(userData->username, meta)) {
		char* resMess = toCharArr(BAD_REQUEST + (string) " - cannot self-challenging");
		strcpy(res, resMess);
		log((string)resMess);
		log("error: cannot self-challenging");
		return;
	}

	userData->status = STATUS_CHALLENGING;
	userData->operationStatus = 1;
	userData->meta = (char*)malloc(strlen(meta) * sizeof(char));
	strcpy(userData->meta, meta);
	char* resMess = toCharArr(OK + (string) " - waiting '" + meta + "'");
	strcpy(res, resMess);
	log("'" + (string)userData->username + "' challenging '" + (string)meta + "'");
}

char* toCharArr(string mess) {
	int messLength = mess.length();
	char* resMess = (char*)malloc(messLength * sizeof(char));
	strcpy(resMess, mess.c_str());
	return resMess;
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
		for (int i = 0; i < MAX_ROOMS; i++) {
			if (!(STATUS_ROOM_GAMING <= rooms[i].status && rooms[i].status <= STATUS_ROOM_OUT)) continue;
			if (rooms[i].status == STATUS_ROOM_OUT) {
				// TODO: Free room.
			}
			else if (rooms[i].status == STATUS_ROOM_GAMING) {
				if (rooms[i].moveStatus == STATUS_MOVE_CLOSED) continue;
				int turn = rooms[i].turn;
				Competitor* challenger = rooms[i].challenger;
				Competitor* competitor = rooms[i].competitor;
				if (turn == TURN_CHALLENGER) {
					//printf("[DEBUG]: send code:'40' - meta:'%s' to client with Socket '%d'\n", competitor->username, competitor->socket);
					printf("'%s' moves '%d%d'\n", challenger->username, rooms[i].move[0], rooms[i].move[1]);
					rooms[i].turn = TURN_COMPETITOR;
				}
				else if (turn == TURN_COMPETITOR) {
					//printf("[DEBUG]: send code:'40' - meta:'%s' to client with Socket '%d'\n", challenger->username, challenger->socket);
					printf("'%s' moves '%d%d'\n", competitor->username, rooms[i].move[0], rooms[i].move[1]);
					rooms[i].turn = TURN_CHALLENGER;
				}
				else {
					log("error: nonsence turn");
				}

				Room* room = &(rooms[i]);
				room->moveCounter++;
				int** map = room->map;
				map[room->move[0]][room->move[1]] = turn;

				string resMap = "\n\n";
				for (int i = 0; i < MAP_SIZE; i++) {
					for (int j = 0; j < MAP_SIZE; j++) {
						if (map[i][j] == -1) resMap += "- ";
						else if (map[i][j] == TURN_CHALLENGER) resMap += "x ";
						else if (map[i][j] == TURN_COMPETITOR) resMap += "o ";
						else cout << "? ";
					}
					resMap += "\n";
				}
				resMap += "\n";

				debug(resMap);

				char* resMapMess = toCharArr(resMap);

				debug(resMapMess);

				toClient(resMapMess, challenger->lisSock);
				toClient(resMapMess, competitor->lisSock);
				
				if (room->moveCounter == MAP_SIZE * MAP_SIZE) {
					char* resMess = toCharArr((string)"[NOTI]: game over");
					toClient(resMess, challenger->lisSock);
					toClient(resMess, competitor->lisSock);
				}

				rooms[i].moveStatus = STATUS_MOVE_CLOSED;
			}
			else {
				log("error: nonsense room status");
			}
		}
	}
} 

void debug(string m) {
	cout <<"[DEBUG]: " << m << endl;
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

		char* resMess = toCharArr(NOT_FOUND + (string)" - account: '" + (string)challenger->meta + (string)"'");
		log(resMess);
		toClient(resMess, challenger->lisSock);
	}
	else {
		UserData* competitor = &(userDatas[res]);
		if (competitor->status == STATUS_LOGGED_IN) {

			log("competitor: '" + (string)competitor->username + "'");
			competitor->status = STATUS_CHALLENGED;
			competitor->operationStatus = STATUS_OPERATION_OPENED;
			competitor->meta = (char*)malloc(strlen(challenger->username) * sizeof(char));
			strcpy(competitor->meta, challenger->username);

			char* resMess = toCharArr((string)"[NOTI]: '" + challenger->username + "' is challenging");
			toClient(resMess, competitor->lisSock);
		} 
		else {
			challenger->status = 1; // NOTE: reset user data's status to be able to challenge others.
			char* resMess = toCharArr(BAD_REQUEST + (string)" - busy account '" + (string)competitor->username + "'");
			toClient(resMess, challenger->lisSock);
			log(resMess);
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
		//printf("[DEBUG]: send '30%s' to client with Socket '%d'\n", competitor->meta, competitor->socket);
	}
	else if (competitor->operationStatus == STATUS_OPERATION_ACCEPTED) {
		int roomI = attachRoom();
		if (roomI == -1) {
			log("error: rooms are full");
			//printf("[DEBUG]: send '33 - rooms are full' to client with Socket '%d'\n", competitor->socket);
			// TODO: Handle full rooms.
		}
		else {
			Room* room = &(rooms[roomI]);
			room->status = STATUS_ROOM_GAMING;
			room->turn = TURN_CHALLENGER;
			Competitor* roomCompetitor = (Competitor*)malloc(sizeof(Competitor));
			roomCompetitor->username = (char*)malloc(strlen(competitor->username) * sizeof(char));
			strcpy(roomCompetitor->username, competitor->username);
			roomCompetitor->socket = competitor->socket;
			roomCompetitor->lisSock = competitor->lisSock;
			Competitor* roomChallenger = (Competitor*)malloc(sizeof(Competitor));
			int challengerI = find(competitor->meta);
			if (challengerI == -1) {
				log("error: not found challenger '" + (string)competitor->meta + "'");
				// TODO: Handle more cases (challenger is offline, challenger not challenging anymore,...)
			}
			else {

				UserData* challenger = &(userDatas[challengerI]);
				roomChallenger->username = (char*)malloc(strlen(challenger->username) * sizeof(char));
				strcpy(roomChallenger->username, challenger->username);
				roomChallenger->socket = challenger->socket;
				roomChallenger->lisSock = challenger->lisSock;
				room->challenger = roomChallenger;
				room->competitor = roomCompetitor;
				room->moveStatus = STATUS_MOVE_CLOSED;
				room->move = (int*)malloc(MOVE_SIZE*sizeof(int));
				room->moveCounter = 0;

				room->map = initMap();

				competitor->status = STATUS_GAMING;
				challenger->status = STATUS_GAMING;
				challenger->room = room;
				competitor->room = room;

				char* resMess = toCharArr(OK + (string)" - room created - challenger: '" + challenger->username + "' & competitor: '" + competitor->username + "'");
				toClient(resMess, challenger->lisSock);
				toClient(resMess, competitor->lisSock);
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

int convertMoveToCoordiates(char* move, int* i, int* j) {

	if (strlen(move) != 2) return 0;
	for (int n = 0; n < 2; n++) 
		if (!(48 <= int(move[n]) && int(move[n]) <= (48 + MAP_SIZE - 1))) return 0;
	
	*i = charToDigit(move[0]);
	*j = charToDigit(move[1]);

	return 1;
}

int charToDigit(char i) {
	return int(i) - 48;
}

int** initMap() {

	int** map;

	map = new int* [MAP_SIZE];
	for (int i = 0; i < MAP_SIZE; i++) {
		map[i] = new int[MAP_SIZE];
	}


	for (int i = 0; i < MAP_SIZE; i++)
		for (int j = 0; j < MAP_SIZE; j++)
			map[i][j] = -1;

	return map;
}

void printMap(int i) {

	cout << "\n'" << rooms[i].challenger->username << "' vs. '" << rooms[i].competitor->username << ";" << endl;

	int** map = rooms[i].map;
	for (int i = 0; i < MAP_SIZE; i++) {
		for (int j = 0; j < MAP_SIZE; j++) {
			if (map[i][j] == -1) cout << "-" << " ";
			else if (map[i][j] == TURN_CHALLENGER) cout << TURN_CHALLENGER << " ";
			else if (map[i][j] == TURN_COMPETITOR) cout << TURN_COMPETITOR << " ";
			else cout << "? ";
		}
		cout << "\n" << endl;
	}
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
