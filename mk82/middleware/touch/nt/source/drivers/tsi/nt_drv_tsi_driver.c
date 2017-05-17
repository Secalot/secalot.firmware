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
 * Local functions
 ******************************************************************************/
static uint32_t TSI_HAL_Recalibrate(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_Init
* Description   : Initialize whole the TSI peripheral to be ready to read capacitance changes
* To initialize the TSI driver, the configuration structure should be handled.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_Init(uint32_t instance, nt_tsi_state_t * tsiState, const nt_tsi_user_config_t * tsiUserConfig)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiSt = g_tsiStatePtr[instance];

    /* OS: start of critical section, critical code will not be preemted */
    NT_OSA_EnterCritical();

    /* Exit if current instance is already initialized. */
    if(tsiSt)
    {
      /* OS: End of critical section. */
        NT_OSA_ExitCritical();

        return kStatus_TSI_Initialized;
    }
    /* Save runtime structure pointer.*/
    tsiSt = g_tsiStatePtr[instance] = tsiState;

    /* Clear the state structure for this instance. */
    memset(tsiSt, 0, sizeof(nt_tsi_state_t));

    /* OS: create the mutex used by whole driver. */
    NT_OSA_MutexCreate(&tsiSt->lock);
    /* OS: create the mutex used by change mode function. */
    NT_OSA_MutexCreate(&tsiSt->lockChangeMode);
    
    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiSt->lock, OSA_WAIT_FOREVER))
    {
        /* TODO: OS End of critical section. */
        NT_OSA_ExitCritical();
        return kStatus_TSI_Error;
    }

    /* OS: End of critical section. */
    NT_OSA_ExitCritical();

    tsiSt->opMode = tsi_OpModeNormal;

    tsiSt->opModesData[tsiSt->opMode].config = *tsiUserConfig->config; /* Store the hardware configuration. */

    tsiSt->pCallBackFunc = tsiUserConfig->pCallBackFunc;
    tsiSt->usrData = tsiUserConfig->usrData;

    NT_TSI_DRV_InitSpecific(base, &tsiSt->opModesData[tsiSt->opMode].config);
    
    /* Clear possible pending flags */
    TSI_ClearStatusFlags(base,(kTSI_EndOfScanFlag | kTSI_OutOfRangeFlag));
    
    TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
    TSI_EnableInterrupts(base, kTSI_EndOfScanInterruptEnable);

#if (FSL_FEATURE_TSI_VERSION == 2)
    TSI_EnablePeriodicalScan(base, false);
#elif (FSL_FEATURE_TSI_VERSION == 4)
    TSI_EnableHardwareTriggerScan(base, false);
#endif

    /* Disable all electrodes */
    tsiState->opModesData[tsiState->opMode].enabledElectrodes = 0;
    
    /* Enable TSI interrupt on NVIC level. */
    EnableIRQ(g_tsiIrqId[instance]);

    tsiSt->status = kStatus_TSI_Initialized;

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiSt->lock);

    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_DeInit
* Description   : De initialize whole the TSI peripheral and driver to be ready
* for any future use and don't load the system.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_DeInit(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    TSI_Type * base = g_tsiBase[instance];
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];

    if (tsiState == NULL)
    {
        return kStatus_TSI_Error;
    }

    TSI_DisableInterrupts(base, kTSI_GlobalInterruptEnable);
    tsiState->opModesData[tsiState->opMode].enabledElectrodes = 0;
    TSI_ClearStatusFlags(base, kTSI_OutOfRangeFlag);
    TSI_ClearStatusFlags(base, kTSI_EndOfScanFlag);
    TSI_EnableModule(base, false);

    /* Disable the interrupt */
    DisableIRQ(g_tsiIrqId[instance]);

    /* Clear runtime structure pointer.*/
    tsiState = NULL;

    /* Gate TSI module clock */
    CLOCK_DisableClock(kCLOCK_Tsi0);
    
    /*Deinit TSI module*/
    TSI_Deinit(base);

    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_SetCallBackFunc
* Description   : Set the TSI call back function pointer for non blocking measurement
*
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_SetCallBackFunc(uint32_t instance, const tsi_callback_t pFuncCallBack, void * usrData)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);
#ifdef NT_OSA
    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];
#endif
    
    /* OS: Critical section. Access to global variable */
    if (knt_Status_OSA_Success != NT_OSA_MutexLock(&tsiState->lock, OSA_WAIT_FOREVER))
    {
        return kStatus_TSI_Error;
    }

    if (g_tsiStatePtr[instance]->status != kStatus_TSI_Initialized)
    {
      /* OS: End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return g_tsiStatePtr[instance]->status;
    }

    g_tsiStatePtr[instance]->pCallBackFunc = pFuncCallBack;
    g_tsiStatePtr[instance]->usrData = usrData;

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_GetEnabledElectrodes
* Description   : Get Enables electrodes for measuring.
*
*END**************************************************************************/
uint64_t NT_TSI_DRV_GetEnabledElectrodes(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    nt_tsi_state_t * tsiState = g_tsiStatePtr[instance];

    return tsiState->opModesData[tsiState->opMode].enabledElectrodes;
}

/*FUNCTION**********************************************************************
*
* Function Name : TSI_DRV_IsBusy
* Description   : Function returns the busy state of the driver
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_GetStatus(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    return g_tsiStatePtr[instance]->status;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_Recalibrate
* Description   : The function force the recalibration process of TSI parameters.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_Recalibrate(uint32_t instance, uint32_t * lowestSignal)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

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

    tsiState->status = kStatus_TSI_Recalibration;

    *lowestSignal = TSI_HAL_Recalibrate();

    tsiState->status = kStatus_TSI_Initialized;

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    if(*lowestSignal == 0)
    {
      return kStatus_TSI_Error;
    }
    else
    {
      return kStatus_TSI_Success;
    }
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_DisableLowPower
* Description   : Enables/Disables the low power module.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_DisableLowPower(uint32_t instance, const nt_tsi_modes_t mode)
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

    if (tsiState->status != kStatus_TSI_LowPower)
    {
        /* TODO: OS End of critical section. */
        NT_OSA_MutexUnlock(&tsiState->lock);

        return tsiState->status;
    }
    TSI_EnableLowPower(base, false);
    TSI_EnableInterrupts(base, kTSI_GlobalInterruptEnable);
    TSI_EnableInterrupts(base, kTSI_EndOfScanInterruptEnable);
    
#if (FSL_FEATURE_TSI_VERSION == 2)
    TSI_EnablePeriodicalScan(base, false);
#elif (FSL_FEATURE_TSI_VERSION == 4)
    TSI_EnableHardwareTriggerScan(base, false);
#endif

    tsiState->status = kStatus_TSI_Initialized;

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    status = NT_TSI_DRV_ChangeMode(instance, mode);

    return status;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_GetMode
* Description   : Function returns the current mode of the driver
*
*END**************************************************************************/
nt_tsi_modes_t NT_TSI_DRV_GetMode(uint32_t instance)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);

    return g_tsiStatePtr[instance]->opMode;
}

/*FUNCTION**********************************************************************
*
* Function Name : NT_TSI_DRV_SaveConfiguration
* Description   : The function save the configuration for one mode of operation.
*
*END**************************************************************************/
tsi_status_t NT_TSI_DRV_SaveConfiguration(uint32_t instance, const nt_tsi_modes_t mode, nt_tsi_operation_mode_t * operationMode)
{
    NT_ASSERT(instance < FSL_FEATURE_SOC_TSI_COUNT);
    NT_ASSERT(operationMode);
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

    *operationMode =  tsiState->opModesData[mode];

    /* OS: End of critical section. */
    NT_OSA_MutexUnlock(&tsiState->lock);

    return  kStatus_TSI_Success;
}

/*FUNCTION**********************************************************************
*
* Empty function recalibrate, will be done later if needed
*
*END**************************************************************************/
static uint32_t TSI_HAL_Recalibrate(void)
{
    return 0;
}

#endif

/*******************************************************************************
 * EOF
 ******************************************************************************/

