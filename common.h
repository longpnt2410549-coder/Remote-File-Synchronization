#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

// 1. First, define the Enum
typedef enum {
    MSG_METADATA_LIST,
    MSG_FILE_CONTENT,
    MSG_DELETE_CMD,
    MSG_SYNC_COMPLETE,
    MSG_CONFLICT_WARN
} MsgType;

// 2. Second, define the Structs
typedef struct {
    char filename[MAX_PATH];
    uint64_t size;
    long mtime;
} FileMeta;

typedef struct {
    MsgType type;
    uint32_t payload_size;
    int dry_run;
    char filename[MAX_PATH];
} MsgHeader;

// 3. LASTLY, declare the function (now it knows what MsgHeader is!)
void receive_file(int socket, MsgHeader header);

#endif