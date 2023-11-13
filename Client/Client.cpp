#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main()
{
    string Host = "127.0.0.1"; // Server IP
    int Port = 5004; // Server Port

    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    
    if (wsResult != 0)
    {
        cerr << "Can't start winsocket, error #" << wsResult << endl;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        cerr << "Can't create socket, error #" << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }
    
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(Port);
    inet_pton(AF_INET, Host.c_str(), &hint.sin_addr);

    int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if (connResult == SOCKET_ERROR)
    {
        cerr << "Can't connect to server, error #" << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return 0;
    }

    char buf[4096];
    string userInput;

    do {
        cout << "> ";
        getline(cin, userInput);

        if (userInput.size() > 0)
        {
            int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
            if (sendResult != SOCKET_ERROR)
            {
                ZeroMemory(buf, 4096);
                int BytesReceived = recv(sock, buf, 4096, 0);
                if (BytesReceived)
                {
                    cout << "Server>" << string(buf, 0, BytesReceived) << endl;
                }
            }
        }
    } while (userInput.size() > 0);

    closesocket(sock);
    WSACleanup();
    return 0;
}