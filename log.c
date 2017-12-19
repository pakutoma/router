#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int is_enabled_log = 1;
static int is_enabled_log_error = 1;

void set_log_mode(int set_enabled_log, int set_enabled_log_error) {
    is_enabled_log = set_enabled_log;
    is_enabled_log_error = set_enabled_log_error;
}

void log_stdout(char *format, ...) {
    if (!is_enabled_log) {
        return;
    }
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    return;
}

void log_error(char *format, ...) {
    if (!is_enabled_log_error) {
        return;
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    return;
}

void log_perror(char *message) {
    if (!is_enabled_log_error) {
        return;
    }
    fprintf(stderr, "%s : %s\n", message, strerror(errno));
    return;
}