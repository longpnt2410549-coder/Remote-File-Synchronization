#include <stdio.h>     // For snprintf
#include <winsock2.h>
#include <dirent.h>    // For opendir/readdir
#include <sys/stat.h>  // For stat
#include <string.h>    // For strncpy
#include "common.h"    // For FileMeta definition
#include <shellapi.h>

void send_file(int socket, char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;

    char buffer[4096];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send(socket, buffer, bytes_read, 0);
    }
    fclose(f);
}

int get_local_metadata(const char *dir_path, FileMeta *list) {
    DIR *d = opendir(dir_path);
    if (!d) return -1;

    struct dirent *dir;
    struct stat st;
    int count = 0;

    while ((dir = readdir(d)) != NULL) {
        // Skip hidden files and directories (like "." and "..")
        if (dir->d_name[0] == '.') continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, dir->d_name);

        if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            strncpy(list[count].filename, dir->d_name, MAX_PATH);
            list[count].size = (uint64_t)st.st_size;
            list[count].mtime = (long)st.st_mtime;
            count++;
        }
    }
    closedir(d);
    return count;
}

void receive_file(int socket, MsgHeader header) {
    char buffer[4096];
    FILE *f = fopen(header.filename, "wb"); // Open for Writing in Binary
    if (!f) return;

    uint32_t bytes_received = 0;
    while (bytes_received < header.payload_size) {
        int expected = (header.payload_size - bytes_received < 4096) ? 
                        header.payload_size - bytes_received : 4096;
        
        int n = recv(socket, buffer, expected, 0);
        if (n <= 0) break;
        
        fwrite(buffer, 1, n, f);
        bytes_received += n;
    }
    fclose(f);
    printf("Successfully downloaded %s (%u bytes)\n", header.filename, header.payload_size);
    
    // Bonus: Open it automatically (Windows specific)
    ShellExecuteA(NULL, "open", header.filename, NULL, NULL, SW_SHOWNORMAL);
}
