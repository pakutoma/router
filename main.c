#include "device.h"
#include "device_init.h"
#include "log.h"
#include "os_init.h"
#include "route.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    if (argc < 3) {
        log_error("Error: Devices didn't assign. \n");
        return -1;
    }
    if (argc < 4) {
        log_error("Error: Next router didn't assign. \n");
        return -1;
    }
    if (argc == 6) {
        set_log_mode(strcmp(argv[4], "0"), strcmp(argv[5], "0"));
    }

    if (os_init() == -1) {
        return -1;
    }

    device_t devices[2];
    devices[0].netif_name = argv[1];
    devices[1].netif_name = argv[2];
    char *next_router_addr = argv[3];
    device_init(devices);

    route(devices, next_router_addr);

    close(devices[0].sock_desc);
    close(devices[1].sock_desc);
    return 0;
}
