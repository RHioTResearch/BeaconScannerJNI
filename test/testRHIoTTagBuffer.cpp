#include <cstddef>
#include <stdio.h>
#include "../src/hcidumpinternal.h"

/**
 * Test the offset of the  struct fields for use with a direct ByteBuffer via JNI
 */
int main(int argc, char * * argv) {
    printf("sizeof(ad_data_inline) = %d\n", sizeof(ad_data_inline));
    printf("offsetof(ad_data_inline.total_length) = %d\n", offsetof(ad_data_inline, total_length));
    printf("offsetof(ad_data_inline.bdaddr_type) = %d\n", offsetof(ad_data_inline, bdaddr_type));
    printf("offsetof(ad_data_inline.bdaddr) = %d\n", offsetof(ad_data_inline, bdaddr));
    printf("offsetof(ad_data_inline.count) = %d\n", offsetof(ad_data_inline, count));
    printf("offsetof(ad_data_inline.rssi) = %d\n", offsetof(ad_data_inline, rssi));
    printf("offsetof(ad_data_inline.time) = %d\n", offsetof(ad_data_inline, time));
    printf("offsetof(ad_data_inline.data) = %d\n", offsetof(ad_data_inline, data));
}
