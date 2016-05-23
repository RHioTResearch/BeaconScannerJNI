#include <src/hcidumpinternal.h>
#include <stdio.h>
#include <cstring>

static bool callback(beacon_info *info) {
    printf("beacon_info:{uuid=%s, major=%d, minor=%d}\n", info->uuid, info->major, info->minor);
}

int main(int argc, char **argv) {
    bool debug = false;
    int dev = 0;
    for(int n = 1; n < argc; n ++) {
        if(strncmp("-i", argv[n], 2) == 0) {
            n ++;
            dev = ::atoi(argv[n]);
        }
        if(argv[n][1] == 'd')
            debug = true;
    }
    hcidumpDebugMode = debug;
    scan_frames(dev, callback);
}
