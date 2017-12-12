#include "device.h"
#include "device_init.h"
#include "log.h"
#include "route.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argv, char *args[]) {
    if (argv < 3) {
        log_error("Error: Devices didn't assign. \n");
        return -1;
    }
    if (argv < 4) {
        log_error("Error: Next router didn't assign. \n");
        return -1;
    }
    if (argv == 6) {
        set_log_mode(strcmp(args[4], "0"), strcmp(args[5], "0"));
    }
    device_t devices[2];
    devices[0].netif_name = args[1];
    devices[1].netif_name = args[2];
    char *next_router_addr = args[3];
    device_init(devices);

    route(devices, next_router_addr);

    close(devices[0].sock_desc);
    close(devices[1].sock_desc);
    return 0;
}
