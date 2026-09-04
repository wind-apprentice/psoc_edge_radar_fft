/* ***************************************************************************
** File name: BGT60TR13C_RegisterList.h
**
** Description: This is the BGT60TR13C register list file for PSOC6 Radar Static Distance Code Example.
** Generated using XENSIV BGT60TRXX register configurator, SDK version3.6.4+4b4a62456
**
*******************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef XENSIV_BGT60TRXX_CONF_H
#define XENSIV_BGT60TRXX_CONF_H

#define XENSIV_BGT60TRXX_CONF_DEVICE (XENSIV_DEVICE_BGT60TR13C)
#define XENSIV_BGT60TRXX_CONF_START_FREQ_HZ (58099999000)
#define XENSIV_BGT60TRXX_CONF_END_FREQ_HZ (63499997000)
#define XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP (512)
#define XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME (8)
#define XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS (1)
#define XENSIV_BGT60TRXX_CONF_NUM_TX_ANTENNAS (1)
#define XENSIV_BGT60TRXX_CONF_SAMPLE_RATE (1000000)
#define XENSIV_BGT60TRXX_CONF_CHIRP_REPETITION_TIME_S (0.00419446)
#define XENSIV_BGT60TRXX_CONF_FRAME_REPETITION_TIME_S (0.0800838)
#define XENSIV_BGT60TRXX_CONF_NUM_REGS (38)

#if defined(XENSIV_BGT60TRXX_CONF_IMPL)
const uint32_t register_list[] = {
    0x11e8270UL,
    0x3140210UL,
    0x9e967fdUL,
    0xb0805b4UL,
    0xd1027ffUL,
    0xf010f00UL,
    0x11000000UL,
    0x13000000UL,
    0x15000000UL,
    0x17000be0UL,
    0x19000000UL,
    0x1b000000UL,
    0x1d000000UL,
    0x1f000b60UL,
    0x21103c51UL,
    0x231ff41fUL,
    0x25005ef7UL,
    0x2d000490UL,
    0x3b000480UL,
    0x49000480UL,
    0x57000480UL,
    0x5911be0eUL,
    0x5b5f840aUL,
    0x5d007000UL,
    0x5f787e1eUL,
    0x61ac0204UL,
    0x630000d7UL,
    0x65001432UL,
    0x67000200UL,
    0x69000000UL,
    0x6b000000UL,
    0x6d000000UL,
    0x6f3e6d10UL,
    0x7f000100UL,
    0x8f000100UL,
    0x9f000100UL,
    0xad000000UL,
    0xb7000000UL,
};
#endif /* XENSIV_BGT60TRXX_CONF_IMPL */

#endif /* XENSIV_BGT60TRXX_CONF_H */
