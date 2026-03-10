#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "common.h"

// Tell the compiler to use the Winsock library
#pragma comment(lib, "ws2_32.lib")


int main(int argc, char *argv[]) {
    WSADATA wsa;
if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
    printf("Failed. Error Code : %d", WSAGetLastError());
    return 1;
}
// ... inside main ...
int sock = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in serv_addr;
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(9000); // The port your friend gave you

// Convert the IP from your command line (argv[2]) to binary
serv_addr.sin_addr.s_addr = inet_addr(argv[2]);
if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
    printf("Invalid address\n");
    return -1;
}

if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("Connection Failed! Is your friend's server on?\n");
} else {
    printf("CONNECTED to %s\n", argv[2]);
}
    int dry_run = 0;
    int watch_interval = 0;

    // Simple argument parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dry-run") == 0) dry_run = 1;
        if (strcmp(argv[i], "--watch") == 0 && i + 1 < argc) {
            watch_interval = atoi(argv[i+1]);
        }
    }

    do {
        printf("\n--- Starting Sync ---\n");
        
        // 1. Collect Local Metadata
        // 2. Connect to Server
        // 3. Send Metadata & Receive Diff
        // 4. If (!dry_run) { Execute Transfers }

        if (watch_interval > 0) {
            printf("Watch mode: Sleeping for %d seconds...\n", watch_interval);
            Sleep(watch_interval * 1000);
        }
    } while (watch_interval > 0);

    return 0;
}

