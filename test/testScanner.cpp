#include <src/hcidumpinternal.h>

static bool callback(beacon_info *info) {

}

int main(int argc, char **argv) {
    if(argc == 2 && argv[1][1] == 'd')
        hcidumpDebugMode = true;
    scan_frames(0, callback);
}
