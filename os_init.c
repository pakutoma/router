#include "log.h"
#include <stdio.h>

int os_init() {
    FILE *fp;

    if ((fp = fopen("/proc/sys/net/ipv4/ip_forward", "w")) == NULL) {
        log_error("Couldn't write /proc/sys/net/ipv4/ip_forward.\n");
        return -1;
    }
    fputs("0", fp);
    fclose(fp);

    return 0;
}
