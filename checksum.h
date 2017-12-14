#pragma once
#include <stdint.h>
uint16_t calc_checksum(uint8_t *header, int32_t length);
uint16_t update_checksum(uint16_t checksum, uint16_t original_value, uint16_t new_value);