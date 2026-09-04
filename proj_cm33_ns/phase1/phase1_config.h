#ifndef PHASE1_CONFIG_H
#define PHASE1_CONFIG_H

#include <stdint.h>

#define PHASE1_NUM_CHIRPS_PER_FRAME          (8U)
#define PHASE1_NUM_SAMPLES_PER_CHIRP         (64U)
#define PHASE1_ZERO_PADDING_FACTOR           (2U)

#define PHASE1_NUM_SAMPLES_PER_FRAME         (PHASE1_NUM_CHIRPS_PER_FRAME * PHASE1_NUM_SAMPLES_PER_CHIRP)
#define PHASE1_NUM_SAMPLES_ZERO_PADDED       (PHASE1_NUM_SAMPLES_PER_CHIRP * PHASE1_ZERO_PADDING_FACTOR)
#define PHASE1_FFT_BINS                      (PHASE1_NUM_SAMPLES_ZERO_PADDED / 2U)

#define PHASE1_ADC_MAX                       (4095U)
#define PHASE1_ADC_SCALE                     (4096.0f)

#define PHASE1_BANDWIDTH_HZ                  (4600000000ULL)
#define PHASE1_LIGHT_SPEED_MPS               (299792458.0f)

#define PHASE1_MIN_VALID_BIN                 (2U)
#define PHASE1_MAX_VALID_BIN                 (PHASE1_FFT_BINS - 2U)

#endif