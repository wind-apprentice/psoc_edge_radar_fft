#include "phase1_mock_data.h"

#include <math.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

static uint32_t mock_rng_state = 0x1234ABCDU;

static float pseudo_noise_unit(void)
{
    mock_rng_state = (1664525U * mock_rng_state) + 1013904223U;
    uint32_t raw = (mock_rng_state >> 8U) & 0x00FFFFFFU;
    return ((float)raw / 16777215.0f) - 0.5f;
}

static uint16_t clamp_u12(float x)
{
    if (x < 0.0f)
    {
        return 0U;
    }
    if (x > (float)PHASE1_ADC_MAX)
    {
        return PHASE1_ADC_MAX;
    }
    return (uint16_t)x;
}

static void fill_case(uint16_t *frame_data,
                      uint32_t frame_len,
                      float dc,
                      uint16_t bin1,
                      float amp1,
                      uint16_t bin2,
                      float amp2,
                      float noise_amp)
{
    if (frame_len != PHASE1_NUM_SAMPLES_PER_FRAME)
    {
        return;
    }

    for (uint32_t chirp = 0U; chirp < PHASE1_NUM_CHIRPS_PER_FRAME; ++chirp)
    {
        uint32_t base = chirp * PHASE1_NUM_SAMPLES_PER_CHIRP;
        float phase_shift = 0.15f * (float)chirp;

        for (uint32_t n = 0U; n < PHASE1_NUM_SAMPLES_PER_CHIRP; ++n)
        {
            float angle1 = (2.0f * (float)M_PI * (float)bin1 * (float)n) /
                           (float)PHASE1_NUM_SAMPLES_ZERO_PADDED;
            float angle2 = (2.0f * (float)M_PI * (float)bin2 * (float)n) /
                           (float)PHASE1_NUM_SAMPLES_ZERO_PADDED;

            float sample = dc;
            sample += amp1 * sinf(angle1 + phase_shift);
            sample += amp2 * sinf(angle2 + 0.5f * phase_shift);
            sample += noise_amp * pseudo_noise_unit();

            frame_data[base + n] = clamp_u12(sample);
        }
    }
}

void phase1_mock_generate(phase1_mock_case_t case_id,
                          uint16_t *frame_data,
                          uint32_t frame_len)
{
    if (frame_data == NULL)
    {
        return;
    }

    switch (case_id)
    {
        case PHASE1_CASE_SINGLE_PEAK:
            fill_case(frame_data, frame_len, 2048.0f, 12U, 700.0f, 28U, 0.0f, 0.0f);
            break;

        case PHASE1_CASE_DOUBLE_PEAK:
            fill_case(frame_data, frame_len, 2048.0f, 10U, 650.0f, 24U, 350.0f, 15.0f);
            break;

        case PHASE1_CASE_DC_NOISE:
            fill_case(frame_data, frame_len, 2550.0f, 14U, 450.0f, 30U, 180.0f, 95.0f);
            break;

        default:
            fill_case(frame_data, frame_len, 2048.0f, 8U, 500.0f, 20U, 0.0f, 0.0f);
            break;
    }
}

const char *phase1_mock_case_name(phase1_mock_case_t case_id)
{
    switch (case_id)
    {
        case PHASE1_CASE_SINGLE_PEAK:
            return "single_peak";
        case PHASE1_CASE_DOUBLE_PEAK:
            return "double_peak";
        case PHASE1_CASE_DC_NOISE:
            return "dc_noise";
        default:
            return "unknown";
    }
}

uint16_t phase1_mock_expected_peak_bin(phase1_mock_case_t case_id)
{
    switch (case_id)
    {
        case PHASE1_CASE_SINGLE_PEAK:
            return 12U;
        case PHASE1_CASE_DOUBLE_PEAK:
            return 10U;
        case PHASE1_CASE_DC_NOISE:
            return 14U;
        default:
            return 0U;
    }
}