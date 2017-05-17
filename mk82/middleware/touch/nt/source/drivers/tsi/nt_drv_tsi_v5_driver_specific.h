/*
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __FSL_TSI_V5_DRIVER_SPECIFIC_H__
#define __FSL_TSI_V5_DRIVER_SPECIFIC_H__

/**
 * \ingroup nt_driver  
 * \{
 */

/**
* Encapsukates SelfCap and Mutual configuration structure for TSI driver.
*
* Use an instance of this structure with NT_TSI_DRV_Init(). This allows you to configure the
* most common settings of the TSI peripheral with a single function call. Settings include:
*
*/
typedef struct {
    tsi_selfCap_config_t            configSelfCap; /**< Hardware configuration for self capacitance measurement */
    tsi_mutualCap_config_t          configMutual;  /**< Hardware configuration for mutual capacitance measurement */
    uint16_t                        thresl;        /**< Low threshold for out-of-range interrupt (wake-up from low-power) */
    uint16_t                        thresh;        /**< High threshold for out-of-range interrupt (wake-up from low-power) */
}tsi_config_t;
    
/*! @brief TSI status flags. */
typedef enum _tsi_sinc_status_flags
{
    kTSI_SwitchEnable  = TSI_SINC_SWITCH_ENABLE_MASK,    /*!< End-Of-Scan flag */
    kTSI_SincOverflowFlag = TSI_SINC_SINC_OVERFLOW_FLAG_MASK,/*!< Out-Of-Range flag */
    kTSI_SincValid  = TSI_SINC_SINC_VALID_MASK,          /*!< End-Of-Scan flag */
    kTSI_SscControlOut  = TSI_SINC_SSC_CONTROL_OUT_MASK  /*!< End-Of-Scan flag */
} tsi_sinc_status_flags_t;

/*! @brief TSI low power status flags. */
typedef struct _tsi_lpwr_status_flags
{
    uint8_t   TSIScanCompleteFlag;
    uint8_t   TSILowPower;
    uint8_t   SelfLowPowerChannelBuff;
    uint8_t   SelfLowPowerSavedFlag;
    uint16_t  SelfLowPowerCountBuff;
} tsi_lpwr_status_flags_t;

#define TF_TSI_SELF_CAP_CHANNEL_COUNT 25U
#define TF_TSI_SELF_CAP_CHANNELS_MASK 0x0000000001FFFFFFULL
#define TF_TSI_SELF_CAP_CHANNEL_0  0U
#define TF_TSI_SELF_CAP_CHANNEL_1  1U
#define TF_TSI_SELF_CAP_CHANNEL_2  2U
#define TF_TSI_SELF_CAP_CHANNEL_3  3U
#define TF_TSI_SELF_CAP_CHANNEL_4  4U
#define TF_TSI_SELF_CAP_CHANNEL_5  5U
#define TF_TSI_SELF_CAP_CHANNEL_6  6U
#define TF_TSI_SELF_CAP_CHANNEL_7  7U
#define TF_TSI_SELF_CAP_CHANNEL_8  8U
#define TF_TSI_SELF_CAP_CHANNEL_9  9U
#define TF_TSI_SELF_CAP_CHANNEL_10 10U
#define TF_TSI_SELF_CAP_CHANNEL_11 11U
#define TF_TSI_SELF_CAP_CHANNEL_12 12U
#define TF_TSI_SELF_CAP_CHANNEL_13 13U
#define TF_TSI_SELF_CAP_CHANNEL_14 14U
#define TF_TSI_SELF_CAP_CHANNEL_15 15U
#define TF_TSI_SELF_CAP_CHANNEL_16 16U
#define TF_TSI_SELF_CAP_CHANNEL_17 17U
#define TF_TSI_SELF_CAP_CHANNEL_18 18U
#define TF_TSI_SELF_CAP_CHANNEL_19 19U
#define TF_TSI_SELF_CAP_CHANNEL_20 20U
#define TF_TSI_SELF_CAP_CHANNEL_21 21U
#define TF_TSI_SELF_CAP_CHANNEL_22 22U
#define TF_TSI_SELF_CAP_CHANNEL_23 23U
#define TF_TSI_SELF_CAP_CHANNEL_24 24U

#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT 6U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_6  6U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_7  7U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_8  8U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_9  9U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_10 10U
#define TF_TSI_MUTUAL_CAP_RX_CHANNEL_11 11U

#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_COUNT 6U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_0  0U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_1  1U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_2  2U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_3  3U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_4  4U
#define TF_TSI_MUTUAL_CAP_TX_CHANNEL_5  5U

#define TF_TSI_MUTUAL_CAP_CHANNELS_MASK 0x1FFFFFFFFE000000ULL
#define TF_TSI_TOTAL_CHANNEL_COUNT ((TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT * TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT) + TF_TSI_SELF_CAP_CHANNEL_COUNT)

/* This macro transforms mutual RX, TX electrode numbers into a single electrode number */
#define NT_TSI_TRANSFORM_MUTUAL(RX,TX) (((0ULL + (TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT*(TX))) + ((RX)-TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT)) + TF_TSI_SELF_CAP_CHANNEL_COUNT)

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

/* \} */
#endif /* __FSL_TSI_V5_DRIVER_SPECIFIC_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

