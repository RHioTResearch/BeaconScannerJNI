#include <src/hcidumpinternal.h>

static bool callback(beacon_info *info) {

}

int main() {
    scan_frames(1, callback);
}
