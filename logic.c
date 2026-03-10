#include <stdio.h>
#include <string.h>
#include "common.h"

void compare_and_diff(FileMeta *client_list, int client_count, 
                      FileMeta *server_list, int server_count) {
    
    printf("Comparing metadata with server ...\n\n");
    printf("Changes detected:\n");

    // 1. Check Client files against Server
    for (int i = 0; i < client_count; i++) {
        int found = 0;
        for (int j = 0; j < server_count; j++) {
            if (strcmp(client_list[i].filename, server_list[j].filename) == 0) {
                found = 1;
                if (client_list[i].mtime > server_list[j].mtime) {
                    printf("[UP] UPLOAD %s (locally modified)\n", client_list[i].filename);
                } else if (client_list[i].mtime < server_list[j].mtime) {
                    // This covers the "newer on server" case
                    printf("[DN] DOWNLOAD %s (newer on server)\n", client_list[i].filename);
                } else {
                    printf("[=]  SKIP %s (identical)\n", client_list[i].filename);
                }
                break;
            }
        }
        if (!found) {
            printf("[UP] UPLOAD %s (new on client)\n", client_list[i].filename);
        }
    }

    // 2. Check for files that exist on Server but NOT on Client (Deletions)
    for (int j = 0; j < server_count; j++) {
        int found = 0;
        for (int i = 0; i < client_count; i++) {
            if (strcmp(server_list[j].filename, client_list[i].filename) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("[x]  DELETE %s (deleted on client, will be removed from server)\n", server_list[j].filename);
        }
    }
}