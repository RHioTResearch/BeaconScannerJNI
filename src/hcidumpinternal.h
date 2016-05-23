#ifndef hcidumpinternal_H
#define hcidumpinternal_H

#include <stdbool.h>
#include <stdint.h>
#include <vector>
#include <functional>
#include <cstring>

// The size of the uuid in the manufacturer data
#define UUID_SIZE 16
// The minimum size of manufacturer data we are interested in. This consists of:
// manufacturer(2), code(2), uuid(16), major(2), minor(2), calibrated power(1)
#define MIN_MANUFACTURER_DATA_SIZE (2+2+UUID_SIZE+2+2+1)

/**
 * The structure for a beacon type of packet in a manufactuer's specific ad structure
 */
typedef struct beacon_info {
    char uuid[36];
    bool isHeartbeat;
    int32_t count;
    int32_t code;
    int32_t manufacturer;
    int32_t major;
    int32_t minor;
    int32_t power;
    int32_t calibrated_power;
    int32_t rssi;
    int64_t time;
} beacon_info;

/**
 * The generic BLE AD structure used in advertising packets
 */
typedef struct ad_structure {
    /** The actual length the data, overall length of structure is length+2 */
    uint8_t length;
    uint8_t type;
    uint8_t data[31];
} ad_structure;

typedef struct ad_data {
    /** The type of the bdaddr; 0 = Public, 1 = Random, other = Reserved */
    uint8_t	bdaddr_type;
    /** The address of the advertising packet */
    uint8_t bdaddr[6];
    /** The rssi of the advertising packet */
    int32_t rssi;
    /** The time the advertising packet was received */
    int64_t time;
    /** The advertising data structures in the packet */
    std::vector<ad_structure*> data;
} ad_data;

/**
 * A version of ad_data that inlines the ad_structures in the data array. This is used to have a contiguous in
 * memory structure that is easily passed between c and java via jni and ByteBuffer.
 */
typedef struct ad_data_inline {
    /** The totalLength of the inline data including all data[] content */
    uint32_t total_length;
    /** The type of the bdaddr; 0 = Public, 1 = Random, other = Reserved */
    uint8_t	bdaddr_type;
    /** The address of the advertising packet */
    uint8_t bdaddr[6];
    /** The count of the data[] elements */
    uint8_t count;
    /** The rssi of the advertising packet */
    int32_t rssi;
    /** The time the advertising packet was received */
    int64_t time;
    /** The advertising data structures in the packet */
    ad_structure data[];
} ad_data_inline;

/**
 * inline function to transform the ad_data into an ad_data_inline structure. The returned pointer must
 * be freed using the free() function.
 */
static inline ad_data_inline* toInline(ad_data& event) {
    // Calculate the inline length
    uint16_t totalLength = sizeof(ad_data_inline);
    for (std::vector<ad_structure*>::iterator it = event.data.begin(); it != event.data.end(); ++it) {
        ad_structure &ads = *(*it);
        // Length does not include the type and length field itself
        totalLength += ads.length + 2;
    }
    uint8_t *tmp = (uint8_t *) malloc(totalLength);
    ad_data_inline* event_inline = (ad_data_inline*) tmp;
    event_inline->total_length = totalLength;
    // Copy the ad_data fields up to count field
    event_inline->bdaddr_type = event.bdaddr_type;
    memcpy(event_inline->bdaddr, event.bdaddr, sizeof(event.bdaddr));
    event_inline->count = event.data.size();
    event_inline->rssi = event.rssi;
    event_inline->time = event.time;

    // Copy the incoming vector<ad_structure> to output ad_structure[]
    ad_structure* adsPtr = (ad_structure*) (tmp+sizeof(ad_data_inline));
    for (std::vector<ad_structure*>::iterator it = event.data.begin(); it != event.data.end(); ++it) {
        ad_structure &ads = *(*it);
        memcpy(adsPtr, &ads, ads.length+2);
        uint8_t *start = (uint8_t*) adsPtr;
        uint8_t *end = start + ads.length+2;
        adsPtr = (ad_structure *) end;
    }
    return event_inline;
}

// The legacy callback function invoked for each beacon event seen by hcidumpinternal
// const char * uuid, int32_t code, int32_t manufacturer, int32_t major, int32_t minor, int32_t power, int32_t rssi, int64_t time
//typedef const beacon_info *beacon_info_stack_ptr;
typedef bool (*beacon_event)(beacon_info *);

// Debug mode flag
extern bool hcidumpDebugMode;

#ifdef __cplusplus
extern "C" {
#endif
// The legacy function hcidumpinternal exports for beacon only callbacks
int32_t scan_frames(int32_t dev, beacon_event callback);

#ifdef __cplusplus
}
#endif

// The generic function hcidumpinternal exports for viewing complete advertising packet callbacks
int32_t scan_for_ad_events(int32_t dev, std::function<bool(ad_data&)> callback);

// The generic function hcidumpinternal exports for viewing complete advertising packet callbacks as inline data
int32_t scan_for_ad_events_inline(int32_t dev, std::function<bool(ad_data_inline&)> callback);

#endif
