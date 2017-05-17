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

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_Init
* Description   : Encapsulates TSI v2 init.
*
*END**************************************************************************/
void NT_TSI_DRV_InitSpecific(TSI_Type *base, const tsi_config_t *config)
{
    TSI_Init(base, config);
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
    NT_ASSERT(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);

    TSI_Type * base = g_tsiBase[instance];
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
      if(tsiState->opModesData[tsi_OpModeLowPower].enabledElectrodes != 0)
      {
          /* Only one elctrode can be enabled in low power mode and proximity. */

          /* Disable al previous enabled. */
          TSI_EnableChannels(base, 0xffff, false);
      }
    }

    if(enable)
    {
        tsiState->opModesData[tsiState->opMode].enabledElectrodes |= (1U << channel);
        TSI_EnableChannel(base, channel, true);
    }
    else
    {
        tsiState->opModesData[tsiState->opMode].enabledElectrodes &= ~(1U << channel);
        TSI_EnableChannel(base, channel, false);
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
    NT_ASSERT(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);
    NT_ASSERT(counter);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];

    if(!TSI_IsChannelEnabled(base, channel))
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

    tsiState->status = kStatus_TSI_Busy;

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    TSI_EnableModule(base, false);
    TSI_EnablePeriodicalScan(base, false);
    TSI_EnableModule(base, true);
    TSI_StartSoftwareTrigger(base);

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
    uint32_t i;
    int32_t channel = -1;

    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

   if((tsiState->opModesData[tsiState->opMode].config.thresl == 0) || (tsiState->opModesData[tsiState->opMode].config.thresh == 0))
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

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

    for(i = 0; i < FSL_FEATURE_TSI_CHANNEL_COUNT; i++)
    {
        if((uint32_t)(1 << i) & tsiState->opModesData[tsiState->opMode].enabledElectrodes)
        {
            channel = i;
            break;
        }
    }
    
    if(channel == -1)
    {
        /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return kStatus_TSI_InvalidChannel;  
    }
    
    tsiState->status = kStatus_TSI_LowPower;
    
    /* Configurate the peripheral for next use */
    TSI_EnableInterrupts(base, kTSI_OutOfRangeInterruptEnable);
    TSI_EnablePeriodicalScan(base, true);
    TSI_SetLowPowerChannel(base, channel);
    TSI_EnableLowPower(base, true);
    TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
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

    if(mode >= tsi_OpModeNoise) /* Neither the noise mode is not supported in TSIv1&2 revision. */
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
        NT_OSA_MutexUnlock(&tsiState->lock);
        return tsiState->status;
    }

    tsiState->opMode = mode;

    TSI_Init(base, &tsiState->opModesData[mode].config);

    /* Disable all electrodes */
    TSI_EnableChannels(base, 0xffff, false);

    /* Enable the set electrodes for current operation mode */
    TSI_EnableChannels(base, tsiState->opModesData[mode].enabledElectrodes, true);

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

        TSI_Init(base, &tsiState->opModesData[mode].config);

        /* Disable all electrodes */
        TSI_EnableChannels(base, 0xffff, false);

        /* Enable the set electrodes for current operation mode */
        TSI_EnableChannels(base, tsiState->opModesData[mode].enabledElectrodes, true);

        TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
        TSI_EnableInterrupts(base, kTSI_EndOfScanInterruptEnable);
    }

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return  kStatus_TSI_Success;
}

/*!
 * @brief Interrupt handler for TSI.
 * This handler uses the tsi State structure to handle the instance depend data.
 * This is not a public API as it is called whenever an interrupt occurs.
 */
void TSI_DRV_IRQHandler(uint32_t instance)
{
    TSI_Type * base = g_tsiBase[instance];
    uint32_t channels = TSI_GetEnabledChannels(base);
    uint32_t i;
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
    
    /* Check if a measure is running and wanted. */
    if(tsiState->status != kStatus_TSI_Busy)
    {
        return;
    }
        
    if(TSI_GetStatusFlags(base) & kTSI_ExternalElectrodeErrorFlag)
    {
      tsiState->status = kStatus_TSI_Overflow;
    }
    
    if(TSI_GetStatusFlags(base) & kTSI_OverrunErrorFlag)
    {
      tsiState->status = kStatus_TSI_Overrun;    
    }
    
    /* Clear all flags */
    TSI_ClearStatusFlags(base,(kTSI_EndOfScanFlag | kTSI_OverrunErrorFlag | 
                        kTSI_OutOfRangeFlag | kTSI_ExternalElectrodeErrorFlag));

    for(i = 0; i < FSL_FEATURE_TSI_CHANNEL_COUNT; i++)
    {
        if((uint32_t)(1 << i) & channels)
        {
            tsiState->counters[i] = TSI_GetNormalModeCounter(base, i);
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

