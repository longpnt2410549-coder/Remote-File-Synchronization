// common.c
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "common.h"

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