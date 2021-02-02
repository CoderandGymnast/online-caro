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
#include "DatabaseOp.h"
#include "UserInfo.h"

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
#define INTERNAL "500: Internal Error"

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
#define STATUS_ROOM_OVER 1
#define STATUS_ROOM_SURRENDER 2

#define STATUS_MOVE_CLOSED 0
#define STATUS_MOVE_OPENED 1

#define REGISTRATION "00"
#define LOG_IN "10"
#define LOG_OUT "11"
#define GET_CHALLENGE_LIST "20" // NOTE: always up-to-date because get directly from DB.
#define GET_SCORE "21"
#define CHALLENGING "30"
#define ACCEPTED "31"
#define DENIED "32"

#define MOVE "40"
#define SURRENDER "41"

using namespace std;

/* data structure definition: */
struct Competitor {
	int schemaID;
	int score;
	char* username;
	SOCKET lisSock;
	SOCKET socket;
};

struct Room {
	int status; // 0: gaming, 1: over.
	int turn; // {0; 1}
	Competitor* challenger;
	Competitor* competitor;
	int** map;
	int moveStatus; // 0: closed, 1: opened.
	int* move;
	int moveCounter;
	char* meta; // stored surrender.
	char* his;
};

struct UserData {
	int schemaID;
	int score;
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
int checkResult(int** map);
void processUnauthenticatedRequest(char* code, char* meta, char* res, int i);
void processGetChallengeList(int i, char* res);
int* getOnlineSchemaIDs(int* counter);
int updateScore(int schemaID, int updatedScore, char* resMess);
void processLogOut(int i, char* res);
void processClientTerminated(int i);
void processDeniedRequest(int i,  char* res);
void processSurrenderRequest(int i, char* res);
void processGetScore(int i, char* res);

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

		cout << "[client]: fully-connected client - main socket: '" << (int)connSock << "' - listen socket: '" << (int)cliLisSock << "'" << endl;;
		counter = 0;

		int* slot = (int*)malloc(sizeof(int));
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
	char* res = (char*)malloc(RESPONSE_SIZE * sizeof(char));

	while (1) {
		ret = recv(connSock, buff, BUFF_SIZE, 0);
		if (ret < 0) printf("Socket '%d' closed\n", connSock);
		else {
			buff[ret] = 0; // NOTE: '\0' (Null terminator, NUL) is a control character with the value 0.
			printf("client socket '%d': '%s'\n", connSock, buff);

			if (!strcmp(buff, "")) {
				processClientTerminated(*slot);
				break;
			}

			processRequest(buff, *slot, res);

			log("response: '" + (string)res + "' - size: '" + to_string(strlen(res)) + "'");
			ret = send(connSock, res, strlen(res), 0); // NOTE: could not send too large data.
			if (ret < 0) {
				if (ret < 0) printf("Socket '%d' closed\n", connSock);
			}
		}
	}

	// closesocket(connSock); // NOTE: processClientTerminated handles this.
	printf("Connection closed - Socket '%d' \n", connSock);
	return 0;
}

void processClientTerminated(int i) {
	UserData* userData = &(userDatas[i]);
	closesocket(userData->socket);
	closesocket(userData->lisSock);
	userData->status = -1; // NOTE: unknown user data status.
	userData->username = (char*)malloc(sizeof(char));
	strcpy_s(userData->username, 1, ""); // NOTE: if userData->username is not allocated, this will cause server crash.
	userData->schemaID = -1;
	userData->score = 0;
	userData->operationStatus = STATUS_OPERATION_CLOSED;
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

	if (strlen(m) < 2) {
		strcpy(res, toCharArr(BAD_REQUEST + (string)" - invalid request"));
		return;
	}

	char* code = (char*)malloc(CODE_SIZE * sizeof(char));
	char* meta = (char*)malloc((strlen(m) - CODE_SIZE) * sizeof(char));
	getCode(m, code, meta);

	UserData* userData = &(userDatas[i]);

	if (!userData->status) { // NOTE: only process in case user data is not logged in (0).
		processUnauthenticatedRequest(code, meta, res, i);
		return; // NOTE: must log in to process further.
	}

	if (!strcmp(code, LOG_OUT)) {
		processLogOut(i, res);
	} else if (!strcmp(code, CHALLENGING)) {
		processChallengingRequest(meta, i, res);
	}
	else if (!strcmp(code, ACCEPTED)) {
		processAcceptingRequest(i, res);
	}
	else if (!strcmp(code, DENIED)) {
		processDeniedRequest(i, res);
	}
	else if (!strcmp(code, MOVE)) {
		processMovingRequest(meta, i, res);
	}
	else if (!strcmp(code, SURRENDER)) {
		processSurrenderRequest(i, res);
	}
	else if (!strcmp(code, GET_CHALLENGE_LIST)) {
		processGetChallengeList(i, res);
	}
	else if (!strcmp(code, GET_SCORE)) {
		processGetScore(i, res);
	}
	else {
		processRequestNotFound(res);
	} // TODO: process logout request (free user data).
}

void processGetScore(int i, char* res) {
	UserData* userData = &(userDatas[i]);
	strcpy(res, toCharArr(OK + (string) " - score: '" + to_string(userData->score) + "'"));
}

void processSurrenderRequest(int i, char* res) {
	UserData* userData = &(userDatas[i]);
	Room* room = userData->room;
	if (room->status == STATUS_ROOM_GAMING && userData->status == STATUS_GAMING) {
		room->status = STATUS_ROOM_SURRENDER;
		room->meta = (char*)malloc(strlen(userData->username) * sizeof(char));
		strcpy(room->meta, userData->username);
		strcpy(res, toCharArr(OK + (string)" - processing surrender..."));
	}
	else if (room->status == STATUS_ROOM_OVER) {
		char* resMess = toCharArr(BAD_REQUEST + (string)" - match over");
		strcpy(res, resMess);
	}
	else {
		strcpy(res, toCharArr(BAD_REQUEST + (string)+" - not in a room"));
		log("error: '" + (string)userData->username + "' not in a room");
	}
}

void processDeniedRequest(int i, char* res) {

	UserData* userData = &(userDatas[i]);

	if (userData->status != STATUS_CHALLENGED) {
		strcpy(res, toCharArr(BAD_REQUEST + (string)" - you are not being challenged"));
		return;
	}

	userData->operationStatus = STATUS_OPERATION_DENIED;

	strcpy(res, toCharArr(OK));
}

void processLogOut(int i, char* res) {

	UserData* userData = &(userDatas[i]);
	userData->status = STATUS_ATTACHED;
	strcpy_s(userData->username, 1, "");
	userData->schemaID = -1; // NOTE: -1 does not exist on DB.
	userData->score = 0;
	userData->operationStatus = STATUS_OPERATION_CLOSED;

	strcpy(res, toCharArr(OK + (string)" - logged out")); // NOTE: do not need strcpy_s in this situation.
}

void processGetChallengeList(int i, char* res) {

	UserData* userData = &(userDatas[i]);
	int schemaID = userData->schemaID;
	string errMess;
	TableDatas schemas;
	DatabaseOp::getInstance().getRankList(schemaID, errMess, schemas);
	if (errMess != "") {
		char* resMess = toCharArr(INTERNAL + errMess);
		strcpy(res, resMess);
		return;
	}
	else {
		// schemas.showTableDatas();
		int* counter = (int*) malloc(sizeof(int));
		*counter = 0;
		int* onlineSchemaIDs = getOnlineSchemaIDs(counter);
		char* resMess = toCharArr(schemas.extractChallengeListMessage(onlineSchemaIDs, *counter)); // TODO: construct response.
		strcpy(res, resMess);
	}

}

int* getOnlineSchemaIDs(int* counter) {

	for (int i = 0; i < 2 * MAX_ROOMS; i++) {
		if (STATUS_LOGGED_IN <= userDatas[i].status && userDatas[i].status <= STATUS_GAMING) {
			(*counter)++;
		}
	}

	int* schemaIDs = (int*)malloc((*counter) * sizeof(int));
	for (int i = 0; i < (*counter); i++) {
		schemaIDs[i] = userDatas[i].schemaID;
	}
	return schemaIDs;
}

void processUnauthenticatedRequest(char* code, char* meta, char* res, int i) {

	// NOTE: message format "<LOG_IN><username>-<password>"

	if (!strcmp(code, REGISTRATION)) {

		if (!strlen(meta)) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing credentials");
			strcpy(res, resMess);
			return;
		}

		int dashCounter = 0;
		for (int i = 0; i < strlen(meta); i++) {
			if (meta[i] == '-') {
				dashCounter++;
				if (dashCounter > 1) {
					char* resMess = toCharArr(BAD_REQUEST + (string)" - invalid credentials");
					strcpy(res, resMess);
					return;
				}
			}
		}

		int dashPos = -1;
		for (int i = 0; i < strlen(meta); i++) {
			if (meta[i] == '-') {
				dashPos = i;
				break;
			}
		}

		if (dashPos == -1) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing password");
			strcpy(res, resMess);
			return;
		}

		string username = ((string)meta).substr(0, dashPos);
		string password = ((string)meta).substr(dashPos + 1, strlen(meta));

		if (!strlen(toCharArr(password))) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing password");
			strcpy(res, resMess);
			return;
		}

		string errMess;

		if (DatabaseOp::getInstance().createAccount(username, password, errMess) != 0) {
			char* resMess = toCharArr(INTERNAL + (string)+" - " + errMess);
			strcpy(res, resMess);
			return;
		}
		else {
			char* resMess = toCharArr(OK + (string)" - created account: username '" + username + "', password: '" + password + "'");
			strcpy(res, resMess);
			return;
		}
	} else if (strcmp(code, LOG_IN)) {
		char* resMess = toCharArr(BAD_REQUEST + (string)" - unauthenticated");
		strcpy(res, resMess);
		return;
	}
	else {

		if (!strlen(meta)) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing credentials");
			strcpy(res, resMess);
			return;
		}

		int dashCounter = 0;
		for (int i = 0; i < strlen(meta); i++) {
			if (meta[i] == '-') {
				dashCounter++;
				if (dashCounter > 1) {
					char* resMess = toCharArr(BAD_REQUEST + (string)" - invalid credentials");
					strcpy(res, resMess);
					return;
				}
			}
		}

		int dashPos = -1;
		for (int i = 0; i < strlen(meta); i++) {
			if (meta[i] == '-') {
				dashPos = i;
				break;
			}
		}

		if (dashPos == -1) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing password");
			strcpy(res, resMess);
			return;
		}

		string username = ((string)meta).substr(0, dashPos);		
		string password = ((string)meta).substr(dashPos + 1, strlen(meta));

		if (!strlen(toCharArr(password))) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - missing password");
			strcpy(res, resMess);
			return;
		}

		// NOTE: check whether the acocunt already logged in.
		if (find(toCharArr(username)) != -1) {
			strcpy(res, toCharArr(BAD_REQUEST + (string)" - account '" + username + "' already logged in"));
			return;
		}

		TableDatas schemas;
		string errMess;
		if (DatabaseOp::getInstance().logIn(username, password, errMess, schemas) != 0) {
			char* resMess = toCharArr(INTERNAL + (string) + " - " + errMess);
			strcpy(res, resMess);
		}
		else { // NOTE: main socket & listen socket is assigned at connection.
			UserInfo user =  schemas.extractUserFromTableDatas();
			int schemaID = user.id;
			UserData* userData = &(userDatas[i]);
			userData->schemaID = schemaID;
			userData->score = user.score;
			userData->status = STATUS_LOGGED_IN;
			userData->username = (char*)malloc(strlen(toCharArr(username))*sizeof(char));
			strcpy(userData->username, toCharArr(username));
			userData->operationStatus = STATUS_OPERATION_CLOSED;
			char* resMess = toCharArr(OK + (string)" - logged in");
			strcpy(res, resMess);
		}

		return;
	}
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
			char* resMess = toCharArr(BAD_REQUEST + (string)" - invalid move '" + meta + "'");
			strcpy(res, resMess);
		}
		else if (room->map[*i][*j] == TURN_COMPETITOR || room->map[*i][*j] == TURN_CHALLENGER) {
			char* resMess = toCharArr(BAD_REQUEST + (string)" - position taken: '" + meta + "'");
			strcpy(res, resMess);
		}
		else {
			room->move[0] = *i;
			room->move[1] = *j;
			room->moveStatus = STATUS_MOVE_OPENED;
			strcpy(res, OK);
		}
	}
	else if (room->status == STATUS_ROOM_OVER) {
		char* resMess = toCharArr(BAD_REQUEST + (string)" - match over");
		strcpy(res, resMess);
	}
	else {
		strcpy(res, toCharArr(BAD_REQUEST + (string)+" - not in a room"));
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
		char* resMess = toCharArr(BAD_REQUEST + (string)" - cannot self-challenging");
		strcpy(res, resMess);
		log((string)resMess);
		log("error: cannot self-challenging");
		return;
	}

	userData->status = STATUS_CHALLENGING;
	userData->operationStatus = 1;
	userData->meta = (char*)malloc(strlen(meta) * sizeof(char));
	strcpy(userData->meta, meta);
	char* resMess = toCharArr(OK + (string)" - waiting '" + meta + "'");
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
	strncpy(code, m, CODE_SIZE);
	strncpy(meta, &(*(m + CODE_SIZE)), mSize - CODE_SIZE);
	code[CODE_SIZE] = 0;
	meta[mSize - CODE_SIZE] = 0;
	log("code: " + (string)code + " - size: " + to_string(sizeof(code) / sizeof(char)));
	log("meta: " + (string)meta + " - size: " + to_string(sizeof(meta) / sizeof(char)));
}

/*
* Check rooms & user data lists to execute operations.
*/
void worker() { // NOTE: worker can not have return.
	while (1) {
		for (int i = 0; i < 2 * MAX_ROOMS; i++) {
			if (!userDatas[i].operationStatus) continue;
			if (userDatas[i].status == STATUS_CHALLENGING) {
				processChallengingStatus(i);
			}
			else if (userDatas[i].status == STATUS_CHALLENGED) {
				processChallengedStatus(i);
			}
			else if (userDatas[i].status == 4) {
				processGamingStatus(i);
			}
		}
		for (int i = 0; i < MAX_ROOMS; i++) {
			if (!(STATUS_ROOM_GAMING <= rooms[i].status && rooms[i].status <= STATUS_ROOM_SURRENDER)) continue;
			if (rooms[i].status == STATUS_ROOM_SURRENDER) {

				Room* room = &(rooms[i]);
				room->status = STATUS_ROOM_OVER; // NOTE: to free room.
				Competitor* challenger = rooms[i].challenger;
				Competitor* competitor = rooms[i].competitor;

				int result;
				debug("Server Crash Preventation");
				if (!strcmp(room->meta, challenger->username)) {
					result = TURN_COMPETITOR;
				}
				else {
					result = TURN_CHALLENGER;
				}

				if (result == TURN_CHALLENGER) {
					string winnerMess = (string)"[NOTI]: winner '" + challenger->username + "'";
					string loserMess = (string)"[NOTI]: loser '" + competitor->username + "'";
					toClient(toCharArr(winnerMess), challenger->lisSock);
					toClient(toCharArr(loserMess), competitor->lisSock);
					char* winnerUpdateResMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					challenger->score += 3;
					int winnerUpdateResult = updateScore(challenger->schemaID, challenger->score, winnerUpdateResMess);
					if (!winnerUpdateResult)toClient(toCharArr(INTERNAL + (string)+" - " + winnerUpdateResMess), competitor->lisSock);
					competitor->score -= 3;
					char* loserUpdateMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int loserUpdateResult = updateScore(competitor->schemaID, competitor->score, loserUpdateMess);
					if (!loserUpdateResult)toClient(toCharArr(INTERNAL + (string)+" - " + loserUpdateMess), competitor->lisSock);
					room->status = STATUS_ROOM_OVER;
					char* tempHis = toCharArr((string)room->his + (string)"\n\n + Winner: '" + challenger->username + "'");
					room->his = tempHis;
				}
				else if (result == TURN_COMPETITOR) {
					string winnerMess = (string)"[NOTI]: winner '" + competitor->username + "'";
					string loserMess = (string)"[NOTI]: loser '" + challenger->username + "'";
					toClient(toCharArr(winnerMess), competitor->lisSock);
					toClient(toCharArr(loserMess), challenger->lisSock);
					competitor->score += 3;
					char* winnerUpdateResMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int winnerUpdateResult = updateScore(competitor->schemaID, competitor->score, winnerUpdateResMess);
					if (!winnerUpdateResult)toClient(toCharArr(INTERNAL + (string)+" - " + winnerUpdateResMess), competitor->lisSock);
					challenger->score -= 3;
					char* loserUpdateMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int loserUpdateResult = updateScore(challenger->schemaID, challenger->score, loserUpdateMess);
					if (!loserUpdateResult)toClient(toCharArr(INTERNAL + (string)+" - " + loserUpdateMess), competitor->lisSock);
					room->status = STATUS_ROOM_OVER;
					char* tempHis = toCharArr((string)room->his + (string)"\n\n + Winner: '" + competitor->username + "'");
					room->his = tempHis;
				}

			} else if (rooms[i].status == STATUS_ROOM_OVER) {

				Room* room = &(rooms[i]);
				room->status = -1;
				toClient(room->his, room->challenger->lisSock);
				toClient(room->his, room->competitor->lisSock);
				char* challengerName = room->challenger->username;
				char* competitorName = room->competitor->username;
				int challengerI = find(challengerName);

				if (challengerI == -1) {
					// NOTE: client terminates or logout.
				}
				else {
					UserData* challenger = &(userDatas[challengerI]);
					challenger->score = room->challenger->score;
					challenger->status = STATUS_LOGGED_IN;
					challenger->room = (Room*)malloc(sizeof(Room)); // NOTE: fake room.
					challenger->operationStatus = STATUS_OPERATION_CLOSED;
					strcpy(challenger->meta, "");
				}

				int competitorI = find(competitorName);

				if (competitorI == -1) {
					// NOTE: client terminates or logout.
				}
				else {
					UserData* competitor = &(userDatas[competitorI]);
					competitor->score = room->competitor->score;
					competitor->status = STATUS_LOGGED_IN;
					competitor->room = (Room*)malloc(sizeof(Room)); // NOTE: fake room.
					competitor->operationStatus = STATUS_OPERATION_CLOSED;
					strcpy(competitor->meta, "");
				}
			}
			else if (rooms[i].status == STATUS_ROOM_GAMING) {
				if (rooms[i].moveStatus == STATUS_MOVE_CLOSED) continue;
				int turn = rooms[i].turn;
				Competitor* challenger = rooms[i].challenger;
				Competitor* competitor = rooms[i].competitor;
				if (turn == TURN_CHALLENGER) {
					//printf("[DEBUG]:d send code:'40' - meta:'%s' to client with Socket '%d'\n", competitor->username, competitor->socket);
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
						if (map[i][j] == -9) resMap += "- ";
						else if (map[i][j] == TURN_CHALLENGER) resMap += "x ";
						else if (map[i][j] == TURN_COMPETITOR) resMap += "o ";
						else cout << "? ";
					}
					resMap += "\n";
				}
				resMap += "\n";

				char* resMapMess = toCharArr(resMap);

				toClient(resMapMess, challenger->lisSock);
				toClient(resMapMess, competitor->lisSock);

				rooms[i].moveStatus = STATUS_MOVE_CLOSED;

				time_t now = time(0);
				char* dt = ctime(&now);

				char* tempHis;
				if (turn == TURN_CHALLENGER) {
					tempHis = toCharArr((string)room->his + "\n" + dt + (string)" - Challenger: " + to_string(room->move[0]) + to_string(room->move[1]));
				}
				else {
					tempHis = toCharArr((string)room->his + "\n" + dt + (string)" - Competitor: " + to_string(room->move[0]) + to_string(room->move[1]));
				}
				room->his = tempHis;

				int matchResult = checkResult(map);
				if (matchResult == TURN_CHALLENGER) {
					string winnerMess = (string)"[NOTI]: winner '" + challenger->username + "'";
					string loserMess = (string)"[NOTI]: loser '" + competitor->username + "'";
					toClient(toCharArr(winnerMess), challenger->lisSock);
					toClient(toCharArr(loserMess), competitor->lisSock);
					char* winnerUpdateResMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					challenger->score += 3;
					int winnerUpdateResult = updateScore(challenger->schemaID, challenger->score, winnerUpdateResMess);
					if (!winnerUpdateResult)toClient(toCharArr(INTERNAL + (string) +" - " + winnerUpdateResMess), competitor->lisSock);
					competitor->score -= 3;
					char* loserUpdateMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int loserUpdateResult = updateScore(competitor->schemaID, competitor->score, loserUpdateMess);
					if (!loserUpdateResult)toClient(toCharArr(INTERNAL + (string) + " - " + loserUpdateMess), competitor->lisSock);
					room->status = STATUS_ROOM_OVER;
					tempHis = toCharArr((string)room->his + (string)"\n\n + Winner: '" + challenger->username + "'" );
					room->his = tempHis;
				}
				else if (matchResult == TURN_COMPETITOR) {
					string winnerMess = (string)"[NOTI]: winner '" + competitor->username + "'";
					string loserMess = (string)"[NOTI]: loser '" + challenger->username + "'";
					toClient(toCharArr(winnerMess), competitor->lisSock);
					toClient(toCharArr(loserMess), challenger->lisSock);
					competitor->score += 3;
					char* winnerUpdateResMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int winnerUpdateResult = updateScore(competitor->schemaID, competitor->score, winnerUpdateResMess);
					if (!winnerUpdateResult)toClient(toCharArr(INTERNAL + (string) +" - " + winnerUpdateResMess), competitor->lisSock);
					challenger->score -= 3;
					char* loserUpdateMess = (char*)malloc(BUFF_SIZE * sizeof(char));
					int loserUpdateResult = updateScore(challenger->schemaID, challenger->score, loserUpdateMess);
					if (!loserUpdateResult)toClient(toCharArr(INTERNAL + (string) +" - " + loserUpdateMess), competitor->lisSock);
					room->status = STATUS_ROOM_OVER;
					tempHis = toCharArr((string)room->his + (string)"\n\n + Winner: '" + competitor->username + "'");
					room->his = tempHis;
				}else if (room->moveCounter == MAP_SIZE * MAP_SIZE) {
					char* resMess = toCharArr((string)"[NOTI]: tie game");
					toClient(resMess, challenger->lisSock);
					toClient(resMess, competitor->lisSock);
					room->status = STATUS_ROOM_OVER;
					tempHis = toCharArr((string)room->his + (string)"\n\nResult: tie game");
					room->his = tempHis;
				}
			}
			else {
				log("error: nonsense room status");
			}
		}
	}
}

int updateScore(int schemaID, int updatedScore, char* resMess) {

	string errMess;
	if (DatabaseOp::getInstance().updateUserScore(schemaID, updatedScore, errMess) != 0) {
		strcpy(resMess, toCharArr(errMess));
		return 0;
	}
	else {
		return 1;
	}
}

int checkResult(int** map) {

	int sum = 0;

	for (int i = 0; i < MAP_SIZE; i++) {
		for (int j = 0; j < MAP_SIZE; j++) {
			sum += map[i][j];
		}
		if (sum == 0) return 0;
		else if (sum == 3) return 1;
		else sum = 0;
	}

	for (int i = 0; i < MAP_SIZE; i++) {
		for (int j = 0; j < MAP_SIZE; j++) {
			sum += map[j][i];
		}
		if (sum == 0) return 0;
		else if (sum == 3) return 1;
		else sum = 0;
	}

	sum = map[0][0] + map[1][1] + map[2][2];

	if (sum == 0) return 0;
	else if (sum == 3) return 1;

	sum = map[0][2] + map[1][1] + map[2][0];

	if (sum == 0) return 0;
	else if (sum == 3) return 1;

	return -1;
}

void debug(string m) {
	cout << "[DEBUG]: " << m << endl;
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

		char* resMess = toCharArr(NOT_FOUND + (string)" - account: '" + (string)challenger->meta + (string)"' offline & not exist.");
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
			roomCompetitor->schemaID = competitor->schemaID;
			roomCompetitor->score = competitor->score;
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
			else { // NOTE: challenger's operation status = closed at challengingStatus.

				UserData* challenger = &(userDatas[challengerI]);
				roomChallenger->schemaID = challenger->schemaID;
				roomChallenger->score = challenger->score;
				roomChallenger->username = (char*)malloc(strlen(challenger->username) * sizeof(char));
				strcpy(roomChallenger->username, challenger->username);
				roomChallenger->socket = challenger->socket;
				roomChallenger->lisSock = challenger->lisSock;
				room->challenger = roomChallenger;
				room->competitor = roomCompetitor;
				room->moveStatus = STATUS_MOVE_CLOSED;
				room->move = (int*)malloc(MOVE_SIZE * sizeof(int));
				room->moveCounter = 0;

				room->map = initMap();
				room->his = (char*) malloc(sizeof(char));// NOTE: init history.
				strcpy(room->his, "H");

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
		competitor->status = STATUS_LOGGED_IN;
		int challengerI = find(competitor->meta);
		if (challengerI == -1) {
			// TODO: Handle more cases (challenger is offline, challenger not challenging anymore,...)
		}
		else {
			UserData* challenger = &(userDatas[challengerI]);
			challenger->status = STATUS_LOGGED_IN;
			strcpy(challenger->meta, "");
			strcpy(competitor->meta, "");
			toClient(toCharArr(OK + (string)" - denied '" + challenger->username + "'"), competitor->lisSock);
			toClient(toCharArr((string)"[NOTI]: challenge denied '" + competitor->username + "'"), challenger->lisSock);
		}
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
			map[i][j] = -9;

	return map;
}

void printMap(int i) {

	cout << "\n'" << rooms[i].challenger->username << "' vs. '" << rooms[i].competitor->username << ";" << endl;

	int** map = rooms[i].map;
	for (int i = 0; i < MAP_SIZE; i++) {
		for (int j = 0; j < MAP_SIZE; j++) {
			if (map[i][j] == -9) cout << "-" << " ";
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
