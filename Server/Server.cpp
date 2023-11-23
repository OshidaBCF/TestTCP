#include <iostream>
#include "Zone.h"
#include <mutex>
#include <string>
#include <vector>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <processthreadsapi.h>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

#define EVENT_FOR_MAIN     (WM_USER + 1)
#define EVENT_FOR_GAME     (WM_USER + 2)
#define EVENT_FOR_WEB      (WM_USER + 3)
#define GAME_SOCKET_EVENT  (WM_USER + 4)
#define WEB_SOCKET_EVENT   (WM_USER + 5)

string webVariableString;
mutex webVariableStringMutex;

void AddToWebString(string str)
{
	webVariableStringMutex.lock();
	webVariableString += str;
	webVariableStringMutex.unlock();
}

vector<zone> zones;
int currentPainter = zone::painterList::CIRCLE;

void clientHandler(WPARAM wParam);
void webClientHandler(WPARAM wParam);

string pseudo1;
string pseudo2;

struct ClientData {
	SOCKET clientSocket;
	int painter;

	ClientData(SOCKET socket, int painter) : clientSocket(socket), painter(painter) {}
};

std::vector<ClientData> clients;
std::vector<ClientData> webClients;

void sendClients(string input)
{
	for (int i = 0; i < clients.size(); i++)
	{
		send(clients[i].clientSocket, input.c_str(), input.length(), 0);
	}
}

// Implémentation de la fonction de gestion des événements
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	SOCKET Accept;
	switch (uMsg) {
	case WM_CLOSE:
		// Gérer l'événement de fermeture de la fenêtre
		MessageBox(NULL, L"Fermeture de la fenêtre cachée.", L"Événement", MB_ICONINFORMATION);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		// Gérer la destruction de la fenêtre
		PostQuitMessage(0);
		break;
	case GAME_SOCKET_EVENT:
	{
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
		{
			if ((Accept = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				closesocket(wParam);
				WSACleanup();
				break;
			}
			WSAAsyncSelect(Accept, hwnd, GAME_SOCKET_EVENT, FD_READ | FD_WRITE | FD_CLOSE);
			string responce = "";
			if (clients.size() == 0) {
				cout << "Player 1 connected!" << endl;
				responce += "Q1";
				clients.emplace_back(Accept, zone::painterList::CIRCLE);
			}
			else if (clients.size() == 1) {
				cout << "Player  2 connected!" << endl;
				responce += "Q2";
				clients.emplace_back(Accept, zone::painterList::CROSS);
			}
			else {
				cout << "Spectator connected!" << endl;
				responce += "Q0";
				clients.emplace_back(Accept, zone::painterList::NONE);
			}
			responce += "S";
			for (int j = 0; j < 3; j++)
			{
				for (int i = 0; i < 3; i++)
				{
					responce += to_string(zones[i + j * 3].painter);
				}
			}
			cout << responce << endl;
			send(Accept, responce.c_str(), responce.size(), 0);

		}
		break;
		case FD_READ:
		{
			clientHandler(wParam);
		}
		break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			break;
		}
	}
	break;
	case WEB_SOCKET_EVENT:
	{
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
		{
			if ((Accept = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				closesocket(wParam);
				WSACleanup();
				break;
			}
			WSAAsyncSelect(Accept, hwnd, WEB_SOCKET_EVENT, FD_READ | FD_WRITE | FD_CLOSE);
			// cout << "Web Client connected!" << endl;
			webClients.emplace_back(Accept, zone::painterList::NONE);
		}
		break;
		case FD_READ:
		{
			webClientHandler(wParam);
		}
		break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			break;
		}
	}
	break;
	default:
		// Laisser les autres messages être gérés par la procédure par défaut
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

bool initializeGameServer(SOCKET& listeningSocket, sockaddr_in& hint, int port, HWND hWnd) {
	// Initialisation de Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Can't Initialize winsock! Quitting...\n";
		return 1;
	}

	// Création du socket
	listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting...\n";
		return false;
	}

	// Configuration du socket
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	// Liaison du socket
	if (bind(listeningSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
		cerr << "Can't bind to IP/port! Quitting...\n";
		closesocket(listeningSocket);
		return false;
	}

	// Mise en écoute du socket
	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "Can't listen on socket! Quitting...\n";
		closesocket(listeningSocket);
		return false;
	}

	WSAAsyncSelect(listeningSocket, hWnd, GAME_SOCKET_EVENT, FD_ACCEPT | FD_CLOSE);

	string output = "Game Server initialized and listening on port " + to_string(port) + "\n";
	cout << output.c_str();
	return true;
}

string checkWinner() {
	// Vérification du gagnant selon les règles du jeu
	// Vérifiez si l'une des combinaisons gagnantes est remplie
	int winner = 0;
	
	// 0,1,2  3,4,5  6,7,8  0,3,6  1,4,7  2,5,8  0,4,8  2,4,6
	int winConditions[8][3] = {{0,1,2}, {3,4,5}, {6,7,8}, {0,3,6}, {1,4,7}, {2,5,8}, {0,4,8}, {2,4,6}};

	for (int i = 0; i < 8; i++)
	{
		int zone0 = winConditions[i][0];
		int zone1 = winConditions[i][1];
		int zone2 = winConditions[i][2];

		if (zones[zone0].painter != 0 && zones[zone0].painter == zones[zone1].painter && zones[zone0].painter == zones[zone2].painter) {
			// S'il y a un gagnant
			winner = zones[zone0].painter; // Assumons que le gagnant est le joueur de la zone[0]

			// Envoi d'un message au client indiquant le gagnant
			if (winner == zone::painterList::CIRCLE) {
				cout << pseudo1 +" wins!\n";
				AddToWebString("Player " + pseudo1 + " Win!\n");
				return "W1"; // Message indiquant la victoire du cercle
			}
			if (winner == zone::painterList::CROSS) {
				cout << pseudo2 + " wins!\n";
				AddToWebString("Player " + pseudo2 + " Win!\n");
				return "W2"; // Message indiquant la victoire des croix
			}
		}


	}
	for (int i = 0; i < 9; i++)
	{
		if (zones[i].painter == 0)
			return "";
	}
	cout << "It's a draw!\n";
	AddToWebString("It's a draw!\n");
	return "W0";
}

void handleMove(ClientData& clientData, char* buf, std::vector<zone> &zones) {
	SOCKET clientSocket = clientData.clientSocket;
	int painter = clientData.painter;

	string responce;
	Vector2i position;
	if (buf[0] == 'P') {
		if (pseudo1 == "" || pseudo2 == "")
		{
			// Envoyer un message indiquant que c'est au tour de l'autre joueur
			string message = "N0";
			send(clientSocket, message.c_str(), message.size(), 0);
			cout << "You cannot play yet, second player hasn't joined: " << currentPainter << endl;
			return;
		}
		if (int(buf[1]) - '0' == currentPainter) {
			position = Vector2i(int(buf[3]) - '0', int(buf[5]) - '0');
			string userInput = "P" + to_string(currentPainter) + "X" + to_string(position.x) + "Y" + to_string(position.y);

			AddToWebString("Player " + to_string(currentPainter) + " played on : " + "X = " + to_string(position.x) + " Y = " + to_string(position.y) + "\n");

			zones[position.x + position.y * 3].painter = currentPainter;
			responce = "S";
			for (int j = 0; j < 3; j++)
			{
				for (int i = 0; i < 3; i++)
				{
					responce += to_string(zones[i + j * 3].painter);
				}
			}

			// Vérification du gagnant après chaque mouvement
			responce += checkWinner();
			cout << responce << endl;
			sendClients(responce);

			// Changement de joueur après un mouvement valide
			if (currentPainter == zone::painterList::CIRCLE) {
				currentPainter = zone::painterList::CROSS;
			}
			else {
				currentPainter = zone::painterList::CIRCLE;
			}
		}
		else {
			// Envoyer un message indiquant que c'est au tour de l'autre joueur
			string message = "N" + to_string(currentPainter);
			send(clientSocket, message.c_str(), message.size(), 0);
			cout << "Not your turn, current player: " << currentPainter << endl;
		}
	}
}

void clientHandler(WPARAM wParam) {
	for (int i = 0; i < clients.size(); i++)
	{
		if (clients[i].clientSocket != (SOCKET)wParam)
		{
			continue;
		}
		SOCKET socket = clients[i].clientSocket;
		int painter = clients[i].painter;

		char buf[4096];
		ZeroMemory(buf, sizeof(buf));

		int byteReceived = recv(socket, buf, sizeof(buf), 0);
		if (byteReceived == SOCKET_ERROR) {
			cerr << "Error in recv(). Quitting\n";
			break;
		}

		if (byteReceived == 0) {
			cout << "Client is disconnected!" << endl;
			clients.erase(clients.begin() + i);
			break;
		}

		// Traitement des données reçues du client
		cout << "Received from client: " << buf << "\n";

		if (buf[0] == 'M')
		{
			string message;
			message = "";
			for (int i = 2; i < byteReceived; i++)
			{
				if (int(buf[1]) - '0' == zone::painterList::CIRCLE)
					pseudo1 += buf[i];
				else
					pseudo2 += buf[i];
			}

			if (int(buf[1]) - '0' == zone::painterList::CIRCLE)
				AddToWebString("Player 1 connected as " + pseudo1 + "!\n");
			else
				AddToWebString("Player 2 connected as " + pseudo2 + "!\n");

			message = "M" + to_string(painter) + pseudo1 + "\rM" + to_string(painter) + pseudo2;
			sendClients(message);
		}

		// Appeler la fonction pour gérer les mouvements du joueur
		handleMove(clients[i], buf, zones);
	}

	return;
}

// Fonction pour le serveur principal
DWORD WINAPI serverMain(LPVOID lpParam) {
	// Creation de la window class
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Définir la classe de la fenêtre
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"MyHiddenWindowClass";

	// Enregistrer la classe de fenêtre
	RegisterClass(&windowClass);

	// Créer la fenêtre cachée
	HWND hiddenWindow = CreateWindowEx(
		0,                              // Styles étendus
		L"MyHiddenWindowClass",        // Nom de la classe
		L"MyHiddenWindow",              // Titre de la fenêtre
		WS_OVERLAPPEDWINDOW,// Style de la fenêtre (fenêtre cachée)
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	// Vérifier si la fenêtre a été créée avec succès
	if (hiddenWindow == NULL) {
		MessageBox(NULL, L"Erreur lors de la création de la fenêtre cachée.", L"Erreur", MB_ICONERROR);
		return 1;
	}

	// Afficher la fenêtre cachée (si nécessaire)
	// ShowWindow(hiddenWindow, SW_SHOWNORMAL);

	// cout << "Server Main thread running...\n";
	
	// Déclaration des variables pour le serveur
	SOCKET listening;
	sockaddr_in hint;
	char buf[4096];
	ZeroMemory(buf, sizeof(buf));

	// Initialisation du serveur
	if (!initializeGameServer(listening, hint, 5004, hiddenWindow)) {
		return 0; // Quitter le thread en cas d'échec de l'initialisation
	}

	int painter = zone::painterList::CIRCLE;
	int winner = 0;

	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			zone newZone(Vector2i(i * 300, j * 300));
			zones.push_back(newZone);
		}
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Fermeture du serveur
	closesocket(listening);
	WSACleanup();

	return 0;
}

bool initializeWebServer(SOCKET& listeningSocket, sockaddr_in& hint, int port, HWND hWnd) {
	// Initialisation de Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Can't Initialize winsock! Quitting...\n";
		return 1;
	}

	// Création du socket
	listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting...\n";
		return false;
	}

	// Configuration du socket
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	// Liaison du socket
	if (bind(listeningSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
		cerr << "Can't bind to IP/port! Quitting...\n";
		closesocket(listeningSocket);
		return false;
	}

	// Mise en écoute du socket
	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "Can't listen on socket! Quitting...\n";
		closesocket(listeningSocket);
		return false;
	}

	WSAAsyncSelect(listeningSocket, hWnd, WEB_SOCKET_EVENT, FD_ACCEPT | FD_CLOSE);

	string output = "Web Server initialized and listening on port " + to_string(port) + "\n";
	cout << output.c_str();
	return true;
}

void webClientHandler(WPARAM wParam)
{
	SOCKET clientWebSocket = (SOCKET)wParam;

	char buf[4096];
	ZeroMemory(buf, sizeof(buf));

	int byteReceived = recv(clientWebSocket, buf, sizeof(buf), 0);
	if (byteReceived == SOCKET_ERROR) {
		cerr << "Error in recv(). Quitting\n";
		return;
	}

	if (byteReceived == 0) {
		cout << "Client is disconnected!" << endl;
		return;
	}

	// cout << "Received from WebServer : " << buf << endl;

	webVariableStringMutex.lock();
	string temp;
	string webVar;
	for (int i = 0; i < webVariableString.size(); i++)
	{
		if (webVariableString[i] != '\n') {
			// Append the char to the temp string.
			temp += webVariableString[i];
		}
		else {
			webVar += "<div>" + temp +"</div>";
			temp.clear();
		}
	}
	webVariableStringMutex.unlock();
	// Message à afficher dans la fenêtre web
	string webMessage = R"(
    <html>
		<head>
			<title>Game Server</title>
			<meta http-equiv="refresh" content="1" />
			<style>
				body {
					background-color: lightgray;
					display: flex;
					flex-direction: column;
					align-items: center;
					height: 100vh;
					margin: 0;
					font-family: Arial, sans-serif;
				}
				header {
					background-color: #333;
					color: white;
					padding: 15px;
					text-align: center;
					width: 100%;
				}
				.message {
					background-color: white;
					padding: 20px;
					border-radius: 5px;
					box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
					margin-top: 20px;
				}
				.chat-box {
					width: 700px;
					height: 800px;
					background-color: #fff;
					border: 1px solid #ccc;
					border-radius: 8px;
					overflow: hidden;
					box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
				}
				.messagesBox {
					height: 700px;
					overflow-y: scroll;
					padding: 10px;
				}
			</style>
		</head>
		<body>
			<header>
				<h1>TicTacToe Online</h1>
			</header>
			<div><h2>Coups jou&#233;s</h2></div>
			<div class="chat-box">
				<div class="messagesBox">)" + webVar + R"(				</div>
			</div>
		</body>
	</html>
	)";

	// Répondre à la requête avec un message HTML
	string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	response += webMessage;

	send(clientWebSocket, response.c_str(), response.size(), 0);
	closesocket(clientWebSocket);
	for (int i = 0; i < webClients.size(); i++)
	{
		if (webClients[i].clientSocket == clientWebSocket)
		{
			webClients.erase(webClients.begin() + i);
		}
	}
}

// Fonction pour le serveur web
DWORD WINAPI webServer(LPVOID lpParam) {
	// Creation de la window class
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Définir la classe de la fenêtre
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"MyHiddenWindowClass";

	// Enregistrer la classe de fenêtre
	RegisterClass(&windowClass);

	// Créer la fenêtre cachée
	HWND hiddenWindow = CreateWindowEx(
		0,                              // Styles étendus
		L"MyHiddenWindowClass",        // Nom de la classe
		L"MyHiddenWindow",              // Titre de la fenêtre
		WS_OVERLAPPEDWINDOW,// Style de la fenêtre (fenêtre cachée)
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	// Vérifier si la fenêtre a été créée avec succès
	if (hiddenWindow == NULL) {
		MessageBox(NULL, L"Erreur lors de la création de la fenêtre cachée.", L"Erreur", MB_ICONERROR);
		return 1;
	}

	// Afficher la fenêtre cachée (si nécessaire)
	// ShowWindow(hiddenWindow, SW_SHOWNORMAL);

	// cout << "Web Server thread running...\n";

	SOCKET webSocket;
	sockaddr_in webHint;
	char buf[4096];
	ZeroMemory(buf, sizeof(buf));

	// Initialisation du serveur web
	if (!initializeWebServer(webSocket, webHint, 5005, hiddenWindow)) {
		return 0; // Quitter le thread en cas d'échec de l'initialisation
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Fermeture du serveur web
	closesocket(webSocket);
	WSACleanup();

	return 0;
}

int main() {
	// Creation de la window class
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Définir la classe de la fenêtre
	WNDCLASS windowClass = {};
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = L"MyHiddenWindowClass";

	// Enregistrer la classe de fenêtre
	RegisterClass(&windowClass);

	// Créer la fenêtre cachée
	HWND hiddenWindow = CreateWindowEx(
		0,                              // Styles étendus
		L"MyHiddenWindowClass",        // Nom de la classe
		L"MyHiddenWindow",              // Titre de la fenêtre
		WS_OVERLAPPEDWINDOW,// Style de la fenêtre (fenêtre cachée)
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	// Vérifier si la fenêtre a été créée avec succès
	if (hiddenWindow == NULL) {
		MessageBox(NULL, L"Erreur lors de la création de la fenêtre cachée.", L"Erreur", MB_ICONERROR);
		return 1;
	}

	// Afficher la fenêtre cachée (si nécessaire)
	//ShowWindow(hiddenWindow, SW_SHOWNORMAL);

	// Gestion Threads
	HANDLE serverThread = CreateThread(NULL, 0, serverMain, NULL, CREATE_NO_WINDOW, NULL);
	if (serverThread == NULL) {
		cerr << "Failed to create server thread!\n";
		return 1;
	}

	HANDLE webThread = CreateThread(NULL, 0, webServer, NULL, CREATE_NO_WINDOW, NULL);
	if (webThread == NULL) {
		cerr << "Failed to create web server thread!\n";
		return 1;
	}

	// cout << "Main thread running...\n";
	
	// Attendre la fin des threads avant de terminer
	WaitForSingleObject(serverThread, INFINITE);
	WaitForSingleObject(webThread, INFINITE);

	CloseHandle(serverThread);
	CloseHandle(webThread);

	return 0;
}