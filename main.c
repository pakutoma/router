#include "init_os.h"
#include "log.h"
#include "route.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
    if (init_os() == -1) {
        return -1;
    }
    if (init_settings("settings.xml") == -1) {
        return -1;
    }
    if (init_route() == -1) {
        return -1;
    }

    route();

    return 0;
}
