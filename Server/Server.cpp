#include <iostream>
#include "Zone.h"
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <thread>
#pragma comment (lib, "ws2_32.lib")
using namespace std;

struct ClientData {
	SOCKET clientSocket;
	int painter;

	ClientData(SOCKET socket, int paint) : clientSocket(socket), painter(paint){}
};


std::vector<ClientData> clients;
std::vector<zone> zones;
int painter = zone::painterList::CIRCLE;

int newPlayerPaint = zone::painterList::CIRCLE;

int winner = 0;
int victories[8][3] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6} };

bool initializeServer(SOCKET& listeningSocket, sockaddr_in& hint, int port) {
	// Initialisation de Winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Can't Initialize winsock! Quitting..." << endl;
		return false;
	}

	// Creation du socket
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

	// Mise en ecoute du socket
	if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
		cerr << "Can't listen on socket! Quitting..." << endl;
		closesocket(listeningSocket);
		WSACleanup();
		return false;
	}

	cout << "Server initialized and listening on port " << port << endl;
	return true;
}

void checkWinner(vector<zone>& zones, int& winner, SOCKET clientSocket) {
	// Verification du gagnant selon les r�gles du jeu
	// Verifiez si l'une des combinaisons gagnantes est remplie

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
		winner = zones[0].painter; // Assumons que le gagnant est le joueur de la zone[0], TODO Utiliser le joueur qui vient de jouer

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

//void handleMove(ClientData& clientData, char* buf) {
//	SOCKET clientSocket = clientData.clientSocket;
//	int& painter = clientData.painter;
//
//	ZeroMemory(buf, 4096);
//
//	int byteReceived = recv(clientSocket, buf, 4096, 0);
//	if (byteReceived == SOCKET_ERROR) {
//		cerr << "Error in recv(). Quitting" << endl;
//		return;
//	}
//
//	if (byteReceived == 0) {
//		cout << "Client is disconnected!" << endl;
//		return;
//	}
//
//	cout << buf << endl;
//
//	Vector2i position;
//	if (buf[0] == 'P') {
//		if (int(buf[1]) - '0' == painter) {
//			position = Vector2i(int(buf[3]) - '0', int(buf[5]) - '0');
//			string userInput = "P" + to_string(painter) + "X" + to_string(position.x) + "Y" + to_string(position.y);
//			zones[position.x + position.y * 3].painter = painter;
//			send(clientSocket, userInput.c_str(), userInput.size() + 1, 0);
//
//			// Verification du gagnant apres chaque mouvement
//			checkWinner(zones, winner, clientSocket);
//
//			// Changement de joueur apres un mouvement valide
//			if (painter == zone::painterList::CIRCLE) {
//				painter = zone::painterList::CROSS;
//			}
//			else {
//				painter = zone::painterList::CIRCLE;
//			}
//		}
//		else {
//			// Envoyer un message indiquant que c'est au tour de l'autre joueur
//			string message = "Not your turn, current player: " + to_string(painter);
//			send(clientSocket, message.c_str(), message.size() + 1, 0);
//			cout << "Not your turn, current player: " << painter << endl;
//		}
//	}
//}

void sendAllClients(string userInput)
{
	for (int i = 0; i < clients.size(); i++)
	{
		send(clients[i].clientSocket, userInput.c_str(), userInput.size() + 1, 0);
	}
}
void clientHandler(ClientData& clientData) {
	SOCKET clientSocket = clientData.clientSocket;

	// Message pour indiquer qu'un client s'est connecte
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
			break;
		}

		// Traitement des donnees recues du client
		cout << "Received from client: " << buf << endl;

		// Appeler la fonction pour gerer les mouvements du joueur
		// handleMove(clientData, buf);

		Vector2i position;
		string userInput;
		if (buf[0] == 'P')
		{
			if (int(buf[1]) - '0' == painter)
			{
				position = Vector2i(int(buf[3]) - '0', int(buf[5]) - '0');
				// userInput = "P" + to_string(painter) + "X" + to_string(position.x) + "Y" + to_string(position.y);
				zones[position.x + position.y * 3].painter = painter;
				userInput = "S";

				for (int j = 0; j < 3; j++)
				{
					for (int i = 0; i < 3; i++)
					{
						userInput += to_string(zones[i + j * 3].painter);
					}
				}
				send(clientSocket, userInput.c_str(), userInput.size(), 0);
				cout << userInput << endl;
				sendAllClients(userInput);

				for (int i = 0; i < 8; i++)
				{
					if (zones[victories[i][0]].painter != zone::painterList::NONE && zones[victories[i][0]].painter == zones[victories[i][1]].painter && zones[victories[i][0]].painter == zones[victories[i][2]].painter)
					{
						winner = painter;
					}
				}

				if (winner == zone::painterList::CIRCLE)
				{
					sendAllClients("W1");
					cout << "W1" << endl;
				}
				else if (winner == zone::painterList::CROSS)
				{
					sendAllClients("W2");
					cout << "W2" << endl;
				}

				if (painter == zone::painterList::CIRCLE)
				{
					painter = zone::painterList::CROSS;
				}
				else if (painter == zone::painterList::CROSS)
				{
					painter = zone::painterList::CIRCLE;
				}
			}
			else
			{
				send(clientSocket, 'N' + to_string(painter).c_str(), 2, 0);
				cout << "N" + to_string(painter) << endl;
			}
		}
		if (buf[0] == 'S')
		{
			userInput = "S";

			for (int j = 0; j < 3; j++)
			{
				for (int i = 0; i < 3; i++)
				{
					userInput += to_string(zones[i + j * 3].painter);
				}
			}
			send(clientSocket, userInput.c_str(), userInput.size(), 0);
			cout << userInput << endl;
		}
		if (buf[0] == 'Q')
		{
			userInput = "Q" + to_string(clientData.painter);
			send(clientSocket, userInput.c_str(), userInput.size(), 0);
			cout << userInput << endl;
		}
	}

	// Fermer le socket du client lorsque la communication est terminee
	closesocket(clientSocket);
}

int main() 
{
	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			zone newZone(Vector2i(i, j));
			zones.push_back(newZone);
		}
	}

	// Declaration des variables pour le serveur
	SOCKET listening;
	sockaddr_in hint;
	char buf[4096];

	// Initialisation du serveur
	if (!initializeServer(listening, hint, 5004)) {
		return 0; // Quitter le programme en cas d'echec de l'initialisation
	}

	// Boucle d'acceptation des clients et traitement
	while (true) {
		// Acceptation des connexions des clients
		SOCKET clientSocket = accept(listening, nullptr, nullptr);
		if (clientSocket != INVALID_SOCKET) {
			ClientData client = ClientData(clientSocket, newPlayerPaint);
			clients.emplace_back(client);
			if (newPlayerPaint == zone::painterList::CROSS)
				newPlayerPaint = zone::painterList::NONE;
			if (newPlayerPaint == zone::painterList::CIRCLE)
				newPlayerPaint = zone::painterList::CROSS;

			std::thread clientThread(clientHandler, std::ref(clients.back()));
			clientThread.detach();
		}
	}

	// Fermeture du serveur
	closesocket(listening);
	WSACleanup();

	return 0;
}