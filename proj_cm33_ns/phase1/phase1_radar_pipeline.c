#include "phase1_radar_pipeline.h"

#include <math.h>
#include <stddef.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define PHASE1_RSLT_BAD_ARG ((cy_rslt_t)0x1001U)
#define PHASE1_RSLT_NOT_READY ((cy_rslt_t)0x1002U)

static bool is_power_of_two(uint32_t n)
{
    return (n > 0U) && ((n & (n - 1U)) == 0U);
}

static void build_hamming_window(float *window, uint32_t n)
{
    if (n <= 1U)
    {
        window[0] = 1.0f;
        return;
    }

    for (uint32_t i = 0; i < n; ++i)
    {
        float ratio = (float)i / (float)(n - 1U);
        window[i] = 0.54f - 0.46f * cosf(2.0f * (float)M_PI * ratio);
    }
}

static uint32_t reverse_bits(uint32_t x, uint32_t bits)
{
    uint32_t y = 0U;
    for (uint32_t i = 0U; i < bits; ++i)
    {
        y = (y << 1U) | (x & 1U);
        x >>= 1U;
    }
    return y;
}

static void fft_in_place(float *real, float *imag, uint32_t n)
{
    uint32_t bits = 0U;
    while ((1U << bits) < n)
    {
        bits++;
    }

    /* Bit-reversal reorder for iterative radix-2 butterflies. */
    for (uint32_t i = 0U; i < n; ++i)
    {
        uint32_t j = reverse_bits(i, bits);
        if (j > i)
        {
            float tr = real[i];
            float ti = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = tr;
            imag[j] = ti;
        }
    }

    /* Cooley-Tukey radix-2 FFT stages. */
    for (uint32_t len = 2U; len <= n; len <<= 1U)
    {
        uint32_t half = len >> 1U;
        float angle = -2.0f * (float)M_PI / (float)len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);

        for (uint32_t i = 0U; i < n; i += len)
        {
            float wr = 1.0f;
            float wi = 0.0f;

            for (uint32_t j = 0U; j < half; ++j)
            {
                uint32_t u = i + j;
                uint32_t v = u + half;

                float vr = real[v] * wr - imag[v] * wi;
                float vi = real[v] * wi + imag[v] * wr;

                float ur = real[u];
                float ui = imag[u];

                real[u] = ur + vr;
                imag[u] = ui + vi;
                real[v] = ur - vr;
                imag[v] = ui - vi;

                float next_wr = wr * wlen_r - wi * wlen_i;
                wi = wr * wlen_i + wi * wlen_r;
                wr = next_wr;
            }
        }
    }
}

static float compute_bin_length_m(void)
{
    float range_resolution = PHASE1_LIGHT_SPEED_MPS / (2.0f * (float)PHASE1_BANDWIDTH_HZ);
    return range_resolution / (float)PHASE1_ZERO_PADDING_FACTOR;
}

cy_rslt_t phase1_radar_pipeline_init(phase1_radar_pipeline_t *ctx)
{
    if ((ctx == NULL) || !is_power_of_two(PHASE1_NUM_SAMPLES_ZERO_PADDED))
    {
        return PHASE1_RSLT_BAD_ARG;
    }

    build_hamming_window(ctx->window, PHASE1_NUM_SAMPLES_ZERO_PADDED);

    for (uint32_t i = 0U; i < PHASE1_FFT_BINS; ++i)
    {
        ctx->spectrum[i] = 0.0f;
    }

    ctx->initialized = true;
    return CY_RSLT_SUCCESS;
}

cy_rslt_t phase1_radar_pipeline_run(phase1_radar_pipeline_t *ctx,
                                    const uint16_t *frame_data,
                                    uint32_t frame_len,
                                    phase1_radar_result_t *result)
{
    if ((ctx == NULL) || (frame_data == NULL) || (result == NULL))
    {
        return PHASE1_RSLT_BAD_ARG;
    }

    if ((!ctx->initialized) || (frame_len != PHASE1_NUM_SAMPLES_PER_FRAME))
    {
        return PHASE1_RSLT_NOT_READY;
    }

    for (uint32_t bin = 0U; bin < PHASE1_FFT_BINS; ++bin)
    {
        ctx->spectrum[bin] = 0.0f;
    }

    for (uint32_t chirp = 0U; chirp < PHASE1_NUM_CHIRPS_PER_FRAME; ++chirp)
    {
        float avg = 0.0f;
        uint32_t base = chirp * PHASE1_NUM_SAMPLES_PER_CHIRP;

        /* Per-chirp DC removal to suppress static bias near bin 0. */
        for (uint32_t n = 0U; n < PHASE1_NUM_SAMPLES_PER_CHIRP; ++n)
        {
            avg += (float)frame_data[base + n] / PHASE1_ADC_SCALE;
        }
        avg /= (float)PHASE1_NUM_SAMPLES_PER_CHIRP;

        /* Copy, zero-pad, and apply Hamming window before FFT. */
        for (uint32_t n = 0U; n < PHASE1_NUM_SAMPLES_ZERO_PADDED; ++n)
        {
            float sample = 0.0f;
            if (n < PHASE1_NUM_SAMPLES_PER_CHIRP)
            {
                sample = ((float)frame_data[base + n] / PHASE1_ADC_SCALE) - avg;
            }

            ctx->work_real[n] = sample * ctx->window[n];
            ctx->work_imag[n] = 0.0f;
        }

        fft_in_place(ctx->work_real, ctx->work_imag, PHASE1_NUM_SAMPLES_ZERO_PADDED);

        /* Magnitude spectrum accumulation across chirps. */
        for (uint32_t bin = 0U; bin < PHASE1_FFT_BINS; ++bin)
        {
            float re = ctx->work_real[bin];
            float im = ctx->work_imag[bin];
            float mag = sqrtf((re * re) + (im * im));
            ctx->spectrum[bin] += mag;
        }
    }

    /* Coherent integration proxy: mean magnitude over all chirps. */
    for (uint32_t bin = 0U; bin < PHASE1_FFT_BINS; ++bin)
    {
        ctx->spectrum[bin] /= (float)PHASE1_NUM_CHIRPS_PER_FRAME;
    }

    uint16_t peak_idx = PHASE1_MIN_VALID_BIN;
    float peak_val = -1.0f;

    /* Peak search over valid bins only. */
    for (uint16_t bin = PHASE1_MIN_VALID_BIN; bin <= (uint16_t)PHASE1_MAX_VALID_BIN; ++bin)
    {
        if (ctx->spectrum[bin] > peak_val)
        {
            peak_val = ctx->spectrum[bin];
            peak_idx = bin;
        }
    }

    uint16_t second_idx = PHASE1_MIN_VALID_BIN;
    float second_val = -1.0f;

    /* Secondary peak ignores +-1 bins around main peak to reduce duplicate lobes. */
    for (uint16_t bin = PHASE1_MIN_VALID_BIN; bin <= (uint16_t)PHASE1_MAX_VALID_BIN; ++bin)
    {
        if ((bin + 1U >= peak_idx) && (bin <= peak_idx + 1U))
        {
            continue;
        }

        if (ctx->spectrum[bin] > second_val)
        {
            second_val = ctx->spectrum[bin];
            second_idx = bin;
        }
    }

    result->spectrum = ctx->spectrum;
    result->spectrum_len = (uint16_t)PHASE1_FFT_BINS;
    result->peak_idx = peak_idx;
    result->second_peak_idx = second_idx;
    result->peak_magnitude = peak_val;
    result->second_peak_magnitude = second_val;
    result->bin_len_m = compute_bin_length_m();

    return CY_RSLT_SUCCESS;
}