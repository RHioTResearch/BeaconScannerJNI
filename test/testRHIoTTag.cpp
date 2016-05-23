#include <src/hcidumpinternal.h>
#include <memory.h>
#include <cmath>
#include <string>

// https://github.com/google/eddystone/blob/master/eddystone-tlm/tlm-plain.md
static uint8_t EDDYSTONE_UUID[] = {0xaa, 0xfe};

static inline void dump_ads(ad_structure& ads) {
    printf("ADS(%x:%d): ", ads.type, ads.length);
    for(int n = 0; n < ads.length; n ++)
        printf("%.02x", ads.data[n]);
    printf("\n");
}
static const char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

float sensorOpt3001Convert(uint16_t rawData) {
    uint16_t e, m;

    m = rawData & 0x0FFF;
    e = (rawData & 0xF000) >> 12;

    return m * (0.01 * exp2(e));
}

/*
 typedef struct
{
  uint8_t   frameType;      // 0x21 nonstandard
  uint8_t   keys;			//  Bit 0: left key (user button), Bit 1: right key (power button), Bit 2: reed relay
  uint8_t   lux[2];         // raw optical sensor data, BE
  uint8_t   gyroX[2];       // gyroscope X axis reading
  uint8_t   gyroY[2];       // gyroscope Y axis reading
  uint8_t   gyroZ[2];       // gyroscope Z axis reading
  uint8_t   accelX[2];      // acceleration X axis reading
  uint8_t   accelY[2];      // acceleration Y axis reading
  uint8_t   accelZ[2];      // acceleration Z axis reading
...
 } eddystoneMisc_t;
 */
static void printMisc(ad_structure misc) {
    uint8_t *data = misc.data;
    int index = 3;
    // Key state
    printf("keys: %s\n", byte_to_binary(data[index]));
    index ++;
    // Light sensor
    uint16_t lux = (data[index]<<8) + data[index+1];
    printf("lux: %.2f\n", sensorOpt3001Convert(lux));
}
enum KeyType {
    LEFT=1,
    RIGHT=2,
    REED=4
};

typedef struct __attribute__ ((__packed__))
{
    uint8_t   frameType;      // TLM
    uint8_t   version;        // 0x00 for now
    uint8_t   vBatt[2];       // Battery Voltage, 1mV/bit, Big Endian
    uint8_t   temp[2];        // Temperature in C. Signed 8.8 fixed point
    uint8_t   advCnt[4];      // Adv count since power-up/reboot
    uint8_t   secCnt[4];      // Time since power-up/reboot in 0.1 second resolution
    // Non-standard TLM data
    uint8_t   keys;			//  Bit 0: left key (user button), Bit 1: right key (power button), Bit 2: reed relay
    uint8_t   lux[2];         // raw optical sensor data, Big Endian
} eddystoneTLM_t;
/*
// Eddystone TLM frame + mods. This is what the SensorTagX custom firmware is sending in the TLM advert packet

*/
static void printTLM(eddystoneTLM_t* tlm) {
    // Version
    if(tlm->version != 0x0) {
        printf("Non-zero version: %d\n", tlm->version);
        return;
    }

    // VBATT
    //printf("vbatt: %d, %d => %d:%d\n", data[index], data[index+1], data[index] << 8, data[index+1]);
    uint16_t vbatt = (tlm->vBatt[0]<<8) + tlm->vBatt[1];
    // TEMP
    //printf("temp: %d, %d\n", data[index], data[index+1]);
    double temp = tlm->temp[0] + tlm->temp[1]/ 256.0;
    double tempF = 1.8*temp + 32;
    // ADV_CNT
    //printf("advCnt: %d, %d, %d, %d\n", data[index], data[index+1], data[index+2], data[index+3]);
    uint32_t advCnt = (tlm->advCnt[0]<<24) + (tlm->advCnt[1]<<16) + (tlm->advCnt[2]<<8) + tlm->advCnt[3];
    // SEC_CNT
    //printf("secCnt: %d, %d, %d, %d\n", data[index], data[index+1], data[index+2], data[index+3]);
    uint32_t secCnt = (tlm->secCnt[0]<<24) + (tlm->secCnt[1]<<16) + (tlm->secCnt[2]<<8) + tlm->secCnt[3];
    // KEYS
    uint8_t keys = tlm->keys;
    std::string tmp;
    if(keys & 0x1)
        tmp.append("Left|");
    if(keys & 0x2)
        tmp.append("Right|");
    if(keys & 0x4)
        tmp.append("Reed");
    // Light sensor
    uint16_t lux = (tlm->lux[0]<<8) + tlm->lux[1];
    printf("\tTLM: vbatt: %dmV, temp:%.2f/%.2f, advCnt: %d, secCnt: %d, lux: %d, keys: %s\n",
           vbatt, temp, tempF, advCnt, secCnt, lux, tmp.c_str());
}


static bool callback(ad_data& info) {
    printf("ad_data:{time=%ld, rssi=%d, count=%ld}\n", info.time, info.rssi, info.data.size());
    for (int i = 0; i < info.data.size(); ++i) {
        ad_structure& ads = *(info.data[i]);
        if(memcmp(ads.data, EDDYSTONE_UUID, 2) == 0 && ads.length > 4) {
            // Move past the frame length
            eddystoneTLM_t *data = (eddystoneTLM_t *) (ads.data+2);
            printf("\tRHIoTTag(%.2X:%.2X:%.2X:%.2X:%.2X:%.2X/%d):\n", info.bdaddr[0], info.bdaddr[1], info.bdaddr[2],
                info.bdaddr[3], info.bdaddr[4], info.bdaddr[5], info.bdaddr_type);
            if(data->frameType == 0x20) {
                printTLM(data);
            } else if(data->frameType == 0x21) {
                printMisc(ads);
            } else {
                printf("\tUnknown frame: %x\n\t", data[2]);
                dump_ads(ads);
            }
        }
    }
    printf("---end\n");
    return false;
}

static inline const char* toHexString(uint8_t *data, uint8_t length) {
    static char buffer[64];
    char *loc = buffer;
    for(int n = 0; n < length; n ++) {
        sprintf(loc, "%.2X:", data[n]);
        loc += 3;
    }
    *loc = 0;
    return buffer;
}
static bool inline_callback(ad_data_inline& info) {
    printf("ad_data:{time=%ld, rssi=%d, count=%d}\n", info.time, info.rssi, info.count);
    uint8_t *start = (uint8_t *) &info.data;
    for (int i = 0; i < info.count; ++i) {
        ad_structure* adsPtr = (ad_structure *) start;
        ad_structure& ads = *adsPtr;
        printf("\tAD(%d:%d): %s\n", ads.type, ads.length, toHexString(ads.data, ads.length));
        if(memcmp(ads.data, EDDYSTONE_UUID, 2) == 0 && ads.length > 4) {
            // Move past the frame length
            eddystoneTLM_t *data = (eddystoneTLM_t *) (ads.data+2);
            printf("\tRHIoTTag(%.2X:%.2X:%.2X:%.2X:%.2X:%.2X/%d):\n", info.bdaddr[0], info.bdaddr[1], info.bdaddr[2],
                   info.bdaddr[3], info.bdaddr[4], info.bdaddr[5], info.bdaddr_type);
            if(data->frameType == 0x20) {
                printTLM(data);
            } else if(data->frameType == 0x21) {
                printMisc(ads);
            } else {
                printf("\tUnknown frame: %x\n\t", data[2]);
                dump_ads(ads);
            }
        }
        start += ads.length+2;
    }
    printf("---end\n");
    return false;
}

int main(int argc, char **argv) {
    bool debug = false;
    bool useInline = false;
    int dev = 0;
    for(int n = 1; n < argc; n ++) {
        // -inline
        if(strncmp("-in", argv[n], 3) == 0)
            useInline = true;
        // -i
        else if(argv[n][1] == 'i') {
            n ++;
            dev = ::atoi(argv[n]);
        } // -d
        else if(argv[n][1] == 'd')
            debug = true;
    }
    hcidumpDebugMode = debug;


    if(useInline)
        scan_for_ad_events_inline(dev, inline_callback);
    else
        scan_for_ad_events(dev, callback);
}
