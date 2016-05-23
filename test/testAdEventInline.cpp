#include <stdio.h>
#include <src/hcidumpinternal.h>

/**
 * Test inlining the ad_event structure to ad_event_inline for use in passing between c and java via ByteBuffer.
 */
int main(int argc, char **argv) {
    ad_data orig;
    orig.bdaddr_type = 1;
    orig.rssi = -50;
    orig.time = 1463720753386;
    orig.bdaddr[0] = 0x85;
    orig.bdaddr[1] = 0xDA;
    orig.bdaddr[2] = 0xD6;
    orig.bdaddr[3] = 0x48;
    orig.bdaddr[4] = 0xB4;
    orig.bdaddr[5] = 0xB0;
    ad_structure ads0 = {1, 1, {0x4}};
    ad_structure ads1 = {2, 3, {0xaa, 0xfe}};
    ad_structure ads2 = {16, 9, {0x43,0x43,0x32,0x36,0x35,0x30,0x20,0x53,0x65,0x6e,0x73,0x6f,0x72,0x54,0x61,0x67}};
    orig.data.push_back(&ads0);
    orig.data.push_back(&ads1);
    orig.data.push_back(&ads2);

    ad_data_inline *test = toInline(orig);
    if(test->total_length != (sizeof(ad_data_inline)+3+5+11))
        printf("Failed on total_length\n");
    if(test->bdaddr_type != orig.bdaddr_type)
        printf("Failed on baddr_type\n");
    if(test->rssi != orig.rssi)
        printf("Failed on rssi\n");
    if(test->time != orig.time)
        printf("Failed on time\n");
    if(memcmp(test->bdaddr, orig.bdaddr, sizeof(test->bdaddr)) != 0)
        printf("Failed on bdaddr\n");
    if(test->count != orig.data.size())
        printf("Failed on count\n");

    printf("sizeof(ad_data_inline)=%d\n", sizeof(ad_data_inline));
    uint8_t *start = (uint8_t *) test;
    uint8_t *end = start + sizeof(ad_data_inline);
    ad_structure* adsPtr = (ad_structure *)end;
    uint64_t diff = (uint8_t *)adsPtr - (uint8_t *)test;
    printf("adsPtr - test = %ld\n", diff);

    if(memcmp(adsPtr, &ads0, ads0.length+2) != 0)
        printf("Failed on ads0\n");
    adsPtr = (ad_structure *)(((uint8_t*)adsPtr) + adsPtr->length+2);
    if(memcmp(adsPtr, &ads1, ads1.length+2) != 0)
        printf("Failed on ads1\n");
    adsPtr = (ad_structure *)(((uint8_t*)adsPtr) + adsPtr->length+2);
    if(memcmp(adsPtr, &ads2, ads2.length+2) != 0)
        printf("Failed on ads2\n");

    free(test);

    return 0;
}
