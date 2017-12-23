#include "log.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static uint64_t timers[100] = {0};
static uint64_t timers_min[100] = {0};
static uint64_t timers_max[100] = {0};
static uint64_t call_count[100] = {0};
static struct timespec start_times[100];

void timer_init(int index) {
    timers[index] = 0;
    timers_min[index] = UINT64_MAX;
    timers_max[index] = 0;
    call_count[index] = 0;
}

void timer_start(int index) {
    call_count[index]++;
    clock_gettime(CLOCK_MONOTONIC, &start_times[index]);
}

void timer_stop(int index) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t t = (now.tv_sec - start_times[index].tv_sec) * 1000000000 + (now.tv_nsec - start_times[index].tv_nsec);
    timers[index] += t;
    if (t < timers_min[index]) {
        timers_min[index] = t;
    }
    if (t > timers_max[index]) {
        timers_max[index] = t;
    }
}

void timer_show(int index) {
    printf("%d: %lutimes,%lu us,%lu us/times,min: %lu us,max: %lu us\n", index, call_count[index], timers[index] / 1000, (timers[index] / 1000) / call_count[index], timers_min[index] / 1000, timers_max[index] / 1000);
}