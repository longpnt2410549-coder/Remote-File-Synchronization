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
long server_file_size = 0;
long server_file_mtime = 0;
FILE *log_file = NULL;

// --- HỆ THỐNG BẮT LỖI CHỐNG VĂNG APP ---
void crash_handler(int sig) // hàm test xem có lỗi không
{
    printf("\n==================================================\n");
    printf("[!!!] PHAT HIEN SERVER BI CRASH - MA LOI: %d [!!!]\n", sig);
    printf("He thong da bi dong bang de ban xem loi thay vi tu tat!\n");
    printf("==================================================\n");
    if (log_file)
        fclose(log_file);
    system("pause");
    exit(1);
}

// Hàm ghi log
void write_log(SOCKET client_fd, const char *format, ...) // viết log vào file
{
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("gui cho client: %s", buffer);
    Sleep(1000);
    if (client_fd==INVALID_SOCKET){
        printf("Oh no");
    }
    send(client_fd, buffer, strlen(buffer), 0);

    if (log_file != NULL)
    {
        va_start(args, format);
        vfprintf(log_file, format, args);
        fflush(log_file);
        va_end(args);
    }
}

int find_file(SOCKET client_fd, char *file) /// hàm tìm file
{
    DIR *dir = opendir(SERVER_DIR);
    if (!dir)
    {
        write_log(client_fd, "Thu muc %s chua ton tai. Dang tao moi...\n", SERVER_DIR);
        _mkdir(SERVER_DIR);
    }

    struct dirent *entry;
    server_file_count = 0;

    while ((entry = readdir(dir)) != NULL) // đọc file trong thư mục
    {
        if (entry->d_name[0] == '.')
            continue;
        if (server_file_count >= MAX_FILES) // đếm xem có nhiều hơn số file quy định k
            break;

        char path[512];
        snprintf(path, sizeof(path), "%s%s", SERVER_DIR, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && (st.st_mode & S_IFREG))
        {
            strncpy(server_files[server_file_count].filename, entry->d_name, MAX_PATH - 1);
            server_files[server_file_count].size = st.st_size;
            server_file_size = st.st_size; // ha
            server_files[server_file_count].mtime = st.st_mtime;
            if (strcmp(server_files[server_file_count].filename, file) == 0) // so sánh xem có đúng file đang cần tìm k
            {
                return 1; // khai báo là file có tồn tại và chuẩn bị để ghi
            }
            server_file_count++; // set thông tin file
        }
    }
    closedir(dir);
    return 0;
}
void scan_server_directory(SOCKET client_fd) // Hàm quét thư mục'
{
    DIR *dir = opendir(SERVER_DIR);
    if (!dir)
    {
        write_log(client_fd, "Thu muc %s chua ton tai. Dang tao moi...\n", SERVER_DIR);
        _mkdir(SERVER_DIR);
        return;
    }

    struct dirent *entry;
    server_file_count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        if (server_file_count >= MAX_FILES)
            break;

        char path[512];
        snprintf(path, sizeof(path), "%s%s", SERVER_DIR, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0 && (st.st_mode & S_IFREG))
        {
            strncpy(server_files[server_file_count].filename, entry->d_name, MAX_PATH - 1);
            server_files[server_file_count].size = st.st_size; // chỗ này đk uh
            server_files[server_file_count].mtime = st.st_mtime;
            server_file_count++;
        }
    }
    closedir(dir); // tương tự find file
}

// thêm đây
int delete_file(const char *filename, SOCKET client_fd)
{
    if (remove(filename) == 0)
    {
        return 1; // success
    }
    else
    {
        perror("[-] Loi khi xoa file");
        return 0; // fail
    }
}

// Hàm xử lý lệnh (Đã sửa int client_fd thành SOCKET client_fd cho chuẩn Windows)
void compare(char *temp, SOCKET client_fd, int i)
{
    char request[256] = {0};
    char file_name[256] = {0};
    char path[512] = {0};
    char temp_path[512];
    long filesize = 0;
    sscanf(temp, "%s %s %ld", request, file_name, &filesize);
    if (sscanf(temp, "%254s %254s", request, file_name) != 2)
    {
        write_log(client_fd, "[-] Loi: Sai cu phap lenh hoac ranh gioi (%s)\n", temp);
        return;
    }

    write_log(client_fd, "\n[YEU CAU %d] %s | File: %s\n\n", i, request, file_name);
    snprintf(path, sizeof(path), "%s%s", SERVER_DIR, file_name);
    // ================= 1. DOWNLOAD =================
    if (strcmp(request, "DOWNLOAD") == 0)
    {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL)
        {
            write_log(client_fd, "[-] Loi: Khong tim thay file %s.\n", file_name);
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

        send(client_fd, (char *)&header, sizeof(MsgHeader), 0);

        char send_buf[4096];
        int n;
        while ((n = fread(send_buf, 1, sizeof(send_buf), fp)) > 0)
        {
            int sent = send(client_fd, send_buf, n, 0);
            if (sent == SOCKET_ERROR)
                break;
        }
        write_log(client_fd, "[+] Hoan tat lenh DOWNLOAD: %s\n", file_name);
        write_log(client_fd,"OK");
        fclose(fp);
    }
    // ================= 2. DELETE  =================
    else if (strcmp(request, "DELETE") == 0)
    {
        FILE *fp = fopen(path, "rb");

        if (fp == NULL)
        {
            write_log(client_fd, "[-] Loi: Khong tim thay file %s.\n", file_name);
            return;
        }

        fclose(fp);

        if (remove(path) == 0)
        {
            write_log(client_fd, "[+] Da xoa file thanh cong!\n");
        }
        else
        {
            write_log(client_fd, "[-] Xoa file that bai!\n");
        }
    }
    // ================= 3. UPLOAD & UPDATE  =================
    else if ((strcmp(request, "UPLOAD") == 0 || strcmp(request, "UPDATE") == 0))
    {
        // 1. Gửi tín hiệu READY để Client bắt đầu ném file qua
        send(client_fd, "READY\n", 6, 0);

        snprintf(temp_path, sizeof(temp_path), "%s%s.tmp", SERVER_DIR, file_name);
        FILE *fp = fopen(temp_path, "wb");
        if (!fp)
        {
            write_log(client_fd, "[-] Khong the tao file tam\n");
            return;
        }

        write_log(client_fd, "[+] Dang nhan file...\n");

        char buf[BUFFER_SIZE];
        int bytes;

        // 2. Vòng lặp này sẽ tự động thoát khi client gọi hàm shutdown(sock, SD_SEND)
        long received = 0;
        while (received < filesize)
        {
            int bytes = recv(client_fd, buf, BUFFER_SIZE, 0);
            if (bytes <= 0)
                break;

            fwrite(buf, 1, bytes, fp);
            received += bytes;
        }

        fclose(fp);

        // 3. Xử lý logic nghiệp vụ sau khi đã nhận đủ dữ liệu
        if (find_file(client_fd, file_name)) // Đã tồn tại file trên server
        {
            if (strcmp(request, "UPDATE") == 0)
            {
                struct stat st_new, st_old;
                stat(temp_path, &st_new);
                stat(path, &st_old);

                if (st_new.st_size != st_old.st_size || st_new.st_mtime > st_old.st_mtime)
                {
                    delete_file(path, client_fd);
                    rename(temp_path, path);
                    write_log(client_fd, "[+] Da cap nhat file thanh cong!\n");
                }
                else
                {
                    write_log(client_fd, "[=] File cu hoac y het -> Bo qua khong cap nhat\n");
                    delete_file(temp_path, client_fd); // Nhớ xoá file tạm
                }
                fclose(fp);
            }
            else // Lệnh UPLOAD nhưng file đã có
            {
                write_log(client_fd, "[-] File da ton tai. Vui long dung lenh UPDATE\n");
                delete_file(temp_path, client_fd);
            }
            fclose(fp);
        }
        else // Chưa có file trên server
        {
            if (strcmp(request, "UPLOAD") == 0)
            {
                rename(temp_path, path);
                write_log(client_fd, "[+] Hoan tat luu file: %s!\n", file_name);
                fclose(fp);
            }
            else // Lệnh UPDATE nhưng chưa có file
            {
                write_log(client_fd, "[-] File %s khong ton tai de UPDATE!!\n", file_name);
                delete_file(temp_path, client_fd);
            }
        }
    }
}
void receive_request(SOCKET client_fd)
{
    char buffer[BUFFER_SIZE];
    char temp[4096];
    int temp_len = 0;
    int i = 0;
    int bytes;

    while ((bytes = recv(client_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        i++;
        if (temp_len + bytes >= sizeof(temp))
        {
            write_log(client_fd, "[-] Bo dem tran! Dang xoa...\n");
            temp_len = 0;
            continue;
        }

        memcpy(temp + temp_len, buffer, bytes);
        temp_len += bytes;
        temp[temp_len] = '\0';

        char *line;
        while ((line = strchr(temp, '\n')) != NULL)
        {
            *line = '\0';

            if (line > temp && *(line - 1) == '\r')
                *(line - 1) = '\0';
            compare(temp, client_fd, i);
            int remaining = temp_len - (line - temp + 1);
            memmove(temp, line + 1, remaining);
            temp_len = remaining;
            temp[temp_len] = '\0';
        }
    }

    write_log(client_fd, "Client disconnected\n");
}

int main()
{
    // KÍCH HOẠT HỘP ĐEN BẮT LỖI
    signal(SIGSEGV, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGABRT, crash_handler);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    log_file = fopen("server_log.txt", "a");

    // SỬA LỖI 1: BẮT BUỘC dùng kiểu `SOCKET` thay vì `int`
    SOCKET server_fd, client_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        write_log(client_fd, "[-] Loi: Khong the tao socket!\n");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
        return 1;
    if (listen(server_fd, 5) == SOCKET_ERROR)
        return 1;

    scan_server_directory(client_fd);
    printf("SERVER DANG CHAY TREN PORT %d\n", PORT);

    char spinner[] = {'|', '/', '-', '\\'};
    int spin_idx = 0;

    while (1)
    {
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
        if (activity == SOCKET_ERROR)
        {
            write_log(client_fd, "\n[-] Loi he thong select(): MA LOI %d. Dang thu lai...\n", WSAGetLastError());
            Sleep(1000);
            continue;
        }

        if (activity > 0 && FD_ISSET(server_fd, &readfds))
        {
            printf("\r Dang cho client ket noi... XONG! \n");

            int addr_len = sizeof(addr);
            client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);

            if (client_fd == INVALID_SOCKET)
                continue;

            write_log(client_fd, "\n>>> CLIENT CONNECTED! <<<\n");
            scan_server_directory(client_fd);
            receive_request(client_fd);
        }
        closesocket(client_fd);
        // write_log(client_fd, "<<< Client da ngat ket noi. >>>\n\n");
    }
    if (log_file)
        fclose(log_file);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}