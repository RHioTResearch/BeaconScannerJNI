#include <src/hcidumpinternal.h>
#include <cstring>

static bool callback(ad_data& info) {
    printf("ad_data:{time=%ld, rssi=%d, count=%ld}\n", info.time, info.rssi, info.data.size());
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
    scan_for_ad_events(dev, callback);
}
