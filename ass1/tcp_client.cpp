#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 3001
#define BUFFER_SIZE 1024

using namespace std;

void client() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = { 0 };

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        cerr << "Connection failed." << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }

    // Get username prompt from server
    recv(sock, buffer, BUFFER_SIZE, 0);
    cout << buffer;
    string username;
    getline(cin, username);
    send(sock, username.c_str(), username.size() + 1, 0);

    while (true) {
        cout << "\nMenu:\n";
        cout << "1. Upload File\n";
        cout << "2. Download File\n";
        cout << "3. List Files\n";
        cout << "4. Exit\n";
        cout << "Enter choice: ";
        string choice;
        getline(cin, choice);
        send(sock, choice.c_str(), choice.size() + 1, 0);

        if (choice == "1") {
            cout << "Enter filename to upload: ";
            string filename;
            getline(cin, filename);

            ifstream infile(filename, ios::binary);
            if (!infile) {
                cout << "File not found.\n";
                continue;
            }

            send(sock, filename.c_str(), filename.size() + 1, 0);

            while (!infile.eof()) {
                infile.read(buffer, BUFFER_SIZE);
                send(sock, buffer, infile.gcount(), 0);
            }
            send(sock, "__END__", 8, 0);
            infile.close();
            cout << "File uploaded.\n";
        }
        else if (choice == "2") {
            cout << "Enter filename to download: ";
            string filename;
            getline(cin, filename);
            send(sock, filename.c_str(), filename.size() + 1, 0);

            int filesize = 0;
            recv(sock, (char*)&filesize, sizeof(int), 0);

            if (filesize == -1) {
                cout << "File not found on server.\n";
                continue;
            }

            ofstream outfile("downloaded_" + filename, ios::binary);
            int totalReceived = 0;
            int bytesReceived = 0;

            while (totalReceived < filesize) {
                bytesReceived = recv(sock, buffer, BUFFER_SIZE, 0);
                if (bytesReceived <= 0) break;
                outfile.write(buffer, bytesReceived);
                totalReceived += bytesReceived;
            }
            outfile.close();
            cout << "Download complete.\n";
        }
        else if (choice == "3") {
            recv(sock, buffer, BUFFER_SIZE, 0);
            cout << "\n" << buffer << endl;
        }
        else if (choice == "4") {
            cout << "Exiting.\n";
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
}

int main() {
    client();
    return 0;
}
