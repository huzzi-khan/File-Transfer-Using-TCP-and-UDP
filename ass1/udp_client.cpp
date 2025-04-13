#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 3002
#define BUFFER_SIZE 1024

using namespace std;

void client() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock;
    struct sockaddr_in serv_addr;
    int serv_len = sizeof(serv_addr);
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    cout << "Enter username: ";
    string username;
    getline(cin, username);

    while (true) {
        cout << "\nMenu:\n";
        cout << "1. Upload File\n";
        cout << "2. Download File\n";
        cout << "3. List Files\n";
        cout << "4. Exit\n";
        cout << "Enter choice: ";
        string choice;
        getline(cin, choice);

        // Send username and command
        sendto(sock, username.c_str(), username.size() + 1, 0, (sockaddr*)&serv_addr, serv_len);
        sendto(sock, choice.c_str(), choice.size() + 1, 0, (sockaddr*)&serv_addr, serv_len);

        if (choice == "1") {
            cout << "Enter filename to upload: ";
            string filename;
            getline(cin, filename);

            ifstream infile(filename, ios::binary);
            if (!infile) {
                cout << "File not found.\n";
                continue;
            }

            sendto(sock, filename.c_str(), filename.size() + 1, 0, (sockaddr*)&serv_addr, serv_len);

            while (!infile.eof()) {
                infile.read(buffer, BUFFER_SIZE);
                sendto(sock, buffer, infile.gcount(), 0, (sockaddr*)&serv_addr, serv_len);
            }

            sendto(sock, "__END__", 8, 0, (sockaddr*)&serv_addr, serv_len);
            infile.close();
            cout << "File uploaded.\n";
        }

        else if (choice == "2") {
            cout << "Enter filename to download: ";
            string filename;
            getline(cin, filename);

            sendto(sock, filename.c_str(), filename.size() + 1, 0, (sockaddr*)&serv_addr, serv_len);

            ofstream outfile("downloaded_" + filename, ios::binary);
            int bytes;
            while (true) {
                bytes = recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&serv_addr, &serv_len);
                if (strncmp(buffer, "FILE_NOT_FOUND", 14) == 0) {
                    cout << "File not found on server.\n";
                    outfile.close();
                    remove(("downloaded_" + filename).c_str());
                    break;
                }
                if (strncmp(buffer, "__END__", 7) == 0 && bytes == 8) break;
                outfile.write(buffer, bytes);
            }
            outfile.close();
            cout << "Download complete.\n";
        }

        else if (choice == "3") {
            recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&serv_addr, &serv_len);
            cout << "\n" << buffer << endl;
        }

        else if (choice == "4") {
            cout << "Goodbye.\n";
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
