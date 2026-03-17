#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdarg.h>
#include <direct.h>
#include <signal.h>
#include <windows.h> // Thêm thư viện này để dùng hàm Sleep()
#include "common.h" 

#pragma comment(lib, "ws2_32.lib")

#define PORT 5000
#define MAX_FILES 100
#define BUFFER_SIZE 1024
#define SERVER_DIR "./server_data/"

FileMeta server_files[MAX_FILES];
int server_file_count = 0;
FILE *log_file = NULL; 

// --- HỆ THỐNG BẮT LỖI CHỐNG VĂNG APP ---
void crash_handler(int sig) {
    printf("\n==================================================\n");
    printf("[!!!] PHAT HIEN SERVER BI CRASH - MA LOI: %d [!!!]\n", sig);
    printf("He thong da bi dong bang de ban xem loi thay vi tu tat!\n");
    printf("==================================================\n");
    if (log_file) fclose(log_file);
    system("pause"); 
    exit(1);
}

// Hàm ghi log
void write_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    if (log_file != NULL) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        fflush(log_file); 
        va_end(args);
    }
}

// Hàm quét thư mục
void scan_server_directory() {
    DIR *dir = opendir(SERVER_DIR);
    if (!dir) {
        write_log("Thu muc %s chua ton tai. Dang tao moi...\n", SERVER_DIR);
        _mkdir(SERVER_DIR); 
        return;
    }

    struct dirent *entry;
    server_file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (server_file_count >= MAX_FILES) break; 

        char path[512];
        snprintf(path, sizeof(path), "%s%s", SERVER_DIR, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && (st.st_mode & S_IFREG)) {
            strncpy(server_files[server_file_count].filename, entry->d_name, MAX_PATH - 1);
            server_files[server_file_count].size = st.st_size;
            server_files[server_file_count].mtime = st.st_mtime;
            server_file_count++;
        }
    }
    closedir(dir);
}

// Hàm xử lý lệnh (Đã sửa int client_fd thành SOCKET client_fd cho chuẩn Windows)
void compare(char* temp, SOCKET client_fd) {
    char request[256] = {0};
    char file_name[256] = {0};
    char path[512] = {0}; 

    if (sscanf(temp, "%254s %254s", request, file_name) != 2) {
        write_log("[-] Loi: Sai cu phap lenh hoac ranh gioi (%s)\n", temp);
        return;
    }

    write_log("\n[ YEU CAU ] %s | File: %s\n", request, file_name);
    snprintf(path, sizeof(path), "%s%s", SERVER_DIR, file_name);
    
    // 1. DOWNLOAD
    if (strcmp(request, "DOWNLOAD") == 0) {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL) {
            write_log("[-] Loi: Khong tim thay file %s.\n", file_name);
            return;
        }

        fseek(fp, 0, SEEK_END);
        uint32_t file_size = (uint32_t)ftell(fp);
        fseek(fp, 0, SEEK_SET);

        MsgHeader header;
        memset(&header, 0, sizeof(header));
        header.type = MSG_FILE_CONTENT;
        header.payload_size = file_size;
        strncpy(header.filename, file_name, MAX_PATH - 1);
        
        send(client_fd, (char*)&header, sizeof(MsgHeader), 0);

        char send_buf[4096];
        int n;
        while ((n = fread(send_buf, 1, sizeof(send_buf), fp)) > 0) {
            int sent = send(client_fd, send_buf, n, 0);
            if (sent == SOCKET_ERROR) {
                write_log("[-] Loi: Client ngat ket noi khi dang tai file!\n");
                break;
            }
        }
        
        fclose(fp);
        write_log("[+] Hoan tat lenh DOWNLOAD: %s\n", file_name);
    }
    // 2. UPLOAD
    else if (strcmp(request, "UPLOAD") == 0) {
        FILE *fp = fopen(path, "wb");
        if (fp == NULL) {
            write_log("[-] Loi: Khong the tao file %s\n", file_name);
            return;
        }

        write_log("[+] Dang nhan file...\n");
        char recv_buf[BUFFER_SIZE];
        int bytes_received;
        
        while ((bytes_received = recv(client_fd, recv_buf, BUFFER_SIZE, 0)) > 0) {
            int written = fwrite(recv_buf, 1, bytes_received, fp);
            if (written < bytes_received) {
                write_log("[-] Loi hdh: Khong the ghi them vao o cung!\n");
                break;
            }
        }

        fclose(fp);
        write_log("[+] Hoan tat nhan file: %s!\n", file_name);
    }
}

int main() {
    // KÍCH HOẠT HỘP ĐEN BẮT LỖI
    signal(SIGSEGV, crash_handler); 
    signal(SIGILL, crash_handler);  
    signal(SIGFPE, crash_handler);  
    signal(SIGABRT, crash_handler); 

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

    log_file = fopen("server_log.txt", "a");

    // SỬA LỖI 1: BẮT BUỘC dùng kiểu `SOCKET` thay vì `int`
    SOCKET server_fd, client_fd; 
    struct sockaddr_in addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        write_log("[-] Loi: Khong the tao socket!\n");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return 1;
    if (listen(server_fd, 5) == SOCKET_ERROR) return 1;

    scan_server_directory();
    write_log("=========================================\n");
    write_log("   SERVER DANG CHAY TREN PORT %d\n", PORT);
    write_log("=========================================\n\n");

    char spinner[] = {'|', '/', '-', '\\'};
    int spin_idx = 0;

    while (1) {
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 150000; 

        printf("\r Dang cho client ket noi... %c ", spinner[spin_idx]);
        fflush(stdout); 
        spin_idx = (spin_idx + 1) % 4;

        int activity = select(0, &readfds, NULL, NULL, &tv);

        // SỬA LỖI 2: CHỐT CHẶN CHỐNG TRÀN TERMINAL
        if (activity == SOCKET_ERROR) {
            write_log("\n[-] Loi he thong select(): MA LOI %d. Dang thu lai...\n", WSAGetLastError());
            Sleep(1000); 
            continue; 
        }

        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            printf("\r Dang cho client ket noi... XONG! \n"); 

            int addr_len = sizeof(addr); 
            client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
            
            if (client_fd == INVALID_SOCKET) continue;

            write_log("\n>>> CLIENT CONNECTED! <<<\n");
            scan_server_directory(); 

            char buffer[BUFFER_SIZE];
            char temp[4096] = {0}; 
            int bytes;

            while ((bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0)) > 0) { 
                buffer[bytes] = '\0'; 
                
                if (strlen(temp) + bytes < sizeof(temp)) {
                    strcat(temp, buffer);
                } else {
                    write_log("[-] Bo dem tran! Dang xoa...\n");
                    memset(temp, 0, sizeof(temp));
                    continue; 
                }
               
                char *line;
                while ((line = strchr(temp, '\n')) != NULL) { 
                    *line = '\0'; 
                    compare(temp, client_fd);
                    memmove(temp, line + 1, strlen(line + 1) + 1);
                }
            }

            closesocket(client_fd);
            write_log("<<< Client da ngat ket noi. >>>\n\n");
        }
    }

    if (log_file) fclose(log_file);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
