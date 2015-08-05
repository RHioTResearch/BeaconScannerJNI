#include <src/hcidumpinternal.h>
#include <stdio.h>

static bool callback(beacon_info *info) {
    printf("beacon_info:{uuid=%s, major=%d, minor=%d}\n", info->uuid, info->major, info->minor);
}

int main(int argc, char **argv) {
    if(argc == 2 && argv[1][1] == 'd')
        hcidumpDebugMode = true;
    scan_frames(0, callback);
}
