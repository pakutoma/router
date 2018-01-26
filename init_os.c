#include "log.h"
#include <stdio.h>

int init_os() {
    FILE *fp;

    if ((fp = fopen("/proc/sys/net/ipv4/ip_forward", "w")) == NULL) {
        log_error("couldn't write /proc/sys/net/ipv4/ip_forward.\n");
        return -1;
    }
    fputs("0", fp);
    fclose(fp);

    if ((fp = fopen("/proc/sys/net/ipv6/conf/all/disable_ipv6", "w")) == NULL) {
        log_error("couldn't write /proc/sys/net/ipv6/conf/all/disable_ipv6.\n");
        return -1;
    }
    fputs("1", fp);
    fclose(fp);

    return 0;
}
