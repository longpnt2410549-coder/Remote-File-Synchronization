#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "common.h"

#pragma comment(lib, "ws2_32.lib")
void receive(SOCKET sock)
{
    char buf[1024];

    int bytes = recv(sock, buf, sizeof(buf) - 1, 0);

    if (bytes == 0)
    {
        printf("Server closed connection\n");
    }
    else if (bytes < 0)
    {
        printf("recv error: %d\n", WSAGetLastError());
    }

    if (bytes <= 0)
        return;

    buf[bytes] = '\0';
    printf("%s", buf);
}
int main(int argc, char *argv[])
{
    // Fix loi tham so dau vao
    if (argc < 2)
    {
        printf("Cach dung: [file].exe <server_ip>\n");
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Don rac bo nho
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); // Khop voi port 9001 cua server

    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // Dung argv[1]
    if (serv_addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Invalid address\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection Failed! Is your friend's server on?\n");
    }
    else
    {
        printf("CONNECTED to %s\n", argv[1]);

        // --- ĐOẠN CODE TEST BẮN DỮ LIỆU ---
        char buffer[1024] = "";
        char line[256];
        printf("1. UPLOAD \n2. DOWNLOAD\n3. UPDATE \n4. DELETE \n Enter command send to server here: ");
        printf("Nhap command (Enter 2 lan de gui):\n");

        while (1)
        {
            fgets(line, sizeof(line), stdin);

            if (strcmp(line, "\n") == 0)
            {
                if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
                {
                    break;
                }
            }

            strcat(buffer, line);
        }

        // --- PARSE TUNG LENH ---
        char temp[1024];
        strcpy(temp, buffer);

        char *cmd = strtok(temp, "\n");

        while (cmd != NULL)
        {
            char test_msg[100];
            char name_file[100];
            int status = 0;
            sscanf(cmd, "%s %s", test_msg, name_file);
            FILE *fp = fopen(name_file, "rb");
            long filesize = 0;

            if (fp)
            {
                fseek(fp, 0, SEEK_END);
                filesize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
            }

            if (strcmp(test_msg, "UPLOAD") == 0 || strcmp(test_msg, "UPDATE") == 0 )
            {
                status = 1;
            }

            char send_msg[200];
            sprintf(send_msg, "%s %s\n", test_msg, name_file);

            // luôn gửi command trước
            char send_buf[256];
            sprintf(send_buf, "%s %s %ld\n", test_msg, name_file, filesize);
            Sleep(1000);
            send(sock, send_buf, strlen(send_buf), 0);
            printf("Da gui lenh: %s", send_msg);

            // ===== UPLOAD / UPDATE =====
            if (status == 1)
            {
                // 1. Chờ server xác nhận READY để tránh dính chùm lệnh và dữ liệu
                char ack[256];
                int r = recv(sock, ack, sizeof(ack) - 1, 0);
                if (r > 0)
                {
                    ack[r] = '\0';
                    printf("Server: %s", ack);
                }

                if (!fp)
                {
                    printf("Khong mo duoc file\n");
                    return 1;
                }

                char buf[1024];
                int n;

                while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
                {
                    send(sock, buf, n, 0);
                }

                fclose(fp);
                printf("[+] Da gui xong du lieu file. Dang cho server xu ly...\n");

                // 2. BÁO SERVER ĐÃ GỬI XONG: Đóng đường gửi, nhưng VẪN MỞ đường nhận

                // 3. NHẬN TRẠNG THÁI TỪ SERVER TRẢ VỀ RỒI MỚI THOÁT
                r = recv(sock, ack, sizeof(ack) - 1, 0);
                if (r > 0)
                {
                    ack[r] = '\0';
                    printf("%s", ack);
                    
                }

                receive(sock);

            }
            // ===== DOWNLOAD =====
            else
            {
                MsgHeader response_header;

                int n = recv(sock, (char *)&response_header, sizeof(MsgHeader), 0);

                if (n > 0 && response_header.type == MSG_FILE_CONTENT)
                {
                    receive_file(sock, response_header);
                    receive(sock);
                }
            }

            int dry_run = 0;
            int watch_interval = 0;

            for (int i = 1; i < argc; i++)
            {
                if (strcmp(argv[i], "--dry-run") == 0)
                    dry_run = 1;
                if (strcmp(argv[i], "--watch") == 0 && i + 1 < argc)
                {
                    watch_interval = atoi(argv[i + 1]);
                }
            }

            if (strcmp(test_msg, "DOWNLOAD") == 0)
            {

            }
            
            receive(sock);
            cmd = strtok(NULL, "\n"); // Đừng quên dòng này để lấy lệnh tiếp theo
        }
                                receive(sock);
        WSACleanup();
        return 0; // This is the final bracket for main
    }
                        receive(sock);
}