// client.c snippet
#include <unistd.h> // for sleep()

int main(int argc, char *argv[]) {
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
            sleep(watch_interval);
        }
    } while (watch_interval > 0);

    return 0;
}

