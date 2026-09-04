/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU
*
* Related Document : See README.md
*
********************************************************************************
 * (c) 2023-2026, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/

#include "cybsp.h"
#include "cy_pdl.h"
#include "retarget_io_init.h"
#include "radar_processing.h"

#if !defined(PHASE1_APP_MODE)
#define PHASE1_APP_MODE (3U)
#endif

/*
 * Mode switch:
 * 1 = synthetic frame -> FFT/spectrum (no radar hardware dependency)
 * 2 = onboard radar smoke test (SPI/IRQ/FIFO sanity only)
 * 3 = onboard radar real signal -> FFT/spectrum
 */
#define PHASE1_APP_MODE_MOCK          (1U)
#define PHASE1_APP_MODE_SENSOR_SMOKE  (2U)
#define PHASE1_APP_MODE_SENSOR_FFT    (3U)

#if (PHASE1_APP_MODE >= PHASE1_APP_MODE_SENSOR_SMOKE)
#include "xensiv_bgt60trxx_mtb.h"
#define XENSIV_BGT60TRXX_CONF_IMPL
#include "BGT60TR13C_RegisterList.h"
#if (XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS != 1)
#error "This integration assumes one RX antenna frame layout."
#endif
#endif

/*******************************************************************************
* Macros
*******************************************************************************/
#define BLINKY_LED_DELAY_MSEC       (1000U)

/* The timeout value in microseconds used to wait for CM55 core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC    (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

#if (PHASE1_APP_MODE >= PHASE1_APP_MODE_SENSOR_SMOKE)
#define XENSIV_BGT60TRXX_IRQ_PRIORITY      (3U)
#define SPI_INTR_NUM                       ((IRQn_Type) CYBSP_SPI_CONTROLLER_IRQ)
#define SPI_INTR_PRIORITY                  (2U)
#define NUM_SAMPLES_PER_FRAME              (XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS * \
                                           XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME * \
                                           XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP)

/* Ignore very near bins to reduce TX-RX coupling dominance during bring-up. */
#define SENSOR_FFT_SEARCH_MIN_BIN          (6U)

static xensiv_bgt60trxx_mtb_t sensor;
static cy_en_scb_spi_status_t init_status;
static cy_stc_scb_spi_context_t spi_context;
static cy_stc_sysint_t irq_cfg;
static volatile bool data_available = false;

/* Keep algorithm knobs aligned with the original static-distance sample. */
int16_t zeroPadding_factor = 1;
float32_t max_range_m = 3.0f;
float32_t min_range_m = 0.15f;

static void mSPI_Interrupt(void)
{
    Cy_SCB_SPI_Interrupt(CYBSP_SPI_CONTROLLER_HW, &spi_context);
}

static void xensiv_bgt60trxx_interrupt_handler(void)
{
    Cy_GPIO_ClearInterrupt(CYBSP_RADAR_INT_PORT, CYBSP_RADAR_INT_PIN);
    NVIC_ClearPendingIRQ(irq_cfg.intrSrc);
    data_available = true;
}

static cy_rslt_t init_onboard_radar_sensor(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Bind sensor driver interface to BSP-generated onboard radar resources. */
    sensor.iface.scb_inst = CYBSP_SPI_CONTROLLER_HW;
    sensor.iface.spi = &spi_context;
    sensor.iface.sel_port = CYBSP_RSPI_CS_PORT;
    sensor.iface.sel_pin = CYBSP_RSPI_CS_PIN;
    sensor.iface.rst_port = CYBSP_RADAR_RESET_PORT;
    sensor.iface.rst_pin = CYBSP_RADAR_RESET_PIN;
    sensor.iface.irq_port = CYBSP_RADAR_INT_PORT;
    sensor.iface.irq_pin = CYBSP_RADAR_INT_PIN;
    sensor.iface.irq_num = CYBSP_RADAR_INT_IRQ;

    irq_cfg.intrSrc = CYBSP_RADAR_INT_IRQ;
    irq_cfg.intrPriority = XENSIV_BGT60TRXX_IRQ_PRIORITY;

    /* Initialize and enable SCB SPI used by onboard BGT60TR13C. */
    init_status = Cy_SCB_SPI_Init(CYBSP_SPI_CONTROLLER_HW, &CYBSP_SPI_CONTROLLER_config, &spi_context);
    if (CY_SCB_SPI_SUCCESS != init_status)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    {
        cy_stc_sysint_t spi_intr_cfg =
        {
            .intrSrc = SPI_INTR_NUM,
            .intrPriority = SPI_INTR_PRIORITY,
        };

        Cy_SysInt_Init(&spi_intr_cfg, &mSPI_Interrupt);
        NVIC_EnableIRQ(SPI_INTR_NUM);
    }

    Cy_SCB_SPI_SetActiveSlaveSelect(CYBSP_SPI_CONTROLLER_HW, CY_SCB_SPI_SLAVE_SELECT1);
    Cy_SCB_SPI_Enable(CYBSP_SPI_CONTROLLER_HW);

    Cy_GPIO_SetSlewRate(CYBSP_RSPI_MOSI_PORT, CYBSP_RSPI_MOSI_PIN, CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYBSP_RSPI_MOSI_PORT, CYBSP_RSPI_MOSI_PIN, CY_GPIO_DRIVE_1_8);
    Cy_GPIO_SetSlewRate(CYBSP_RSPI_CLK_PORT, CYBSP_RSPI_CLK_PIN, CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYBSP_RSPI_CLK_PORT, CYBSP_RSPI_CLK_PIN, CY_GPIO_DRIVE_1_8);

    result = xensiv_bgt60trxx_mtb_init(&sensor, register_list, XENSIV_BGT60TRXX_CONF_NUM_REGS);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    result = xensiv_bgt60trxx_mtb_interrupt_init(&sensor, NUM_SAMPLES_PER_FRAME);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    Cy_SysInt_Init(&irq_cfg, xensiv_bgt60trxx_interrupt_handler);
    NVIC_ClearPendingIRQ(irq_cfg.intrSrc);
    NVIC_EnableIRQ(irq_cfg.intrSrc);
    Cy_GPIO_ClearInterrupt(CYBSP_RADAR_INT_PORT, CYBSP_RADAR_INT_PIN);
    NVIC_ClearPendingIRQ(irq_cfg.intrSrc);

    Cy_SysLib_Delay(1000U);
    return CY_RSLT_SUCCESS;
}

static cy_rslt_t run_sensor_smoke_test(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t frame_counter = 0U;
    static uint16_t samples[NUM_SAMPLES_PER_FRAME] = {0};

    result = init_onboard_radar_sensor();
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    /* Test pattern mode validates digital transport path independent of scene. */
    if (xensiv_bgt60trxx_enable_data_test_mode(&sensor.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    if (xensiv_bgt60trxx_start_frame(&sensor.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    printf("Sensor smoke test started. Waiting for FIFO IRQ...\r\n");

    for (;;)
    {
        while (data_available == false)
        {
        }
        data_available = false;

        if (xensiv_bgt60trxx_get_fifo_data(&sensor.dev, samples, NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
        {
            uint16_t min_sample = 0x0FFFU;
            uint16_t max_sample = 0U;
            uint32_t sum = 0U;

            for (uint32_t i = 0U; i < NUM_SAMPLES_PER_FRAME; ++i)
            {
                uint16_t sample = samples[i];
                if (sample < min_sample)
                {
                    min_sample = sample;
                }
                if (sample > max_sample)
                {
                    max_sample = sample;
                }
                sum += sample;
            }

            printf("Frame=%lu | Min=%u | Max=%u | Avg=%lu | S0=%u | S1=%u | S2=%u | S3=%u\r\n",
                   (unsigned long)frame_counter,
                   (unsigned int)min_sample,
                   (unsigned int)max_sample,
                   (unsigned long)(sum / NUM_SAMPLES_PER_FRAME),
                   (unsigned int)samples[0],
                   (unsigned int)samples[1],
                   (unsigned int)samples[2],
                   (unsigned int)samples[3]);
        }
        else
        {
            printf("FIFO read failed on frame %lu\r\n", (unsigned long)frame_counter);
        }

        ++frame_counter;
    }
}

static cy_rslt_t run_sensor_fft_test(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t frame_counter = 0U;
    static uint16_t sensor_samples[NUM_SAMPLES_PER_FRAME] = {0};

    result = init_onboard_radar_sensor();
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    result = init_static_distance(&context);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    /* Disable test pattern to process real IF data from the radar front-end. */
    if (xensiv_bgt60trxx_enable_data_test_mode(&sensor.dev, false) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return CY_RSLT_TYPE_ERROR;
    }

    printf("Sensor FFT mode started. Source=real radar IF signal\r\n");
    printf("Spectrum source=integrated_chirp | FFT bins=%u | Valid search=[%u..%u]\r\n",
        (unsigned int)context.fft_size,
        (unsigned int)context.skip,
        (unsigned int)context.max_range_bin);

    for (;;)
    {
        uint32_t wait_ms = 0U;

        /* Acquire exactly one fresh frame each loop. */
        data_available = false;
        Cy_GPIO_ClearInterrupt(CYBSP_RADAR_INT_PORT, CYBSP_RADAR_INT_PIN);
        NVIC_ClearPendingIRQ(irq_cfg.intrSrc);

        if (xensiv_bgt60trxx_start_frame(&sensor.dev, true) != XENSIV_BGT60TRXX_STATUS_OK)
        {
            printf("Frame start failed on frame %lu\r\n", (unsigned long)frame_counter);
            return CY_RSLT_TYPE_ERROR;
        }

        while (data_available == false)
        {
            Cy_SysLib_Delay(1U);
            wait_ms++;
            if (wait_ms > 250U)
            {
                printf("IRQ timeout on frame %lu\r\n", (unsigned long)frame_counter);
                (void)xensiv_bgt60trxx_start_frame(&sensor.dev, false);
                return CY_RSLT_TYPE_ERROR;
            }
        }
        data_available = false;

        if (xensiv_bgt60trxx_get_fifo_data(&sensor.dev, sensor_samples, NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
        {
            /* Keep original sample flow: process frame but ignore distance return if not needed. */
            float32_t ignored_distance_m = get_static_distance(&context, sensor_samples);

            if (ignored_distance_m < 0.0f)
            {
                printf("Pipeline run failed at frame=%lu\r\n", (unsigned long)frame_counter);
            }
            else
            {
                uint16_t search_min = SENSOR_FFT_SEARCH_MIN_BIN;
                uint16_t search_max = context.max_range_bin;
                uint16_t raw_peak_idx = context.skip;
                uint16_t search_peak_idx = context.skip;
                float raw_peak_val = -1.0f;
                float search_peak_val = -1.0f;
                float noise_sum = 0.0f;
                uint32_t noise_count = 0U;

                if (search_min < context.skip)
                {
                    search_min = context.skip;
                }
                if (search_max >= (uint16_t)context.fft_size)
                {
                    search_max = (uint16_t)(context.fft_size - 1);
                }
                if (search_min > search_max)
                {
                    search_min = context.skip;
                }

                for (uint16_t b = context.skip; b <= search_max; ++b)
                {
                    float v = context.integrated_chirp[b];
                    if (v > raw_peak_val)
                    {
                        raw_peak_val = v;
                        raw_peak_idx = b;
                    }
                }

                for (uint16_t b = search_min; b <= search_max; ++b)
                {
                    float v = context.integrated_chirp[b];
                    if (v > search_peak_val)
                    {
                        search_peak_val = v;
                        search_peak_idx = b;
                    }
                    noise_sum += v;
                    noise_count++;
                }

                 /* Diagnostic summary from one full spectrum snapshot. */
                 float noise_floor = (noise_count > 0U) ? (noise_sum / (float)noise_count) : 0.0f;
                float snr_like = (noise_floor > 0.0001f) ? (search_peak_val / noise_floor) : 0.0f;
                 float peak_range_cm = search_peak_idx * context.bin_len * 100.0f;

                 printf("Frame=%lu | WaitMs=%lu | RawPeakBin=%u | SearchPeakBin=%u | Peak=%.3f | Range=%.2f cm | Noise=%.3f | SNRx=%.2f | B2=%.2f B6=%.2f B10=%.2f B14=%.2f\r\n",
                       (unsigned long)frame_counter,
                       (unsigned long)wait_ms,
                       (unsigned int)raw_peak_idx,
                       (unsigned int)search_peak_idx,
                       (double)search_peak_val,
                       (double)peak_range_cm,
                       (double)noise_floor,
                       (double)snr_like,
                       (double)context.integrated_chirp[2],
                       (double)context.integrated_chirp[6],
                       (double)context.integrated_chirp[10],
                       (double)context.integrated_chirp[14]);
            }
        }
        else
        {
            printf("FIFO read failed on frame %lu\r\n", (unsigned long)frame_counter);
        }

        if (xensiv_bgt60trxx_start_frame(&sensor.dev, false) != XENSIV_BGT60TRXX_STATUS_OK)
        {
            printf("Frame stop failed on frame %lu\r\n", (unsigned long)frame_counter);
            return CY_RSLT_TYPE_ERROR;
        }

        ++frame_counter;
    }
}
#endif

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function of the CM33 non-secure application. 
* 
* It initializes the device and board peripherals. It also initializes the 
* retarget-io middleware to be used with the debug UART port using which 
* "Hello World!" is printed on the debug UART. The LED pin is initialized with
* default configurations. The CM55 core is enabled and then the programs enters
* an infinite while loop which toggles the LED1 with a frequency of 1 Hz.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals. */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution. */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io middleware */
    init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen. */
    printf("\x1b[2J\x1b[;H");

    printf("****************** Phase 1 / Phase 2 Bring-up Harness ******************\r\n\n");

#if (PHASE1_APP_MODE == 0U)
    /* Enable CM55. */
    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
#endif

#if (PHASE1_APP_MODE == PHASE1_APP_MODE_SENSOR_SMOKE)
    printf("Mode=sensor_smoke | Source=onboard BGT60TR13C\r\n\r\n");
    result = run_sensor_smoke_test();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Sensor smoke test init failed: 0x%08lx\r\n", (unsigned long)result);
        handle_app_error();
    }
#elif (PHASE1_APP_MODE == PHASE1_APP_MODE_SENSOR_FFT)
    printf("Mode=sensor_fft | Source=onboard BGT60TR13C\r\n\r\n");
    result = run_sensor_fft_test();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Sensor FFT mode init failed: 0x%08lx\r\n", (unsigned long)result);
        handle_app_error();
    }
#else
    printf("Mode not supported in this branch. Set PHASE1_APP_MODE to 2U or 3U.\r\n");
    handle_app_error();
#endif

    for(;;)
    {
        Cy_GPIO_Inv(CYBSP_USER_LED1_PORT, CYBSP_USER_LED1_PIN);
        Cy_SysLib_Delay(BLINKY_LED_DELAY_MSEC);
    }
}

/* [] END OF FILE */