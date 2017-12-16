#pragma once
#include <stdint.h>
uint16_t calc_checksum(uint8_t *header, int32_t length);
uint32_t calc_fcs(uint8_t *data, int32_t length);