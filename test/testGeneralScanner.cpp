#include <src/hcidumpinternal.h>

static bool callback(ad_data& info) {
    printf("ad_data:{time=%d, rssi=%d, count=%d}\n", info.time, info.rssi, info.data.size());
}

int main(int argc, char **argv) {
    if(argc == 2 && argv[1][1] == 'd')
        hcidumpDebugMode = true;
    scan_frames(0, callback);
}
