#ifndef PHASE1_MOCK_DATA_H
#define PHASE1_MOCK_DATA_H

#include <stdint.h>
#include "phase1_config.h"

typedef enum
{
    PHASE1_CASE_SINGLE_PEAK = 0,
    PHASE1_CASE_DOUBLE_PEAK = 1,
    PHASE1_CASE_DC_NOISE = 2,
    PHASE1_CASE_COUNT
} phase1_mock_case_t;

void phase1_mock_generate(phase1_mock_case_t case_id,
                          uint16_t *frame_data,
                          uint32_t frame_len);

const char *phase1_mock_case_name(phase1_mock_case_t case_id);
uint16_t phase1_mock_expected_peak_bin(phase1_mock_case_t case_id);

#endif