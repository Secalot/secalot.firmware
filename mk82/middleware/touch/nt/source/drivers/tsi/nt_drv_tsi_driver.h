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
#ifndef __FSL_TSI_DRIVER_H__
#define __FSL_TSI_DRIVER_H__

/**
 * \defgroup tsi_drivers TSI Drivers
 * \ingroup ntapi
 *
 * Drivers represent the lowest level of abstraction in TSI peripheral control;
 *
 * Drivers contain common and TSI version specific files. There are functions like
 * \ref NT_TSI_DRV_Init, \ref NT_TSI_DRV_EnableElectrode, \ref NT_TSI_DRV_Measure, etc.
 *
 * \{
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fsl_device_registers.h"
#include "../source/system/nt_system_prv.h"
#if (FSL_FEATURE_TSI_VERSION == 2)
#include "fsl_tsi_v2.h" 
#elif (FSL_FEATURE_TSI_VERSION == 4)
#include "fsl_tsi_v4.h"
#elif (FSL_FEATURE_TSI_VERSION == 5)
#include "fsl_tsi_v5.h"
#include "nt_drv_tsi_v5_driver_specific.h"
#endif

#if FSL_FEATURE_SOC_TSI_COUNT || defined(NT_DOXYGEN)

/**
 * The number of total TSI channels corresponds to FSL_FEATURE_TSI_CHANNEL_COUNT
 * if not defined elsewhere
 */
#ifndef TF_TSI_TOTAL_CHANNEL_COUNT
#define TF_TSI_TOTAL_CHANNEL_COUNT FSL_FEATURE_TSI_CHANNEL_COUNT
#endif

/**
 * \brief Call back routine of TSI driver.
 *
 * The function is called on end of the measure of the TSI driver. The function
 * can be called from interrupt, so the code inside the callback should be short
 * and fast.
 * \param instance - instance of the TSI peripheral
 * \param usrData - user data (type is void*), the user data are specified by function \ref NT_TSI_DRV_SetCallBackFunc
 * \return - none
 */
typedef void (*tsi_callback_t)(uint32_t instance, void* usrData);

/** Error codes for the TSI driver. */
typedef enum _tsi_status
{
  kStatus_TSI_Success = 0,
  kStatus_TSI_Busy,           /*!< TSI still in progress */
  kStatus_TSI_Overflow,       /*!< TSI counter out of range */
  kStatus_TSI_Overrun,        /*!< TSI measurement overrun  */
  kStatus_TSI_LowPower,       /*!< TSI is in low power mode */
  kStatus_TSI_Recalibration,  /*!< TSI is under recalibration process */
  kStatus_TSI_InvalidChannel, /*!< Invalid TSI channel */
  kStatus_TSI_InvalidMode,    /*!< Invalid TSI mode */
  kStatus_TSI_Initialized,    /*!< The driver is initialized and ready to measure */
  kStatus_TSI_Error           /*!< The general driver error */
} tsi_status_t;

/**
* User configuration structure for TSI driver.
*
* Use an instance of this structure with NT_TSI_DRV_Init(). This allows you to configure the
* most common settings of the TSI peripheral with a single function call. Settings include:
*
*/
typedef struct FtTsiUserConfig {
    tsi_config_t                        *config;        /**< A pointer to hardware configuration. Can't be NULL. */
    tsi_callback_t                      pCallBackFunc;  /**< A pointer to call back function of end of measurement. */
    void                                * usrData;      /**< A user data of call back function. */
} nt_tsi_user_config_t;

/**
* Driver operation mode definition.
*
* The operation name definition used for TSI driver.
*
*/
typedef enum FtTsiModes
{
  tsi_OpModeNormal = 0,        /**< The normal mode of TSI. */
  tsi_OpModeProximity,         /**< The proximity sensing mode of TSI. */
  tsi_OpModeLowPower,          /**< The low power mode of TSI. */
  tsi_OpModeNoise,             /**< The noise mode of TSI. This mode is not valid with TSI HW, valid only for the TSIL HW. */
  tsi_OpModeCnt,               /**< Count of TSI modes - for internal use. */
  tsi_OpModeNoChange           /**< The special value of operation mode that allows call for example \ref NT_TSI_DRV_DisableLowPower function without change of operation mode. */
}nt_tsi_modes_t;

/**
* Driver operation mode data hold structure.
*
* This is the operation mode data hold structure. The structure is keep all needed data
* to be driver able to switch the operation modes and properly set up HW peripheral.
*
*/
typedef struct FtTsiOperationMode
{
  uint64_t                              enabledElectrodes;  /**< The back up of enabled electrodes for operation mode */
  tsi_config_t                          config;             /**< A hardware configuration. */
}nt_tsi_operation_mode_t;

/**
* Driver data storage place.
*
* It must be created by the application code and the pointer is handled by the \ref NT_TSI_DRV_Init function
* to driver. The driver keeps all context data for itself run. Settings include:
*
*/
typedef struct FtTsiState {
  tsi_status_t                          status;             /**< Current status of the driver. */
  tsi_callback_t                        pCallBackFunc;      /**< A pointer to call back function of end of measurement. */
  void                                  *usrData;           /**< A user data pointer handled by call back function. */
  nt_mutex_t                            lock;               /**< Used by whole driver to secure the context data integrity. */
  nt_mutex_t                            lockChangeMode;     /**< Used by change mode function to secure the context data integrity. */ 
  nt_tsi_modes_t                        opMode;             /**< Storage of current operation mode. */
  nt_tsi_operation_mode_t               opModesData[tsi_OpModeCnt]; /**< Data storage of individual operational modes. */
  uint16_t                              counters[TF_TSI_TOTAL_CHANNEL_COUNT]; /**< The mirror of last state of counter registers */
}nt_tsi_state_t;


/** Table of base addresses for TSI instances. */
extern TSI_Type * const g_tsiBase[];

/** Table to save TSI IRQ enumeration numbers defined in CMSIS header file. */
extern const IRQn_Type g_tsiIrqId[FSL_FEATURE_SOC_TSI_COUNT];

/** Table to save pointers to context data. */
extern nt_tsi_state_t * g_tsiStatePtr[FSL_FEATURE_SOC_TSI_COUNT];

/*******************************************************************************
 * API
 ******************************************************************************/

/**
 * \defgroup tsi_drivers_api API Functions
 * \ingroup tsi_drivers
 * General Function definition of the drivers.
 *
 * \{ */

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \brief Initializes a TSI instance for operation.
*
* This function initializes the run-time state structure and prepares the
* entire peripheral to measure the capacitances on electrodes.
 \code

   static nt_tsi_state_t myTsiDriverStateStructure;

   nt_tsi_user_config_t myTsiDriveruserConfig =
   {
    .config =
       {
          ...
       },
     .pCallBackFunc = APP_myTsiCallBackFunc,
     .usrData = myData,
   };

   if(NT_TSI_DRV_Init(0, &myTsiDriverStateStructure, &myTsiDriveruserConfig) != kStatus_TSI_Success)
   {
      // Error, the TSI is not initialized
   }
  \endcode
*
* \param instance The TSI module instance.
* \param tsiState A pointer to the TSI driver state structure memory. The user is only
*  responsible to pass in the memory for this run-time state structure where the TSI driver
*  will take care of filling out the members. This run-time state structure keeps track of the
*  current TSI peripheral and driver state.
* \param tsiUserConfig The user configuration structure of type nt_tsi_user_config_t. The user
*   populates the members of this structure and  passes the pointer of this structure
*  into the function.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_Init(uint32_t instance, nt_tsi_state_t * tsiState, const nt_tsi_user_config_t * tsiUserConfig);

/**
* \brief Initializes TSI v5 in both modes (self cap, mutual).
*
*
 \code
   NT_TSI_DRV_InitSpecific(0, &myTsiDriveruserConfig) 
  \endcode
* \param base The TSI module instance.
* \param config The user configuration structure of type tsi_config_t. The user
*   populates the members of this structure and passes the pointer of this structure
*  into the function.
* \return none.
*/
void NT_TSI_DRV_InitSpecific(TSI_Type *base, const tsi_config_t *config);

/**
* \brief Shuts down the TSI by disabling interrupts and the peripheral.
*
* This function disables the TSI interrupts and the peripheral.
*
 \code
   if(NT_TSI_DRV_DeInit(0) != kStatus_TSI_Success)
   {
      // Error, the TSI is not de-initialized
   }
  \endcode
* \param instance The TSI module instance.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_DeInit(uint32_t instance);

/**
* \brief Enables/disables one electrode of the TSI module.
*
* Function must be called for each used electrodes after initialization of TSI driver.
*
  \code
        // On the TSI instance 0, enable electrode with index 5
    if(NT_TSI_DRV_EnableElectrode(0, 5, TRUE) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }
  \endcode
* \param instance   The TSI module instance.
* \param channel    Index of TSI channel.
* \param enable     TRUE - for channel enable, FALSE for disable.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_EnableElectrode(uint32_t instance, const uint32_t channel, const bool enable);

/**
* \brief Returns a mask of the enabled electrodes of the TSI module.
*
* The function returns the mask of the enabled electrodes of the current mode.
*
  \code
    uint32_t enabledElectrodeMask;
    enabledElectrodeMask = NT_TSI_DRV_GetEnabledElectrodes(0);
  \endcode
* \param instance The TSI module instance.
* \return Mask of enabled electrodes for current mode.
*/
uint64_t NT_TSI_DRV_GetEnabledElectrodes(uint32_t instance);

/**
* \brief Starts the measure cycle of the enabled electrodes.
*
* The function is non blocking. Therefore, the results can be obtained after the driver completes the measure cycle.
*         The end of the measure cycle can be checked by pooling the \ref NT_TSI_DRV_GetStatus function or wait for registered callback function by using the
*         \ref NT_TSI_DRV_SetCallBackFunc or \ref NT_TSI_DRV_Init.
*
  \code
    // Example of the pooling style of use of NT_TSI_DRV_Measure() function
    if(NT_TSI_DRV_Measure(0) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }

    while(NT_TSI_DRV_GetStatus(0) != kStatus_TSI_Initialized)
    {
        // Do something useful - don't waste the CPU cycle time
    }

  \endcode
* \param instance The TSI module instance.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_Measure(uint32_t instance);

/**
* \brief Returns the last measured value.
*
* This function returns the last measured value in the previous measure cycle.
*           The data is buffered inside the driver.
*
  \code
    // Get the counter value from TSI instance 0 and 5th channel

    uint32_t result;

    if(NT_TSI_DRV_GetCounter(0, 5, &result) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not read
    }

  \endcode
* \param instance The TSI module instance.
* \param channel The TSI electrode index.
* \param counter The pointer to 16 bit value where will be stored channel counter value.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_GetCounter(uint32_t instance, const uint32_t channel, uint16_t * counter);

/**
* \brief Returns the current status of the driver.
*
* This function returns the current working status of the driver.
*
  \code
    // Get the current status of TSI driver

    tsi_status_t status;

    status = NT_TSI_DRV_GetStatus(0);


  \endcode
* \param instance The TSI module instance.
* \return An current status of the driver.
*/
tsi_status_t NT_TSI_DRV_GetStatus(uint32_t instance);

/**
* \brief Enters the low power mode of the TSI driver.
*
* This function switches the driver to low power mode and immediately enables the
*            low power functionality of the TSI peripheral. Before calling this
*           function, the low power mode must be configured - Enable the right electrode
*           and recalibrate the low power mode to get the best performance for this mode.
*
  \code
    // Switch the driver to the low power mode
    uint16_t signal;

    // The first time is needed to configure the low power mode configuration

    (void)NT_TSI_DRV_ChangeMode(0, tsi_OpModeLowPower); // I don't check the result because I believe in.
    // Enable the right one electrode for low power AKE up operation
    (void)NT_TSI_DRV_EnableElectrode(0, 5, true);
    // Recalibrate the mode to get the best performance for this one electrode
    (void)NT_TSI_DRV_Recalibrate(0, &signal);

    if(NT_TSI_DRV_EnableLowPower(0) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't go to low power mode
    }


  \endcode
* \param instance The TSI module instance.
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_EnableLowPower(uint32_t instance);

/**
* \brief This function returns back the TSI driver from the low power to standard operation
*
* Function switch the driver back form low power mode and it can immediately change
*           the operation mode to any other or keep the driver in low power
*           configuration, to be able go back to low power state.
*
  \code
    // Switch the driver from the low power mode

    if(NT_TSI_DRV_DisableLowPower(0, tsi_OpModeNormal) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't go from low power mode
    }


  \endcode
* \param instance   The TSI module instance.
* \param mode       The new operation mode request
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_DisableLowPower(uint32_t instance, const nt_tsi_modes_t mode);

/**
* \brief Automatically recalibrates all important TSI settings.
*
* This function forces the driver to start the recalibration procedure
*           for the current operation mode to get the best possible TSI hardware settings.
*           The computed setting is stored into the operation mode data and can be
*           loaded and saved by the \ref NT_TSI_DRV_LoadConfiguration or the \ref NT_TSI_DRV_SaveConfiguration
*           functions.
*
* \warning The function could take more time to return
*          and is blocking.
*
  \code
    // Recalibrate current mode
    uint16_t signal;

    // Recalibrate the mode to get the best performance for this one electrode

    if(NT_TSI_DRV_Recalibrate(0, &signal) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't recalibrate this mode
    }


  \endcode
* \param instance   The TSI module instance.
* \param lowestSignal       The pointer to variable where will be store the lowest signal of all electrodes
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_Recalibrate(uint32_t instance, uint32_t * lowestSignal);

/**
* \brief Sets the callback function that is called when the measure cycle ends.
*
* This function sets up or clears, (parameter pFuncCallBack  = NULL), the callback function pointer
*           which is called after each measure cycle ends. The user can also set the custom user data,
*           that is handled by the parameter to a call back function. One function can be called by more sources.
*
  \code
    // Clear previous call back function

    if(NT_TSI_DRV_SetCallBackFunc(0, NULL, NULL) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't set up the call back function at the moment
    }

    // Set new call back function

    if(NT_TSI_DRV_SetCallBackFunc(0, myFunction, (void*)0x12345678) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't set up the call back function at the moment
    }


  \endcode
* \param instance       The TSI module instance.
* \param pFuncCallBack  The pointer to application call back function
* \param usrData        The user data pointer
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_SetCallBackFunc(uint32_t instance, const tsi_callback_t pFuncCallBack, void * usrData);

/**
* \brief Changes the current working operation mode.
*
* This function changes the working operation mode of the driver.
*
  \code
    // Change operation mode to low power

    if(NT_TSI_DRV_ChangeMode(0, tsi_OpModeLowPower) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't change the operation mode into low power
    }

  \endcode
* \param instance       The TSI module instance.
* \param mode           The requested new operation mode
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_ChangeMode(uint32_t instance, const nt_tsi_modes_t mode);

/**
* \brief Returns the current working operation mode.
*
* This function returns the current working operation mode of the driver.
*
  \code
    // Gets current operation mode of TSI driver
    nt_tsi_modes_t mode;

    mode = NT_TSI_DRV_GetMode(0);

  \endcode
* \param instance       The TSI module instance.
* \return An current operation mode of TSI driver.
*/
nt_tsi_modes_t NT_TSI_DRV_GetMode(uint32_t instance);

/**
* \brief Loads the new configuration into a specific mode.
*
* This function loads the new configuration into a specific mode.
*           This can be used when the calibrated data are stored in any NVM
*           to load after startup of the MCU to avoid run recalibration that takes
*           more time.
*
  \code
    // Load operation mode configuration

    extern const nt_tsi_operation_mode_t * myTsiNvmLowPowerConfiguration;

    if(NT_TSI_DRV_LoadConfiguration(0, tsi_OpModeLowPower, myTsiNvmLowPowerConfiguration) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't load the configuration
    }

  \endcode
* \param instance       The TSI module instance.
* \param mode           The requested new operation mode
* \param operationMode  The pointer to storage place of the configuration that should be loaded
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_LoadConfiguration(uint32_t instance, const nt_tsi_modes_t mode, const nt_tsi_operation_mode_t * operationMode);

/**
* \brief Saves the TSI driver configuration for a specific mode.
*
* This function saves the configuration of a specific mode.
*           This can be used when the calibrated data should be stored in any backup memory
*           to load after the start of the MCU to avoid running the recalibration that takes 
*           more time.
*
  \code
    // Save operation mode configuration

    extern nt_tsi_operation_mode_t  myTsiNvmLowPowerConfiguration;

    if(NT_TSI_DRV_SaveConfiguration(0, tsi_OpModeLowPower, &myTsiNvmLowPowerConfiguration) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't save the configuration
    }

  \endcode
* \param instance       The TSI module instance.
* \param mode           The requested new operation mode
* \param operationMode  The pointer to storage place of the configuration that should be save
* \return An error code or kStatus_TSI_Success.
*/
tsi_status_t NT_TSI_DRV_SaveConfiguration(uint32_t instance, const nt_tsi_modes_t mode, nt_tsi_operation_mode_t * operationMode);

#if defined(__cplusplus)
}
#endif

/** \} */ //end of tsi_drivers_api group 
/** \} */ //end of tsi_drivers group 

#endif
#endif /* __FSL_TSI_DRIVER_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
