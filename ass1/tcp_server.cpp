#include <iostream>
#include <fstream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>  // for _mkdir
#include <ctime>

#pragma comment(lib, "ws2_32.lib")
#define PORT 3001
#define BUFFER_SIZE 1024

using namespace std;

// Function to log upload/download/disconnection
void log_transfer(const string& username, const string& action, const string& filename = "") {
    ofstream log("server_log.txt", ios::app);
    time_t now = time(0);
    if (filename.empty()) {
        log << "[" << ctime(&now) << "] " << username << " " << action << ".\n";
    }
    else {
        log << "[" << ctime(&now) << "] " << username << " " << action << " " << filename << " via TCP\n";
    }
    log.close();
}

// Updated: Function to send list of files to client and log it
void send_file_list(SOCKET client_sock, const string& username) {
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
    send(client_sock, list.c_str(), list.length() + 1, 0);

    // Log the list request
    ofstream log("server_log.txt", ios::app);
    time_t now = time(0);
    log << "[" << ctime(&now) << "] " << username << " requested file list.\n";
    log.close();
}

// Threaded function to handle a client
void handle_client(SOCKET client_sock) {
    char buffer[BUFFER_SIZE];
    char username[100] = { 0 };

    // Ask and receive username
    send(client_sock, "Enter username: ", 17, 0);
    recv(client_sock, username, sizeof(username), 0);

    // Log connection
    time_t now = time(0);
    cout << "[" << ctime(&now) << "] " << username << " connected.\n";
    log_transfer(username, "connected");

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (received <= 0) break;

        string command = buffer;

        if (command == "1") { // Upload
            char filename[100] = { 0 };
            recv(client_sock, filename, sizeof(filename), 0);
            string full_path = "files/" + string(filename);

            ofstream outfile(full_path, ios::binary);
            int bytesReceived = 0;
            while ((bytesReceived = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
                if (string(buffer) == "__END__") break;
                outfile.write(buffer, bytesReceived);
            }
            outfile.close();
            log_transfer(username, "uploaded", filename);
        }

        else if (command == "2") { // Download
            char filename[100] = { 0 };
            recv(client_sock, filename, sizeof(filename), 0);
            string full_path = "files/" + string(filename);
            ifstream infile(full_path, ios::binary);
            if (!infile) {
                int error = -1;
                send(client_sock, (char*)&error, sizeof(int), 0);  // send -1 as error
            }
            else {
                infile.seekg(0, ios::end);
                int filesize = infile.tellg();
                infile.seekg(0, ios::beg);

                send(client_sock, (char*)&filesize, sizeof(int), 0);  // send filesize

                while (!infile.eof()) {
                    infile.read(buffer, BUFFER_SIZE);
                    send(client_sock, buffer, infile.gcount(), 0);
                }
                infile.close();
                log_transfer(username, "downloaded", filename);
            }
        }

        else if (command == "3") { // List
            send_file_list(client_sock, username);
        }

        else if (command == "4") { // Exit
            break;
        }
    }

    // Log disconnection
    time_t exit_time = time(0);
    cout << "[" << ctime(&exit_time) << "] " << username << " disconnected.\n";
    log_transfer(username, "disconnected");

    closesocket(client_sock);
}

// Server setup
void server() {
    _mkdir("files");  // Create folder if it doesn't exist

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);
    cout << "TCP Server listening on port " << PORT << "...\n";

    while (true) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        thread t(handle_client, client_sock);
        t.detach();
    }

    closesocket(server_sock);
    WSACleanup();
}

int main() {
    server();
    return 0;
}
