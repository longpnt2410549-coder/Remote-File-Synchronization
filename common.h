// common.h
#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

typedef enum {
    MSG_METADATA_LIST,
    MSG_FILE_CONTENT,
    MSG_DELETE_CMD,
    MSG_SYNC_COMPLETE,
    MSG_CONFLICT_WARN  // Added for Bonus: Conflict detection
} MsgType;

typedef struct {
    char filename[MAX_PATH];
    uint64_t size;
    long mtime; 
} FileMeta;

typedef struct {
    MsgType type;
    uint32_t payload_size;
    int dry_run;       // Added for Bonus: Tells receiver not to write to disk
} MsgHeader;

#endif