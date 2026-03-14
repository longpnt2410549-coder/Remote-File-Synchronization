#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "common.h"

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[]) {
    // Fix loi tham so dau vao
    if (argc < 2) {
        printf("Cach dung: %s <server_ip>\n", argv[0]);
        return 1;
    }

    WSADATA wsa;
   if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Don rac bo nho
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); // Khop voi port 9001 cua server

    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // Dung argv[1]
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed! Is your friend's server on?\n");
    } else {
        printf("CONNECTED to %s\n", argv[1]);
        
        // --- ĐOẠN CODE TEST BẮN DỮ LIỆU ---
        char test_msg[] = "DOWNLOAD test.txt\n";
        send(sock, test_msg, strlen(test_msg), 0);
            // After sending the request...
MsgHeader response_header;
int n = recv(sock, (char*)&response_header, sizeof(MsgHeader), 0);

if (n > 0 && response_header.type == MSG_FILE_CONTENT) {
    receive_file(sock, response_header);
}
        printf("Da gui lenh test: %s\n", test_msg);
        // ----------------------------------
    }

    int dry_run = 0;
    int watch_interval = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dry-run") == 0) dry_run = 1;
        if (strcmp(argv[i], "--watch") == 0 && i + 1 < argc) {
            watch_interval = atoi(argv[i+1]);
        }
    }

do {
    printf("\n--- Starting Sync ---\n");
    
    // ... your existing code to send the request ...

    // ADD THIS HERE:
    MsgHeader response_header;
    int n = recv(sock, (char*)&response_header, sizeof(MsgHeader), 0);

    if (n > 0 && response_header.type == MSG_FILE_CONTENT) {
        receive_file(sock, response_header);
    }

    if (watch_interval > 0) {
        printf("Watch mode: Sleeping for %d seconds...\n", watch_interval);
        Sleep(watch_interval * 1000);
    }
} while (watch_interval > 0); // Make sure the semicolon is here!

    closesocket(sock);
    WSACleanup();
    return 0;
} // This is the final bracket for main