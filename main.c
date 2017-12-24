#include "device.h"
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

    char *device_names[NUMBER_OF_DEVICES];
    device_names[0] = argv[1];
    device_names[1] = argv[2];

    char *next_router_addr = argv[3];
    device_init(device_names);

    route(next_router_addr);

    return 0;
}
