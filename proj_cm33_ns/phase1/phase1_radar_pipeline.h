#ifndef PHASE1_RADAR_PIPELINE_H
#define PHASE1_RADAR_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>
#include "cy_result.h"
#include "phase1_config.h"

typedef struct
{
    float spectrum[PHASE1_FFT_BINS];
    float window[PHASE1_NUM_SAMPLES_ZERO_PADDED];
    float work_real[PHASE1_NUM_SAMPLES_ZERO_PADDED];
    float work_imag[PHASE1_NUM_SAMPLES_ZERO_PADDED];
    bool initialized;
} phase1_radar_pipeline_t;

typedef struct
{
    const float *spectrum;
    uint16_t spectrum_len;
    uint16_t peak_idx;
    uint16_t second_peak_idx;
    float peak_magnitude;
    float second_peak_magnitude;
    float bin_len_m;
} phase1_radar_result_t;

cy_rslt_t phase1_radar_pipeline_init(phase1_radar_pipeline_t *ctx);
cy_rslt_t phase1_radar_pipeline_run(phase1_radar_pipeline_t *ctx,
                                    const uint16_t *frame_data,
                                    uint32_t frame_len,
                                    phase1_radar_result_t *result);

#endif