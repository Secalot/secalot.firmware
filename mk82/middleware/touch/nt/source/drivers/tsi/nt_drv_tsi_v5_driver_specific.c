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
#include <string.h>
#include "nt_drv_tsi_driver.h"
#if FSL_FEATURE_SOC_TSI_COUNT
/*******************************************************************************
 * Definitions
 ******************************************************************************/
extern IRQn_Type tsi_irq_ids[FSL_FEATURE_SOC_TSI_COUNT];
extern void TSI_DRV_IRQHandler0(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile tsi_lpwr_status_flags_t tsi_lpwr_status;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_Init
* Description   : Encapsulates TSI v5 init.
*
*END**************************************************************************/
void NT_TSI_DRV_InitSpecific(TSI_Type *base, const tsi_config_t *config)
{
    TSI_InitSelfCapMode(base, &config->configSelfCap);
    TSI_EnableModule(base, true);
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_EnableElectrode
* Description   : Enables/Disables the electrode for measuring.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_EnableElectrode(uint32_t instance, const uint32_t channel, const bool enable)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);
    NT_ASSERT(channel < TF_TSI_TOTAL_CHANNEL_COUNT);
    
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];

    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

    if (tsiState->status != kStatus_TSI_Initialized)
    {
      /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);
        
        return tsiState->status;
    }

    /* Check the condition for low power mode. */
    if((tsiState->opMode == tsi_OpModeLowPower) || (tsiState->opMode == tsi_OpModeProximity))
    {
        tsiState->opModesData[tsiState->opMode].enabledElectrodes = 0;
    }

    if(enable)
    {
        tsiState->opModesData[tsiState->opMode].enabledElectrodes |= (1ULL << channel);
    }
    else
    {
        tsiState->opModesData[tsiState->opMode].enabledElectrodes &= ~(1ULL << channel);
    }
    
    /* Check self/mutual setting feasibility */
    if(channel > (TF_TSI_SELF_CAP_CHANNEL_COUNT-1U))
    {
        NT_ASSERT(!(tsiState->opModesData[tsiState->opMode].enabledElectrodes & (1ULL<<((channel-TF_TSI_SELF_CAP_CHANNEL_COUNT)/TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT))));
        NT_ASSERT(!(tsiState->opModesData[tsiState->opMode].enabledElectrodes & (1ULL<<(((channel-TF_TSI_SELF_CAP_CHANNEL_COUNT)%TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT)+TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT))));
    }

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_GetCounter
* Description   : Function returns the counter value of selected channel
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_GetCounter(uint32_t instance, const uint32_t channel, uint16_t * counter)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);
    NT_ASSERT(channel < TF_TSI_TOTAL_CHANNEL_COUNT);
    NT_ASSERT(counter);

    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
      
    if(!((1ULL << channel) & (tsiState->opModesData[tsiState->opMode].enabledElectrodes)))   /* Check the channel number. */
    {
        return kStatus_TSI_InvalidChannel;
    }

    *counter = tsiState->counters[channel];

    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_Measure
* Description   : This function gets (measure) capacitance of enabled electrodes
*               from the TSI module using a non-blocking method.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_Measure(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
    uint64_t    pen;
    uint32_t    first_pen;
    uint32_t    mutual_rx, mutual_tx;
    
    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

    if (tsiState->status != kStatus_TSI_Initialized)
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return tsiState->status;
    }

    if(!tsiState->opModesData[tsiState->opMode].enabledElectrodes)
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return kStatus_TSI_InvalidChannel;
    }
    
    tsiState->status = kStatus_TSI_Busy;

    /* If there is a self-cap electrode used, initialize self-cap sensing*/
    if(tsiState->opModesData[tsiState->opMode].enabledElectrodes & 0x01FFFFFFU)
    {
        /* Find electrode with the lowest index */
        first_pen = 0U;
        pen = tsiState->opModesData[tsiState->opMode].enabledElectrodes;
        while (((pen >> first_pen) & 0x1U) == 0U) {
            first_pen++;
        }
    
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        /* Init self-cap sensing */
        TSI_EnableModule(base, false);
        TSI_InitSelfCapMode(base, &tsiState->opModesData[tsiState->opMode].config.configSelfCap);
        TSI_SetSelfCapMeasuredChannel(base, first_pen);
        TSI_EnableHardwareTriggerScan(base, false);
        TSI_EnableModule(base, true);
        TSI_StartSoftwareTrigger(base);
    }
    /* If there are no self-cap electrodes used, initialize mutual-cap sensing*/
    else
    {
        /* Find electrode with the lowest index */
        first_pen = TF_TSI_SELF_CAP_CHANNEL_COUNT;
        pen = tsiState->opModesData[tsiState->opMode].enabledElectrodes;
        while (((pen >> first_pen) & 0x1U) == 0U) {
            first_pen++;
        }
        
        /*Parse electrode number into rx, tx components*/
        mutual_tx = (first_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) / TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT;
        mutual_rx = (first_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) % TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT;
        
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        /* Init mutual-cap sensing */
        TSI_EnableModule(base, false);
        TSI_InitMutualCapMode(base, &tsiState->opModesData[tsiState->opMode].config.configMutual);
        TSI_SetMutualCapTxChannel(base, (tsi_mutual_tx_channel_t)mutual_tx);
        TSI_SetMutualCapRxChannel(base, (tsi_mutual_rx_channel_t)mutual_rx);
        TSI_EnableHardwareTriggerScan(base, false);
        TSI_EnableModule(base, true);
        TSI_StartSoftwareTrigger(base);
    }
    
    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_EnableLowPower
* Description   : Enables/Disables the low power module.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_EnableLowPower(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
    tsi_status_t status;

    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }
        
    if ((status = NT_TSI_DRV_ChangeMode(instance, tsi_OpModeLowPower)) != kStatus_TSI_Success)
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return status;
    }

    if(tsiState->opModesData[tsiState->opMode].enabledElectrodes == 0)
    {
      /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return kStatus_TSI_InvalidChannel;
    }
    
    tsiState->status = kStatus_TSI_LowPower;
    
    /* Enable TSI to run in STOP and VLPS mode */
    TSI_EnableModule(base, false);
    TSI_EnableLowPower(base, true);
    TSI_EnableModule(base, true);
    
    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return kStatus_TSI_Success;
}


/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_ChangeMode
* Description   : The function change the current mode.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_ChangeMode(uint32_t instance, const nt_tsi_modes_t mode)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];

    if((mode == tsiState->opMode) || (mode == tsi_OpModeNoChange))
    {
        return  kStatus_TSI_Success;
    }

    if(mode >= tsi_OpModeCnt)
    {
        return kStatus_TSI_InvalidMode;
    }

    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lockChangeMode, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

    if (tsiState->status != kStatus_TSI_Initialized)
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lockChangeMode);
        
        return tsiState->status;
    }
        
    tsiState->opMode = mode;

    NT_TSI_DRV_InitSpecific(base, &tsiState->opModesData[mode].config);

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lockChangeMode);

    return  kStatus_TSI_Success;
}


/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_LoadConfiguration
* Description   : The function load the configuration for one mode of operation.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_LoadConfiguration(uint32_t instance, const nt_tsi_modes_t mode, const nt_tsi_operation_mode_t * operationMode)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);
    NT_ASSERT(operationMode);
    TSI_Type * base;
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
    
    if(mode >= tsi_OpModeCnt)
    {
        return kStatus_TSI_InvalidMode;
    }

    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

    tsiState->opModesData[mode] = *operationMode;

    /* In case that the loaded configuration is active one, update the HW also. */
    if(mode == tsiState->opMode)
    {
        base = g_tsiBase[instance];

        NT_TSI_DRV_InitSpecific(base, &tsiState->opModesData[mode].config);
        TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
        TSI_EnableInterrupts(base, kTSI_EndOfScanInterruptEnable);
    }

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return  kStatus_TSI_Success;
}

/*!
* @brief Get Sinc status flags
* This function get tsi Sinc status flags.
*
* @param    base  TSI peripheral base address.
* @return         The mask of these status flags combination.
*/
static inline uint32_t TSI_GetSincFlags(TSI_Type *base)
{
    return (base->SINC & (kTSI_EndOfScanFlag | kTSI_OutOfRangeFlag));
}

/*!
 * @brief Interrupt handler for TSI.
 * This handler uses the tsi State structure to handle the instance depend data.
 * This is not a public API as it is called whenever an interrupt occurs.
 */
void TSI_DRV_IRQHandler(uint32_t instance)
{
    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
    uint64_t channels = tsiState->opModesData[tsiState->opMode].enabledElectrodes;
    uint64_t next_pen, pen;
    uint32_t curr_channel;

    if(TSI_GetSincFlags(base) & kTSI_SincOverflowFlag)
    {
      tsiState->status = kStatus_TSI_Overflow;
    }
    
    if(tsi_lpwr_status.TSILowPower)
    {
      tsi_lpwr_status.SelfLowPowerCountBuff =  TSI_GetCounter(base);
      tsi_lpwr_status.SelfLowPowerChannelBuff = TSI_GetSelfCapMeasuredChannel(base);
      tsi_lpwr_status.TSILowPower = 0;
    }
    
    /* Clear interrupt flags. */
    TSI_ClearStatusFlags(base, kTSI_OutOfRangeFlag);
    TSI_ClearStatusFlags(base, kTSI_EndOfScanFlag);
    
    /* self-cap electrode was measured */
    if(!(TSI_GetSensingMode(base)))
    {
        curr_channel = TSI_GetSelfCapMeasuredChannel(base);
        
        /* Save TSICNT value */
        if((uint64_t)(1 << curr_channel) & channels)
        {
            tsiState->counters[curr_channel] = ((uint16_t)0xFFFF - TSI_GetCounter(base));
        }
        
        /* Find next enabled electrode */
        next_pen = curr_channel + 1;
        pen = channels;
        while (((((pen >> next_pen) & 0x1U)) == 0U) && (next_pen < (TF_TSI_TOTAL_CHANNEL_COUNT-1U))) 
        {
            next_pen++;
        }
        
        /* If the next electrode is self-cap, get ready for next self-cap measurement */
        if(next_pen < TF_TSI_SELF_CAP_CHANNEL_COUNT)
        {
            /* Measurement must continue on next channel. */
            TSI_SetSelfCapMeasuredChannel(base, next_pen);
            TSI_StartSoftwareTrigger(base);
            tsi_lpwr_status.TSIScanCompleteFlag = 0;
            return;
        }
        /* If the next electrode is mutual-cap, get ready for next mutual-cap measurement (init TSI for mutual-cap)*/
        else if(next_pen < TF_TSI_TOTAL_CHANNEL_COUNT)
        {
            TSI_EnableModule(base, false);
            TSI_InitMutualCapMode(base, &tsiState->opModesData[tsiState->opMode].config.configMutual);
            TSI_SetMutualCapTxChannel(base, (tsi_mutual_tx_channel_t)((next_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) / TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT));
            TSI_SetMutualCapRxChannel(base, (tsi_mutual_rx_channel_t)((next_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) % TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT));
            TSI_EnableHardwareTriggerScan(base, false);
            TSI_EnableModule(base, true);
            TSI_StartSoftwareTrigger(base);
            tsi_lpwr_status.TSIScanCompleteFlag = 0;
            return;
        }
        else
        {
          tsi_lpwr_status.TSIScanCompleteFlag = 1;
        }
    }
    
    /* mutual-cap electrode was measured */
    else
    {
        curr_channel = (uint32_t)TSI_GetRxMutualCapMeasuredChannel(base) + (uint32_t)TSI_GetTxMutualCapMeasuredChannel(base)*TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT + TF_TSI_SELF_CAP_CHANNEL_COUNT;
        
        /* Save TSICNT value, artificially increase sensitivity and inverse counter value */
        if(((uint64_t)1 << curr_channel) & channels)
        {
            tsiState->counters[curr_channel] = TSI_GetCounter(base);
        }
        
        /* Find next enabled electrode */
        next_pen = curr_channel + 1;
        pen = channels;
        while (((((pen >> next_pen) & 0x1U)) == 0U) && (next_pen < (TF_TSI_TOTAL_CHANNEL_COUNT))) 
        {
            next_pen++;
        }
        
        /* Get ready for next mutual-cap measurement if there is any */
        if(next_pen < TF_TSI_TOTAL_CHANNEL_COUNT)
        {
            TSI_SetMutualCapTxChannel(base, (tsi_mutual_tx_channel_t)((next_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) / TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT));
            TSI_SetMutualCapRxChannel(base, (tsi_mutual_rx_channel_t)((next_pen - TF_TSI_SELF_CAP_CHANNEL_COUNT) % TF_TSI_MUTUAL_CAP_RX_CHANNEL_COUNT));
            TSI_StartSoftwareTrigger(base);
            return;
        }
        else
        {
          tsi_lpwr_status.TSIScanCompleteFlag = 1;
        }
    }
    
    if(tsiState->pCallBackFunc)
    {
        tsiState->pCallBackFunc(instance, tsiState->usrData);
    }

    if(tsiState->status != kStatus_TSI_LowPower)
    {
        /* Return status of the driver to initialized state */
        tsiState->status = kStatus_TSI_Initialized;
    }
}

#endif

/*******************************************************************************
 * EOF
 ******************************************************************************/

