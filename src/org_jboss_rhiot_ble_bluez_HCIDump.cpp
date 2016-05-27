#include <string.h>
#include <stdlib.h>
#include "org_jboss_rhiot_ble_bluez_HCIDump.h"
#include "hcidumpinternal.h"
#include <chrono>
#include <thread>
using namespace std;

extern JavaVM *theVM;
static jboolean useAdData = true;
// the beacon_info pointer shared with java as a direct ByteBuffer when using the beacon oriented scanner
static beacon_info *javaBeaconInfo;
// The ad_data_inline pointer shared with java as a direct ByteBuffer when using the general scanner
static ad_data_inline *javaAdData;

static jobject byteBufferObj;
// a cached object handle to the org.jboss.summit2015.ble.bluez.HCIDump class
static jclass hcidumpClass;
// The JNIEnv passed to the initBuffer native method
static JNIEnv *javaEnv = nullptr;
static jmethodID eventNotification;

// The callback for the
extern "C" bool ble_event_callback_to_java(beacon_info * info);
extern "C" bool ble_ad_event_callback_to_java(ad_data_inline& info);

/**
 * Called by the scanner thread entry points to attach the thread to the JavaVM and allocate the
 * direct ByteBuffer to javaBeaconInfo mapping.
 */
static void attachToJavaVM() {
    if(javaEnv == nullptr) {
        // We need to attach this thread the to jvm
        int status;
        if ((status = theVM->GetEnv((void**)&javaEnv, JNI_VERSION_1_8)) < 0) {
            if ((status = theVM->AttachCurrentThreadAsDaemon((void**)&javaEnv, nullptr)) < 0) {
                return;
            }
        }
        //
        if(useAdData) {
            javaAdData = (ad_data_inline *) javaEnv->GetDirectBufferAddress(byteBufferObj);
            memset(javaAdData, 0, sizeof(ad_data_inline));
        } else {
            javaBeaconInfo = (beacon_info *) javaEnv->GetDirectBufferAddress(byteBufferObj);
            memset(javaBeaconInfo, 0, sizeof(beacon_info));
        }
    }
}

/**
 * Simple test event generator function to validate callback into java
 */
static void eventGenerator() {
    attachToJavaVM();
    beacon_info info;
    // Initialize common data
    strcpy(info.uuid, "DAF246CEF20111E4B116123B93F75CBA");
    info.calibrated_power = -38;
    info.code = 1;
    info.isHeartbeat = false;
    info.count = 0;
    info.major = 12345;
    info.minor = 11111;
    info.power = -48;
    info.rssi = -50;
    for(int n = 0; n < 100; n ++) {
        chrono::milliseconds now = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
        info.count ++;
        info.time = now.count();
        ble_event_callback_to_java(&info);
    }
}

// Flag to cause wait loop for debugger to attach to the java process
static bool waiting = false;

/**
 * The thread entry point for running the hcidump scanning loop
 * @param device the numeric value of the host controller interface instance to scan
 */
static void runScanner(int device) {
    // Connect this thread to the JavaVM instance
    attachToJavaVM();
    while(waiting)
        this_thread::yield();
    if(useAdData)
        scan_for_ad_events_inline(device, ble_ad_event_callback_to_java);
    else
        scan_frames(device, ble_event_callback_to_java);
}

JNIEXPORT void JNICALL Java_org_jboss_rhiot_ble_bluez_HCIDump_allocScanner
        (JNIEnv *env, jclass clazz, jobject bb, jint device, jboolean isGeneral) {
    printf("begin Java_org_jboss_rhiot_ble_bluez_HCIDump_allocScanner(%x,%x,%x)\n", env, clazz, bb);
    // Create global references to the ByteBuffer and HCIDump class for use in other native threads
    byteBufferObj = (jobject) env->NewGlobalRef(bb);
    hcidumpClass = (jclass) env->NewGlobalRef(clazz);
    eventNotification = env->GetStaticMethodID(hcidumpClass, "eventNotification", "()Z");
    if(eventNotification == nullptr) {
        fprintf(stderr, "Failed to lookup eventNotification()Z on: jclass=%s", clazz);
        exit(1);
    }
    printf("Found eventNotification=%x\n", eventNotification);

    thread::id tid;
#if 0
    // Simple test thread
    thread t(eventGenerator, device);
#else
    // Run the scanner
    thread t(runScanner, device);
#endif
    tid = t.get_id();
    t.detach();
    printf("end Java_org_jboss_rhiot_ble_bluez_HCIDump_allocScanner, tid=%x, hcidumpClassRef=%x, eventNotification=%x\n", tid, hcidumpClass, eventNotification);
}

JNIEXPORT void JNICALL Java_org_jboss_rhiot_ble_bluez_HCIDump_freeScanner
        (JNIEnv *env, jclass clazz) {
    printf("begin Java_org_jboss_rhiot_ble_bluez_HCIDump_freeScanner(%x,%x)\n", env, clazz);
    javaEnv->DeleteGlobalRef(hcidumpClass);
    javaEnv->DeleteGlobalRef(byteBufferObj);
}

/*
 * Class:     org_jboss_rhiot_ble_bluez_HCIDump
 * Method:    enableDebugMode
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_org_jboss_rhiot_ble_bluez_HCIDump_enableDebugMode
        (JNIEnv *env, jclass clazz, jboolean flag) {

    hcidumpDebugMode = flag;
}

/**
* Callback invoked by the hdidumpinternal.c code when a LE_ADVERTISING_REPORT event is seen on the stack. This
 * passes the event info back to java via the javaAdInfo/javaBeaconInfo pointer and returns the stop flag
 * indicator as returned by the event notification callback return value.
*/
static long eventCount = 0;
extern "C" bool ble_event_callback_to_java(beacon_info * info) {
    if(hcidumpDebugMode) {
        printf("ble_event_callback_to_java(%ld: %s, code=%d, time=%lld)\n", eventCount, info->uuid, info->code,
               info->time);
    }
    eventCount ++;
    // Copy the event data to javaBeaconInfo
    memcpy(javaBeaconInfo, info, sizeof(*info));
    // Notify java that the buffer has been updated
    jboolean stop = javaEnv->CallStaticBooleanMethod(hcidumpClass, eventNotification);
    return stop == JNI_TRUE;
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
extern "C" bool ble_ad_event_callback_to_java(ad_data_inline& info) {
    if(hcidumpDebugMode) {
        printf("ble_ad_event_callback_to_java(%ld: %s, time=%lld)\n", eventCount, toHexString(info.bdaddr, 6), info.time);
    }

    eventCount ++;
    // Copy the event data to javaAdData
    memcpy(javaAdData, &info, info.total_length);

    // Notify java that the buffer has been updated
    jboolean stop = javaEnv->CallStaticBooleanMethod(hcidumpClass, eventNotification);
    return stop == JNI_TRUE;
}
