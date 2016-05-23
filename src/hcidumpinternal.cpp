#include "hcidumpinternal.h"

/*
 *  Code from the bluez tools/hcidump.c
 *  Copyright 2015-2016 Red Hat
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2002  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2003-2011  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <functional>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ctype.h>

extern "C" {
#include "parser.h"
#include "sdp.h"
#include <bluetooth.h>
#include <hci.h>

#include "hci_lib.h"
}

#define SNAP_LEN	HCI_MAX_FRAME_SIZE

/* Modes */
enum {
    PARSE,
    READ,
    WRITE,
    PPPDUMP,
    AUDIO
};

// A stop flag
bool stop_scan_frames = false;

// Debug mode flag
bool hcidumpDebugMode = false;

/* Default options */
static int  snap_len = SNAP_LEN;

struct hcidump_hdr {
    uint16_t	len;
    uint8_t		in;
    uint8_t		pad;
    uint32_t	ts_sec;
    uint32_t	ts_usec;
} __attribute__ ((packed));
#define HCIDUMP_HDR_SIZE (sizeof(struct hcidump_hdr))

struct btsnoop_hdr {
    uint8_t		id[8];		/* Identification Pattern */
    uint32_t	version;	/* Version Number = 1 */
    uint32_t	type;		/* Datalink Type */
} __attribute__ ((packed));
#define BTSNOOP_HDR_SIZE (sizeof(struct btsnoop_hdr))

struct btsnoop_pkt {
    uint32_t	size;		/* Original Length */
    uint32_t	len;		/* Included Length */
    uint32_t	flags;		/* Packet Flags */
    uint32_t	drops;		/* Cumulative Drops */
    uint64_t	ts;		/* Timestamp microseconds */
    uint8_t		data[0];	/* Packet Data */
} __attribute__ ((packed));
#define BTSNOOP_PKT_SIZE (sizeof(struct btsnoop_pkt))

static uint8_t btsnoop_id[] = { 0x62, 0x74, 0x73, 0x6e, 0x6f, 0x6f, 0x70, 0x00 };

static uint32_t btsnoop_version = 0;
static uint32_t btsnoop_type = 0;

struct pktlog_hdr {
    uint32_t	len;
    uint64_t	ts;
    uint8_t		type;
} __attribute__ ((packed));
#define PKTLOG_HDR_SIZE (sizeof(struct pktlog_hdr))

static int open_socket(int dev)
{
    struct sockaddr_hci addr;
    struct hci_filter flt;
    int sk, opt;

    /* Create HCI socket */
    sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (sk < 0) {
        perror("Can't create raw socket");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
        perror("Can't enable data direction info");
        return -1;
    }

    opt = 1;
    if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0) {
        perror("Can't enable time stamp");
        return -1;
    }

    /* Setup filter */
    hci_filter_clear(&flt);
    hci_filter_all_ptypes(&flt);
    hci_filter_all_events(&flt);
    if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
        perror("Can't set filter");
        return -1;
    }

    /* Bind socket to the HCI device */
    memset(&addr, 0, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = dev;
    if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("Can't attach to device hci%d. %s(%d)\n",
                dev, strerror(errno), errno);
        return -1;
    }

    return sk;
}

#define EVENT_NUM 77
static char const *event_str[EVENT_NUM + 1] = {
        "Unknown",
        "Inquiry Complete",
        "Inquiry Result",
        "Connect Complete",
        "Connect Request",
        "Disconn Complete",
        "Auth Complete",
        "Remote Name Req Complete",
        "Encrypt Change",
        "Change Connection Link Key Complete",
        "Master Link Key Complete",
        "Read Remote Supported Features",
        "Read Remote Ver Info Complete",
        "QoS Setup Complete",
        "Command Complete",
        "Command Status",
        "Hardware Error",
        "Flush Occurred",
        "Role Change",
        "Number of Completed Packets",
        "Mode Change",
        "Return Link Keys",
        "PIN Code Request",
        "Link Key Request",
        "Link Key Notification",
        "Loopback Command",
        "Data Buffer Overflow",
        "Max Slots Change",
        "Read Clock Offset Complete",
        "Connection Packet Type Changed",
        "QoS Violation",
        "Page Scan Mode Change",
        "Page Scan Repetition Mode Change",
        "Flow Specification Complete",
        "Inquiry Result with RSSI",
        "Read Remote Extended Features",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Unknown",
        "Synchronous Connect Complete",
        "Synchronous Connect Changed",
        "Sniff Subrate",
        "Extended Inquiry Result",
        "Encryption Key Refresh Complete",
        "IO Capability Request",
        "IO Capability Response",
        "User Confirmation Request",
        "User Passkey Request",
        "Remote OOB Data Request",
        "Simple Pairing Complete",
        "Unknown",
        "Link Supervision Timeout Change",
        "Enhanced Flush Complete",
        "Unknown",
        "User Passkey Notification",
        "Keypress Notification",
        "Remote Host Supported Features Notification",
        "LE Meta Event",
        "Unknown",
        "Physical Link Complete",
        "Channel Selected",
        "Disconnection Physical Link Complete",
        "Physical Link Loss Early Warning",
        "Physical Link Recovery",
        "Logical Link Complete",
        "Disconnection Logical Link Complete",
        "Flow Spec Modify Complete",
        "Number Of Completed Data Blocks",
        "AMP Start Test",
        "AMP Test End",
        "AMP Receiver Report",
        "Short Range Mode Change Complete",
        "AMP Status Change",
};

#define LE_EV_NUM 5
static char const *ev_le_meta_str[LE_EV_NUM + 1] = {
        "Unknown",
        "LE Connection Complete",
        "LE Advertising Report",
        "LE Connection Update Complete",
        "LE Read Remote Used Features Complete",
        "LE Long Term Key Request",
};

static const char *bdaddrtype2str(uint8_t type)
{
    switch (type) {
        case 0x00:
            return "Public";
        case 0x01:
            return "Random";
        default:
            return "Reserved";
    }
}

static const char *evttype2str(uint8_t type)
{
    switch (type) {
        case 0x00:
            return "ADV_IND - Connectable undirected advertising";
        case 0x01:
            return "ADV_DIRECT_IND - Connectable directed advertising";
        case 0x02:
            return "ADV_SCAN_IND - Scannable undirected advertising";
        case 0x03:
            return "ADV_NONCONN_IND - Non connectable undirected advertising";
        case 0x04:
            return "SCAN_RSP - Scan Response";
        default:
            return "Reserved";
    }
}

char toHexChar(int b) {
    char c = '0';
    if(b < 10)
        c = '0' + b;
    else
        c = 'A' + b - 10;
    return c;
}
static inline void print_bytes(uint8_t *data, u_int8_t len) {
    int n;
    printf("{");
    for(n = 0; n < len; n ++) {
        printf("0x%x,", data[n]);
    }
    printf("};\n");
}
static inline void hex_debug(int level, struct frame *frm)
{
    unsigned char *buf = (unsigned char*) frm->ptr;
    register int i, n;
    int num = frm->len;

    for (i = 0, n = 1; i < num; i++, n++) {
        if (n == 1)
            p_indent(level, frm);
        printf("%2.2X ", buf[i]);
        if (n == DUMP_WIDTH) {
            printf("\n");
            n = 0;
        }
    }
    if (i && n != 1)
        printf("\n");
}

static inline void dump_ads(ad_structure& ads) {
    printf("ADS(%x:%d): ", ads.type, ads.length);
    for(int n = 0; n < ads.length; n ++)
        printf("%.02x", ads.data[n]);
    printf("\n");
}

/**
* Parse the AD Structure values found in the AD payload
*/
static inline void ext_inquiry_data_dump(int level, struct frame *frm, uint8_t *data, ad_data& info) {
    ad_structure *adsPtr;
    char *str;
    int i;

    int length = data[0];
    adsPtr = (ad_structure *) malloc(length+2);
    adsPtr->type = data[1];
    // Do not include the checksum
    adsPtr->length = length-1;
    info.data.push_back(adsPtr);
    if (length <= 1)
        return;

    ad_structure& ads = *adsPtr;
    data += 2;
    memcpy(ads.data, data, ads.length);
    if(hcidumpDebugMode)
        dump_ads(ads);

    // Just for debugging
    switch (ads.type) {
        case 0x01:
        if(hcidumpDebugMode) {
            p_indent(level, frm);
            printf("Flags:");
            for (i = 0; i < ads.length; i++)
                printf(" 0x%2.2x", data[i]);
            printf("\n");
        }
            break;

        case 0x02:
        case 0x03:
            // incomplete/complete List of 16-bit Service Class UUIDs
        case 0x04:
        case 0x05:
            // incomplete/complete List of 32-bit Service Class UUIDs
        case 0x06:
        case 0x07:
            if(hcidumpDebugMode) {
                // incomplete/complete List of 128-bit Service Class UUIDs
                p_indent(level, frm);
                printf("%s service classes:", ads.type == 0x06 ? "Incomplete" : "Complete");
                for (i = 0; i < ads.length / 2; i++)
                    printf(" 0x%4.4x", bt_get_le16(data + i * 2));
                printf("\n");
            }
            break;

        case 0x08:
        case 0x09:
            if(hcidumpDebugMode) {
                str = (char *) malloc(ads.length + 1);
                if (str) {
                    snprintf(str, ads.length + 1, "%s", (char *) data);
                    for (i = 0; i < ads.length; i++)
                        if (!isprint(str[i]))
                            str[i] = '.';
                    p_indent(level, frm);
                    printf("%s local name: \'%s\'",
                           ads.type == 0x08 ? "Shortened" : "Complete", str);
                    for (i = 0; i < ads.length; i++)
                        printf(" 0x%2.2x", data[i]);
                    printf("\n");
                    free(str);
                }
            }
            break;

        case 0x0a: {
            uint8_t power = *data;
            if (hcidumpDebugMode) {
                p_indent(level, frm);
                printf("TX power level: %d\n", power);
            }
        }
            break;
        case 0x12:
            //  Slave Connection Interval Range sent by Gimbals
            if(hcidumpDebugMode) {
                p_indent(level, frm);
                printf("Slave Connection Interval Range %d bytes data\n", ads.length);
            }
            break;

        case 0x16:
            if(hcidumpDebugMode) {
                    printf("ServiceData16(%x %x), len=%d", data[0], data[1], ads.length);
                for (i = 2; i < ads.length; i++)
                    printf(" 0x%2.2x", data[i]);
                printf("\n");
            }
            break;
        case 0xff:

            if(hcidumpDebugMode) {
                p_indent(level, frm);
                printf("ManufacturerData(%d bytes), valid=%d:", ads.length, ads.length >= MIN_MANUFACTURER_DATA_SIZE);
                print_bytes(data, ads.length);
                hex_debug(level, frm);
            }
            break;

        default:
            p_indent(level, frm);
            printf("Unknown type 0x%02x with %d bytes data\n", ads.type, ads.length);
            if(hcidumpDebugMode) {
                print_bytes(data, ads.length);
                hex_debug(level, frm);
            }
            break;
    }
}

static inline void evt_le_advertising_report_dump(int level, struct frame *frm, ad_data& packet)
{
    uint8_t num_reports = get_u8(frm);
    const uint8_t RSSI_SIZE = 1;

    while (num_reports--) {
        char addr[18];
        le_advertising_info *info = (le_advertising_info *) frm->ptr;
        int offset = 0;

        p_ba2str(&info->bdaddr, addr);
        packet.bdaddr_type = info->bdaddr_type;
        memcpy(packet.bdaddr, &info->bdaddr, sizeof(packet.bdaddr));

        if(hcidumpDebugMode) {
            p_indent(level, frm);
            printf("%s (%d)\n", evttype2str(info->evt_type), info->evt_type);

            p_indent(level, frm);
            printf("bdaddr %s (%s)\n", addr, bdaddrtype2str(info->bdaddr_type));
        }
#ifdef FULL_DEBUG
        printf("Debug(%d): [", info->length);
        for(int n = 0; n < info->length; n ++) {
            printf("%x", info->data[n]);
        }
        printf("]\n");
#endif
        while (offset < info->length) {
            int eir_data_len = info->data[offset];

            ext_inquiry_data_dump(level, frm, &info->data[offset], packet);

            offset += eir_data_len + 1;
        }

        frm->ptr += LE_ADVERTISING_INFO_SIZE + info->length;
        frm->len -= LE_ADVERTISING_INFO_SIZE + info->length;

        packet.time = frm->ts.tv_sec;
        packet.time *= 1000;
        packet.time += frm->ts.tv_usec/1000;
        packet.rssi = ((int8_t *) frm->ptr)[frm->len - 1];
        if(hcidumpDebugMode) {
            p_indent(level, frm);
            printf("RSSI: %d\n", packet.rssi);
        }
        frm->ptr += RSSI_SIZE;
        frm->len -= RSSI_SIZE;
    }
}

static inline void le_meta_ev_dump(int level, struct frame *frm, ad_data& info)
{
    evt_le_meta_event *mevt = (evt_le_meta_event *) frm->ptr;
    uint8_t subevent;

    subevent = mevt->subevent;

    frm->ptr += EVT_LE_META_EVENT_SIZE;
    frm->len -= EVT_LE_META_EVENT_SIZE;

    if(hcidumpDebugMode) {
        p_indent(level, frm);
        printf("%s\n", ev_le_meta_str[subevent]);
    }
    switch (mevt->subevent) {
        case EVT_LE_CONN_COMPLETE:
            //evt_le_conn_complete_dump(level + 1, frm);
            printf("Skipping EVT_LE_CONN_COMPLETE\n");
            break;
        case EVT_LE_ADVERTISING_REPORT:
            evt_le_advertising_report_dump(level + 1, frm, info);
            break;
        case EVT_LE_CONN_UPDATE_COMPLETE:
            //evt_le_conn_update_complete_dump(level + 1, frm);
            printf("Skipping EVT_LE_CONN_UPDATE_COMPLETE\n");
            break;
        case EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
            //evt_le_read_remote_used_features_complete_dump(level + 1, frm);
            printf("Skipping EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE\n");
            break;
        default:
            hex_debug(level, frm);
            break;
    }
}

static inline void event_dump(int level, struct frame *frm, ad_data& info)
{
    hci_event_hdr *hdr = (hci_event_hdr *)frm->ptr;
    uint8_t event = hdr->evt;

    if (event <= EVENT_NUM) {
        if(hcidumpDebugMode) {
            p_indent(level, frm);
            printf("HCI Event: %s (0x%2.2x) plen %d\n", event_str[hdr->evt], hdr->evt, hdr->plen);
        }
    } else if (hdr->evt == EVT_TESTING) {
        p_indent(level, frm);
        printf("HCI Event: Testing (0x%2.2x) plen %d\n", hdr->evt, hdr->plen);
    } else {
        p_indent(level, frm);
        printf("HCI Event: code 0x%2.2x plen %d\n", hdr->evt, hdr->plen);
    }

    frm->ptr += HCI_EVENT_HDR_SIZE;
    frm->len -= HCI_EVENT_HDR_SIZE;

    if (event == EVT_CMD_COMPLETE) {
        evt_cmd_complete *cc = (evt_cmd_complete *) frm->ptr;
        if (cc->opcode == cmd_opcode_pack(OGF_INFO_PARAM, OCF_READ_LOCAL_VERSION)) {
            read_local_version_rp *rp = (read_local_version_rp *)frm->ptr + EVT_CMD_COMPLETE_SIZE;
        }
    }

    switch (event) {
        case EVT_LOOPBACK_COMMAND:
            printf("Skipping EVT_LOOPBACK_COMMAND\n");
            break;
        case EVT_CMD_COMPLETE:
            printf("Skipping EVT_CMD_COMPLETE\n");
            break;
        case EVT_LE_META_EVENT:
            le_meta_ev_dump(level + 1, frm, info);
            break;

        default:
            printf("Skipping event=%d\n", event);
            raw_dump(level+1, frm);
            break;
    }
}

static inline void extractBeaconInfo(ad_structure& ads, beacon_info *info) {

    uint8_t *data = ads.data;
    // Get the manufacturer code from the first two octets
    int index = 0;
    info->manufacturer = 256 * data[index++] + data[index++];

    // Get the first octet of the beacon code
    info->code = 256 * data[index++] + data[index++];

    // Get the proximity uuid
    int n;
    for(n = 0; n < 2*UUID_SIZE; n += 2, index ++) {
        int b0 = (data[index] & 0xf0) >> 4;
        int b1 = data[index] & 0x0f;
        char c0 = toHexChar(b0);
        char c1 = toHexChar(b1);
        info->uuid[n] = c0;
        info->uuid[n+1] = c1;
    }
    // null terminate the 2*UUID_SIZE bytes that make up the uuid string
    info->uuid[2*UUID_SIZE] = '\0';
    // Get the beacon major id
    int m0 = data[index++];
    int m1 = data[index++];
    info->major = 256 * m0 + m1;
    printf("Major: %d\n", info->major);
    // Get the beacon minor id
    m0 = data[index++];
    m1 = data[index++];
    info->minor = 256 * m0 + m1;
    // Get the transmitted power, which is encoded as the 2's complement of the calibrated Tx Power
    info->calibrated_power = data[index] - 256;
}

static void do_parse(struct frame *frm, ad_data& info) {
    uint8_t type = *(uint8_t *)frm->ptr;

    frm->ptr++; frm->len--;
    switch (type) {
        case HCI_EVENT_PKT:
            event_dump(0, frm, info);
            break;

        default:
            p_indent(0, frm);
            printf("Unknown: type 0x%2.2x len %d\n", type, frm->len);
            hex_debug(0, frm);
            break;
    }
}

/*
    This is the process_frames function from hcidump.c with the addition of the beacon_event callback
 */
int process_frames(int dev, int sock, int fd, unsigned long flags, std::function<bool(ad_data&)> callback)
{
    struct cmsghdr *cmsg;
    struct msghdr msg;
    struct iovec  iv;
    struct hcidump_hdr *dh;
    struct btsnoop_pkt *dp;
    struct frame frm;
    struct pollfd fds[2];
    int nfds = 0;
    uint8_t *buf, *ctrl;
    int len, hdr_size = HCIDUMP_HDR_SIZE;

    if (sock < 0)
        return -1;

    if (snap_len < SNAP_LEN)
        snap_len = SNAP_LEN;

    if (flags & DUMP_BTSNOOP)
        hdr_size = BTSNOOP_PKT_SIZE;

    buf = (uint8_t*) malloc(snap_len + hdr_size);
    if (!buf) {
        perror("Can't allocate data buffer");
        return -1;
    }

    frm.data = buf + hdr_size;

    ctrl = (uint8_t *) malloc(100);
    if (!ctrl) {
        free(buf);
        perror("Can't allocate control buffer");
        return -1;
    }

    if (dev == HCI_DEV_NONE)
        printf("system: ");
    else
        printf("device: hci%d ", dev);

    printf("snap_len: %d filter: 0x%lx\n", snap_len, parser.filter);

    memset(&msg, 0, sizeof(msg));

    fds[nfds].fd = sock;
    fds[nfds].events = POLLIN;
    fds[nfds].revents = 0;
    nfds++;

    long frameNo = 0;
    bool stopped = false;
    while (!stopped) {
        int i, n = poll(fds, nfds, 5000);

        if (n <= 0) {
            if(stop_scan_frames)
                break;
            continue;
        }

        for (i = 0; i < nfds; i++) {
            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                if (fds[i].fd == sock)
                    printf("device: disconnected\n");
                else
                    printf("client: disconnect\n");
                return 0;
            }
        }

        iv.iov_base = frm.data;
        iv.iov_len  = snap_len;

        msg.msg_iov = &iv;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = 100;

        len = recvmsg(sock, &msg, MSG_DONTWAIT);
        if (len < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            perror("Receive failed");
            return -1;
        }

        /* Process control message */
        frm.data_len = len;
        frm.dev_id = dev;
        frm.in = 0;

        cmsg = CMSG_FIRSTHDR(&msg);
        while (cmsg) {
            int dir;
            switch (cmsg->cmsg_type) {
                case HCI_CMSG_DIR:
                    memcpy(&dir, CMSG_DATA(cmsg), sizeof(int));
                    frm.in = (uint8_t) dir;
                    break;
                case HCI_CMSG_TSTAMP:
                    memcpy(&frm.ts, CMSG_DATA(cmsg), sizeof(struct timeval));
                    break;
            }
            cmsg = CMSG_NXTHDR(&msg, cmsg);
        }

        frm.ptr = frm.data;
        frm.len = frm.data_len;

        /* Parse and print */
        frameNo ++;
        if(hcidumpDebugMode) {
            printf("Begin do_parse(ts=%ld.%ld)#%ld\n", frm.ts.tv_sec, frm.ts.tv_usec, frameNo);
        }
        ad_data event;
        do_parse(&frm, event);
        int64_t time = event.time;
        if(time > 0) {
            stopped = callback(event);
            // Free the ad_structure.data
            for(int n = 0; n < event.data.size(); n ++) {
                ad_structure* ads = event.data[n];
                free(ads);
            }
        }
        if(hcidumpDebugMode) {
            printf("End do_parse(info.time=%lld, ad.count=%ld)\n", time, event.data.size());
        }
    }

    free(buf);
    free(ctrl);

    return 0;
}

/**
 * The legacy iBeacon style of scanner
 */
int scan_frames(int32_t device, beacon_event callback) {

    // Lambda wrapper around the legacy callback
    std::function<bool(ad_data&)> wrapper = [&] (ad_data& event) {
        beacon_info *info = nullptr;
        // Check for a manufacturer specific data frame that looks like a beacon event
        for (std::vector<ad_structure*>::iterator it = event.data.begin(); it != event.data.end(); ++it) {
            ad_structure& ads = *(*it);
            if (ads.type == 0xff) {
                if(hcidumpDebugMode)
                    printf("ManufacturerData(%d bytes): ", ads.length);
                if (ads.length >= MIN_MANUFACTURER_DATA_SIZE) {
                    if(hcidumpDebugMode) {
                        for (int i = 0; i < ads.length; ++i) {
                            printf("%x", ads.data[i]);
                        }
                        printf("\n");
                    }
                    // Pull out the beacon info
                    info = new beacon_info;
                    extractBeaconInfo(ads, info);
                    if(hcidumpDebugMode) {
                        printf("Major:%d, Minor:%d\n", info->major, info->minor);
                        printf("UUID(%ld):%s\n", strlen(info->uuid), info->uuid);
                    }
                    // Free the ad_structure.data
                    for(int n = 0; n < event.data.size(); n ++) {
                        ad_structure* ads = event.data[n];
                        free(ads);
                    }
                    return (*callback)(info);
                }
                if(hcidumpDebugMode)
                    printf("\n");
            }
        }
        return false;
    };
    return scan_for_ad_events(device, wrapper);
}

int32_t scan_for_ad_events(int32_t device, std::function<bool(ad_data&)> callback) {
    unsigned long flags = 0;

    flags |= DUMP_TSTAMP;
    flags |= DUMP_EXT;
    flags |= DUMP_VERBOSE;
    int socketfd = open_socket(device);
    printf("Scanning hci%d, socket=%d, hcidumpDebugMode=%d\n", device, socketfd, hcidumpDebugMode);
    return process_frames(device, socketfd, -1, flags, callback);
}

int32_t scan_for_ad_events_inline(int32_t device, std::function<bool(ad_data_inline&)> callback) {
    // Lambda wrapper around the ad_data& based callback that inlines the vector<ad_structure*> data
    std::function<bool(ad_data &)> wrapper = [&](ad_data &event) {
        ad_data_inline* event_inline = toInline(event);
        if(hcidumpDebugMode) {
            printf("HCI:ad_data:{time=%ld, rssi=%d, count=%d}\n", event_inline->time, event_inline->rssi,
                   event_inline->count);
            printf("\tHCI:AD(%d:%d) vs inline(%d:%d)\n", event.data[0]->type, event.data[0]->length,
                   event_inline->data[0].type, event_inline->data[0].length);
        }
        bool stop = callback(*event_inline);
        free(event_inline);
        return stop;
    };
    return scan_for_ad_events(device, wrapper);
}
