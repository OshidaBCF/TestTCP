#include <iostream>
#include "Zone.h"
#include <string>
#include <vector>
#include <WS2tcpip.h>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

int  main() {
	std::vector<zone> zones;
	int painter = zone::painterList::CIRCLE;

	for (int j = 0; j < 3; j++)
	{
		for (int i = 0; i < 3; i++)
		{
			zone newZone(Vector2i(i * 300, j * 300));
			zones.push_back(newZone);
		}
	}

	int winner = 0;

	while (true) {
		// winsock
		WSADATA wsData;
		WORD ver = MAKEWORD(2, 2);

		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0) {
			cerr << "Can't Initialize winsock! Quitting..." << endl;
			return 0;
		}

		// Create a socket!
		SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
		if (listening == INVALID_SOCKET) {
			cerr << "Can't create a socket! Quitting..." << endl;
			return 0;
		}

		// Bind the ip and port to socket!
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(5004);
		hint.sin_addr.S_un.S_addr = INADDR_ANY;

		bind(listening, (sockaddr*)&hint, sizeof(hint));

		// Tell winsock for listening
		listen(listening, SOMAXCONN);

		// Wait for connection!
		sockaddr_in client;
		int clientSize = sizeof(client);

		SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

		char host[NI_MAXHOST]; // Client's name!
		char service[NI_MAXSERV]; // Service (IP:PORT) wthe client when connect!

		ZeroMemory(host, NI_MAXHOST); // same of memset (host, 0 , NI_MAXHOST);
		ZeroMemory(service, NI_MAXSERV);

		if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
			cout << host << " connected on port " << service << endl;
		}
		else {
			inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
			cout << host << " connected on port " <<
				ntohs(client.sin_port) << endl;
		}

		// Close socket listening
		closesocket(listening);

		// loop accept message back to client!
		char buf[4096];
		string userInput;

		while (true) {
			ZeroMemory(buf, 4096);

			int byteReceived = recv(clientSocket, buf, 4096, 0) + 1;
			if (byteReceived == SOCKET_ERROR) {
				cerr << "Error in recv(). Quitting" << endl;
				break;
			}

			if (byteReceived == 0) {
				cout << "Client is disconnected!" << endl;
				break;
			}

			Vector2i position;
			if (buf[0] == 'P')
			{
				if (buf[1] == '-')
				{
					painter = -1;
					position = Vector2i(int(buf[4]) - '0', int(buf[6]) - '0');
				}
				else
				{
					painter = 1;
					position = Vector2i(int(buf[3]) - '0', int(buf[5]) - '0');
				}
				userInput = "P" + to_string(painter) + "X" + to_string(position.x) + "Y" + to_string(position.y);
				zones[position.x + position.y * 3].painter = painter;
			}

			send(clientSocket, userInput.c_str(), userInput.size() + 1, 0);
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
				winner = painter;
			}

			if (winner == -1)
			{
				send(clientSocket, "W1", 3, 0);
			}
			else if (winner == 1)
			{
				send(clientSocket, "W2", 3, 0);
			}

			// message back to client!
			//send(clientSocket, buf, byteReceived + 1, 0);
		}

		closesocket(clientSocket);
	}
	WSACleanup();
	system("pause");
}