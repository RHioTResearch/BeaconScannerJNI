#ifndef hcidumpinternal_H
#define hcidumpinternal_H

#include <stdbool.h>
#include <stdint.h>
#include <vector>

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
    int8_t length;
    int8_t type;
    uint8_t *data;
} ad_structure;
typedef struct ad_data {
    /** The rssi of the advertising packet */
    int32_t rssi;
    /** The time the advertising packet was received */
    int64_t time;
    /** The advertising data structures in the packet */
    std::vector<ad_structure> data;
} ad_data;

// The legacy callback function invoked for each beacon event seen by hcidumpinternal
// const char * uuid, int32_t code, int32_t manufacturer, int32_t major, int32_t minor, int32_t power, int32_t rssi, int64_t time
//typedef const beacon_info *beacon_info_stack_ptr;
typedef bool (*beacon_event)(beacon_info *);

/**
 * The general callback that provides acess to all ad_structures seen in an advertising packet
 */
typedef bool (*ad_event)(ad_data& data);

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
int32_t scan_for_ad_events(int32_t dev, ad_event callback);

#endif
