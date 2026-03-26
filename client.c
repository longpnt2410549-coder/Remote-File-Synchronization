#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "common.h"

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[]) {
    int dry_run = 0;
    int watch_interval = 0;
    char batch_buffer[8192] = {0}; 
    char input[256];
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
// ... after connection success ...

char batch_buffer[8192] = {0}; 
char input[256];

do {
    printf("\n[BATCH MODE] Type 'SYNC' to send, 'FILE' to load script, or enter a command:\n> ");
    
    // Clear the input buffer each time
    if (fgets(input, sizeof(input), stdin) == NULL) break;

    // 1. FILE SCRIPTING MODE
    if (strncmp(input, "FILE", 4) == 0) {
        FILE *f = fopen("commands.txt", "r");
        if (f) {
            memset(batch_buffer, 0, sizeof(batch_buffer));
            fread(batch_buffer, 1, sizeof(batch_buffer), f);
            fclose(f);
            send(sock, batch_buffer, strlen(batch_buffer), 0);
            printf("[System] Script sent.\n");
        } else {
            printf("[Error] commands.txt not found!\n");
        }
    } 
    // 2. TRIGGER SYNC (SEND BATCH)
    else if (strncmp(input, "SYNC", 4) == 0) {
        if (strlen(batch_buffer) > 0) {
            send(sock, batch_buffer, strlen(batch_buffer), 0);
            memset(batch_buffer, 0, sizeof(batch_buffer)); // Clear for next batch
            
            // Wait for Server Response
            MsgHeader response_header;
            int n = recv(sock, (char*)&response_header, sizeof(MsgHeader), 0);
            if (n > 0 && response_header.type == MSG_FILE_CONTENT) {
                receive_file(sock, response_header);
            }
        } else {
            printf("[!] Batch is empty. Type some commands first!\n");
        }
    } 
    // 3. ADD TO BATCH
    else {
        strcat(batch_buffer, input); 
        printf("   (Queued: %s)", input);
    }

} while (watch_interval >= 0); // Changed to >= 0 so it stays open for manual input
    
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