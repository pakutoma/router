#include "device.h"
#include <net/ethernet.h>
#include <netinet/in.h>

int find_device(uint8_t macaddr[ETH_ALEN], device_t devices[NUMBER_OF_DEVICES]) {
    int i;
    for (i = 0; i < NUMBER_OF_DEVICES; i++) {
        if (memcmp(devices[i].hw_addr, macaddr, sizeof(uint8_t) * ETH_ALEN) == 0) {
            return i;
        }
    }
    return -1;
}