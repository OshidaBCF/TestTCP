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

#define EVENT_FOR_MAIN (WM_USER + 1)
#define EVENT_FOR_GAME (WM_USER + 2)
#define EVENT_FOR_WEB  (WM_USER + 3)
#define WM_SHARED_SOCKET_EVENT  (WM_USER + 4)

std::string message;
std::mutex messageMutex;


void clientHandler();

struct ClientData {
	SOCKET clientSocket;
	std::vector<zone> zones;
	int painter;
	int winner;

	ClientData(SOCKET socket) : clientSocket(socket), painter(zone::painterList::CIRCLE), winner(0) {}
};

std::vector<ClientData> clients;

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
	case EVENT_FOR_MAIN:
		messageMutex.lock();
		for (int i = 0; i < clients.size(); i++)
		{
			send(clients[i].clientSocket, message.c_str(), message.size() + 1, 0);
		}
		messageMutex.unlock();
		PostMessage(hwnd, EVENT_FOR_WEB, wParam, lParam);
		break;
	case EVENT_FOR_GAME:
		messageMutex.lock();
		clientHandler();
		messageMutex.unlock();
		PostMessage(hwnd, EVENT_FOR_WEB, wParam, lParam);
		break;
	case EVENT_FOR_WEB:
		messageMutex.lock();

		messageMutex.unlock();
		break;
	case WM_SHARED_SOCKET_EVENT:
	{
		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
		{
			if ((Accept = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				break;
			}
			else
				printf("accept() is OK!\n");

			WSAAsyncSelect(Accept, hwnd, WM_SHARED_SOCKET_EVENT, FD_READ | FD_WRITE | FD_CLOSE);
	

			/*//----------------------
			// Accept the connection.
			AcceptSocket = accept(listeningSocket, NULL, NULL);
			if (AcceptSocket == INVALID_SOCKET) {
				wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
				closesocket(listeningSocket);
				WSACleanup();
				return 1;
			}
			else
			{
				clients.emplace_back(AcceptSocket);
				// Ici, vous pouvez gérer la logique pour les spectateurs et les joueurs dans le même thread
				// Par exemple, examinez si un joueur est déjà connecté, puis assignez le nouveau client comme spectateur
				if (clients.size() <= 2) {
					cout << "Player connected!" << endl; // Message lorsque qu'un joueur se connecte
					// Traitez les joueurs
					// Assignez le joueur et initiez le jeu si nécessaire
				}
				else {
					cout << "Spectator connected!" << endl; // Message lorsque qu'un joueur se connecte
					// Traitez les spectateurs
					// Affichez l'état actuel du jeu ou d'autres informations pertinentes aux spectateurs
				}
			}*/
		}
		break;
		case FD_READ:
		{
			SOCKET data_socket = (SOCKET)wParam;
			char buf[4096];
			ZeroMemory(buf, 4096);
			int BytesReceived = recv(data_socket, buf, 4096, 0);
			if (BytesReceived)
			{
				// Rien
			}
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

bool initializeServer(SOCKET& listeningSocket, sockaddr_in& hint, int port, HWND hWnd) {
	// Initialisation de Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Can't Initialize winsock! Quitting..." << endl;
		return false;
	}

	// Création du socket
	listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting..." << endl;
		WSACleanup();
		return false;
	}

	// Configuration du socket
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	// Liaison du socket
	if (bind(listeningSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
		cerr << "Can't bind to IP/port! Quitting..." << endl;
		closesocket(listeningSocket);
		WSACleanup();
		return false;
	}

	// Mise en écoute du socket
	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "Can't listen on socket! Quitting..." << endl;
		closesocket(listeningSocket);
		WSACleanup();
		return false;
	}

	WSAAsyncSelect(listeningSocket, hWnd, WM_SHARED_SOCKET_EVENT, FD_ACCEPT | FD_CLOSE);

	cout << "Server initialized and listening on port " << port << endl;
	return true;
}

void checkWinner(vector<zone>& zones, int& winner, SOCKET clientSocket) {
	// Vérification du gagnant selon les règles du jeu
	// Vérifiez si l'une des combinaisons gagnantes est remplie

	// 0 1 2
	// 3 4 5
	// 6 7 8
	// 0,1,2  3,4,5  6,7,8  0,3,6  1,4,7  2,5,8  0,4,8  2,4,6
	if ((zones[0].painter != 0 && zones[0].painter == zones[1].painter && zones[0].painter == zones[2].painter) ||
		(zones[3].painter != 0 && zones[3].painter == zones[4].painter && zones[3].painter == zones[5].painter) ||
		(zones[6].painter != 0 && zones[6].painter == zones[7].painter && zones[6].painter == zones[8].painter) ||
		(zones[0].painter != 0 && zones[0].painter == zones[3].painter && zones[0].painter == zones[6].painter) ||
		(zones[1].painter != 0 && zones[1].painter == zones[4].painter && zones[1].painter == zones[7].painter) ||
		(zones[2].painter != 0 && zones[2].painter == zones[5].painter && zones[2].painter == zones[8].painter) ||
		(zones[0].painter != 0 && zones[0].painter == zones[4].painter && zones[0].painter == zones[8].painter) ||
		(zones[2].painter != 0 && zones[2].painter == zones[4].painter && zones[2].painter == zones[6].painter))
	{
		// S'il y a un gagnant
		winner = zones[0].painter; // Assumons que le gagnant est le joueur de la zone[0]

		// Envoi d'un message au client indiquant le gagnant
		if (winner == zone::painterList::CIRCLE) {
			send(clientSocket, "W1", 3, 0); // Message indiquant la victoire du cercle
			cout << "Player 1 wins!" << endl;
		}
		else if (winner == zone::painterList::CROSS) {
			send(clientSocket, "W2", 3, 0); // Message indiquant la victoire de la croix
			cout << "Player 2 wins!" << endl;
		}
	}
	else {
		// Si aucun gagnant, vous pouvez envoyer un message pour continuer le jeu ou indiquer qu'il n'y a pas encore de gagnant
		// Exemple : 
		send(clientSocket, "NoWinnerYet", 12, 0);
		cout << "No winner yet!" << endl;
	}
}

void handleMove(ClientData& clientData, char* buf) {
	SOCKET clientSocket = clientData.clientSocket;
	std::vector<zone>& zones = clientData.zones;
	int& painter = clientData.painter;
	int& winner = clientData.winner;

	ZeroMemory(buf, 4096);

	int byteReceived = recv(clientSocket, buf, 4096, 0);
	if (byteReceived == SOCKET_ERROR) {
		cerr << "Error in recv(). Quitting" << endl;
		return;
	}
	if (byteReceived == 0) {
		cout << "Client is disconnected!" << endl;
		return;
	}

	cout << buf << endl;

	Vector2i position;
	if (buf[0] == 'P') {
		if (int(buf[1]) - '0' == painter) {
			position = Vector2i(int(buf[3]) - '0', int(buf[5]) - '0');
			string userInput = "P" + to_string(painter) + "X" + to_string(position.x) + "Y" + to_string(position.y);
			zones[position.x + position.y * 3].painter = painter;
			send(clientSocket, userInput.c_str(), userInput.size() + 1, 0);

			// Vérification du gagnant après chaque mouvement
			checkWinner(zones, winner, clientSocket);

			// Changement de joueur après un mouvement valide
			if (painter == zone::painterList::CIRCLE) {
				painter = zone::painterList::CROSS;
			}
			else {
				painter = zone::painterList::CIRCLE;
			}
		}
		else {
			// Envoyer un message indiquant que c'est au tour de l'autre joueur
			string message = "Not your turn, current player: " + to_string(painter);
			send(clientSocket, message.c_str(), message.size() + 1, 0);
			cout << "Not your turn, current player: " << painter << endl;
		}
	}
}

void clientHandler() {
	for (int i = 0; i < clients.size(); i++)
	{
		SOCKET clientSocket = clients[i].clientSocket;
		std::vector<zone> zones = clients[i].zones;
		int painter = clients[i].painter;
		int winner = clients[i].winner;

		// Message pour indiquer qu'un client s'est connecté
		cout << "Client connected!" << endl;

		char buf[4096];
		while (true) {
			ZeroMemory(buf, sizeof(buf));

			int byteReceived = recv(clientSocket, buf, sizeof(buf), 0);
			if (byteReceived == SOCKET_ERROR) {
				cerr << "Error in recv(). Quitting" << endl;
				break;
			}

			if (byteReceived == 0) {
				cout << "Client is disconnected!" << endl;
				// TODO remove client from clientlist
				break;
			}

			// Traitement des données reçues du client
			cout << "Received from client: " << buf << endl;

			// Appeler la fonction pour gérer les mouvements du joueur
			handleMove(clients[i], buf);
		}

		// Fermer le socket du client lorsque la communication est terminée
		closesocket(clientSocket);
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
	//ShowWindow(hiddenWindow, SW_SHOWNORMAL);

	cout << "Server Main thread running...\n";
	
	// Déclaration des variables pour le serveur
	SOCKET listening;
	sockaddr_in hint;
	char buf[4096];

	// Initialisation du serveur
	if (!initializeServer(listening, hint, 5004, hiddenWindow)) {
		return 0; // Quitter le thread en cas d'échec de l'initialisation
	}

	std::vector<zone> zones;
	int painter = zone::painterList::CIRCLE;
	int winner = 0;

	// Do something at some point iguess
	MSG msg;
	DWORD Ret;
	while (Ret = GetMessage(&msg, NULL, 0, 0))
	{

		if (Ret == -1)

		{

			printf("\nGetMessage() failed with error %d\n", GetLastError());

			return 1;

		}

		else

			printf("\nGetMessage() is pretty fine!\n");



		printf("Translating a message...\n");

		TranslateMessage(&msg);

		printf("Dispatching a message...\n");

		DispatchMessage(&msg);

	}

	// Fermeture du serveur
	closesocket(listening);
	WSACleanup();

	return 0;
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
	//ShowWindow(hiddenWindow, SW_SHOWNORMAL);

	// Lancer la boucle de messages
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	cout << "Web Server thread running...\n";
	// Mettez ici le code du serveur web
	// Il peut gérer les requêtes HTTP pour afficher l'état du jeu sur un navigateur
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
		cerr << "Failed to create server thread!" << endl;
		return 1;
	}

	HANDLE webThread = CreateThread(NULL, 0, webServer, NULL, CREATE_NO_WINDOW, NULL);
	if (webThread == NULL) {
		cerr << "Failed to create web server thread!" << endl;
		return 1;
	}

	cout << "Main thread running...\n";
	// Logique du thread principal (interface utilisateur, traitement supplémentaire, etc.)
	// recv message
	// put message in mutex
	// send event to serverThread
	// recv event from serverThread
	// get message from mutex
	// send message to client(s)
	// send event to webThread

	// Attendre la fin des threads avant de terminer
	WaitForSingleObject(serverThread, INFINITE);
	WaitForSingleObject(webThread, INFINITE);

	CloseHandle(serverThread);
	CloseHandle(webThread);

	return 0;
}