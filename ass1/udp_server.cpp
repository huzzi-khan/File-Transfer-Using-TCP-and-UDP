#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define PORT 3002
#define BUFFER_SIZE 1024

using namespace std;

struct sockaddr_in client_addr;
int client_len = sizeof(client_addr);
SOCKET server_sock;

void log_transfer(const string& username, const string& action, const string& filename = "") {
    ofstream log("server_log_udp.txt", ios::app);
    time_t now = time(0);
    if (filename.empty()) {
        log << "[" << ctime(&now) << "] " << username << " " << action << ".\n";
    }
    else {
        log << "[" << ctime(&now) << "] " << username << " " << action << " " << filename << " via UDP\n";
    }
    log.close();
}

void send_file_list(const string& username) {
    string list = "Files:\n";
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile("files\\*", &data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                list += data.cFileName;
                list += "\n";
            }
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }
    sendto(server_sock, list.c_str(), list.length() + 1, 0, (sockaddr*)&client_addr, client_len);
    log_transfer(username, "requested file list");
}

void handle_udp() {
    char buffer[BUFFER_SIZE];
    char username[100];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        memset(username, 0, 100);

        // Step 1: Receive username
        recvfrom(server_sock, username, sizeof(username), 0, (sockaddr*)&client_addr, &client_len);

        // Step 2: Receive command
        recvfrom(server_sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&client_addr, &client_len);
        string command = buffer;

        if (command == "1") { // Upload
            char filename[100] = { 0 };
            recvfrom(server_sock, filename, sizeof(filename), 0, (sockaddr*)&client_addr, &client_len);
            string full_path = "files/" + string(filename);
            ofstream outfile(full_path, ios::binary);

            while (true) {
                int bytes = recvfrom(server_sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&client_addr, &client_len);
                if (strncmp(buffer, "__END__", 7) == 0 && bytes == 8) break;
                outfile.write(buffer, bytes);
            }

            outfile.close();
            log_transfer(username, "uploaded", filename);
        }

        else if (command == "2") { // Download
            char filename[100] = { 0 };
            recvfrom(server_sock, filename, sizeof(filename), 0, (sockaddr*)&client_addr, &client_len);
            string full_path = "files/" + string(filename);
            ifstream infile(full_path, ios::binary);
            if (!infile) {
                sendto(server_sock, "FILE_NOT_FOUND", 15, 0, (sockaddr*)&client_addr, client_len);
            }
            else {
                while (!infile.eof()) {
                    infile.read(buffer, BUFFER_SIZE);
                    sendto(server_sock, buffer, infile.gcount(), 0, (sockaddr*)&client_addr, client_len);
                }
                sendto(server_sock, "__END__", 8, 0, (sockaddr*)&client_addr, client_len);
                infile.close();
                log_transfer(username, "downloaded", filename);
            }
        }

        else if (command == "3") {
            send_file_list(username);
        }

        else if (command == "4") {
            log_transfer(username, "disconnected");
        }
    }
}

void server() {
    _mkdir("files");

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    cout << "UDP Server listening on port " << PORT << "...\n";

    handle_udp();

    closesocket(server_sock);
    WSACleanup();
}

int main() {
    server();
    return 0;
}
