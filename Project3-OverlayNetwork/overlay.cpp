#include "overlay.h"
#include <string.h>

void printUsage() {
    printf("Usage: ./overlay <mode> ...\n where mode in [router, end-host]");
}

int main(int argc, char** args) {
    if (argc <= 1) {
        printUsage();
        exit(1);
    }
    if (argc > 1) {
        if (strcmp(args[1], "router") == 0) {
            printf("Running in router mode\n");
        }
        else if (strcmp(args[1], "end-host") == 0) {
            printf("Running as end-host\n");
        }
        else {
            printf("Argument \"%s\" does not match one of [router, end-host]\n", args[1]);
        }
    }
    return 0;
}
