/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
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
#ifndef NT_MODULES_H
#define NT_MODULES_H

/**
 * \defgroup modules Modules
 * \ingroup ntapi
 * Modules represent the data-acquisition layer in the NXP Touch system;
 * it is the layer that is tighly coupled with the hardware module available on
 * the NXP MCU device.
 *
 * Each Module implements a set of functions contained in the nt_module_interface
 * structure. This interface is used by the system to process all modules in
 * a generic way during the data-acquisition or data-processing phases.
 *
 * \{
 */

#include "nt_types.h"
#include "nt_filters.h"
#include "nt_electrodes.h"

// Forward declaration
struct nt_module;
struct nt_electrode;

/**
 * \defgroup gmodules General API
 * \ingroup modules
 * General Function definition of the modules.
 *
 * \{
 */

/** Module's modes. 
 */
enum nt_module_mode {
    NT_MODULE_MODE_NORMAL       = 0,    /**< The module is in a standard touch measure mode. */
    NT_MODULE_MODE_PROXIMITY    = 1,    /**< The module is in a proximity mode. */
    NT_MODULE_MODE_LOW_POWER    = 2,    /**< The module is in a low-power mode. */
};

/** Generic flags for Module processing. 
 */
enum nt_module_flags {
    NT_MODULE_NEW_DATA_FLAG         = 1 << NT_FLAGS_SYSTEM_SHIFT(0),     /**< The new data is ready to be processed. */ 
    NT_MODULE_TRIGGER_DISABLED_FLAG = 1 << NT_FLAGS_SYSTEM_SHIFT(1),     /**< Disables the trigger for the current module (in fact, the module is disabled). */
    NT_MODULE_DIGITAL_RESULTS_FLAG  = 1 << NT_FLAGS_SYSTEM_SHIFT(2),     /**< The digital data only flag (only touch / release information - no analog value). */
    NT_MODULE_OVERFLOW_FLAG         = 1 << NT_FLAGS_SYSTEM_SHIFT(3),     /**< Measured module data out of the specified range */
};

/** Container that covers all possible variants of the module parameters.
 */
union nt_module_params
{
  const struct nt_module_tsi_params     * tsi;  /**< Pointer to the TSI module specific parameters. */
  const struct nt_module_gpio_params    * gpio; /**< Pointer to the GPIO module specific parameters. */
  const struct nt_module_gpioint_params * gpioint; /**< Pointer to the GPIO interrupt module specific parameters. */
};

/**
 *  The main structure representing the Module instance; this structure is used as a base for all
 *  module implementations. The type of the module is specified by selecting the right module interface.
 *
 *  This structure can be allocated in ROM.
 */
struct nt_module {
    const struct nt_module_interface    *interface;     /**< Module interface. Can't be NULL. */
    const struct nt_electrode           * const * electrodes;   /**< A pointer to the list of electrodes. Can't be NULL. */
    const union nt_module_params        module_params;        /**< An instance module params. Can't be NULL.*/
    void                                *config;        /**< A pointer to the harware configuration. Can't be NULL. */
    uint8_t                             instance;       /**< An instance of the module. */
};

/**
 * \defgroup gmodules_api API Functions
 * \ingroup gmodules
 * General API functions of the modules.
 *
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Recalibrate the module.
 *              The function forces the recalibration process of the module to get optimized parameters. 
 * \param module Pointer to the module to be recalibrated.
 * \param configuration Pointer to the module configuration that must be used as a startup configuration for the recalibration.
 * \return The lowest signal measured within the module. // todo is this a good return value?
 * This is an example of recalibrating the module settings of the FT library:
   \code
    // to do
    if(nt_module_recalibrate(&my_nt_module, &my_nt_module) == NT_FAILURE)
    {
      printf("The change of mode for my_nt_module failed.");
    }
    // The FT successfuly chenge mode of my_nt_module
  
   \endcode
 */
  
uint32_t nt_module_recalibrate(const struct nt_module *module, void *configuration);

/**
 * \brief Changes the module mode of the operation.
 * \param module Pointer to the module.
 * \param mode New requested mode of the module.
 * \param electrode Pointer to the electrode used in special modes (low-power & proximity); only one electrode is enabled in these modes.
 * \return
 *   - NT_SUCCESS if the mode was properly changed
 *   - NT_FAILURE if the mode cannot be changed
 * This is an example of changing the mode of the module operation in the FT library:
   \code
    
    if(nt_module_change_mode(&my_nt_module, NT_MODULE_MODE_PROXIMITY, &my_proximity_electrode) == NT_FAILURE)
    {
      printf("The change of mode for my_nt_module failed.");
    }
    // The FT successfuly changed mode of my_nt_module
  
   \endcode
 */
int32_t nt_module_change_mode(struct nt_module *module, const enum nt_module_mode mode, const struct nt_electrode *electrode);

/**
 * \brief Load module configuration for the selected mode.
 *              The function loads the new configuration to the module for the selected mode of operation. 
 * \param module Pointer to the module.
 * \param mode Mode of the module.
 * \param config Pointer to the configuration data of the module, the type is dependent on the target module.
 * \return
 *   - NT_SUCCESS if the load operation was properly done
 *   - NT_FAILURE if the load operation cannot be finished
 * This is an example of loading the configuration data of the module in the FT library:
   \code    
    // I want to load new configuration for the TSI module and the proximity mode
    if(nt_module_load_configuration(&my_nt_module, NT_MODULE_MODE_PROXIMITY, &my_module_tsi_proximity_configuration) == NT_FAILURE)
    {
      printf("Loading of new configuration for the my_nt_module failed.");
    }
    // The FT successfuly loaded the new configuration of the my_nt_module.
  
   \endcode
 */
int32_t nt_module_load_configuration(struct nt_module *module, const enum nt_module_mode mode, const void *config);

/**
 * \brief Saves the module configuration for the selected mode.
 *              The function saves the configuration from the module for the selected mode of operation into the user storage place. 
 * \param module Pointer to the module.
 * \param mode Mode of the module.
 * \param config Pointer to the configuration data variable of the module, where the current configuration should be stored. The type is dependent on the target module.
 * \return
 *   - NT_SUCCESS if the save operation was properly done
 *   - NT_FAILURE if the save operation cannot be finished
 * This is an example of saving the configuration data of the module in the FT library:
   \code    
    // I want to save the configuration of the TSI module and the proximity mode into my variable
    // tsi_config_t my_module_tsi_proximity_configuration;
    if(nt_module_save_configuration(&my_nt_module, NT_MODULE_MODE_PROXIMITY, &my_module_tsi_proximity_configuration) == NT_FAILURE)
    {
      printf("Saving of the current configuration for the my_nt_module failed.");
    }
    // The FT successfuly saved the current configuration of the my_nt_module
  
   \endcode
 */
int32_t nt_module_save_configuration(struct nt_module *module, const enum nt_module_mode mode, void *config);

#ifdef __cplusplus
}
#endif

/** \} end of gmodules_api group */
/** \} end of gmodules group */
/** \} end of modules group */

#endif
