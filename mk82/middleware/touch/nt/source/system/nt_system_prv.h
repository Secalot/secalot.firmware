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
#ifndef NT_SYSTEM_PRV_H
#define NT_SYSTEM_PRV_H

#include "nt_system.h"
#include "../source/system/nt_system_mem_prv.h"
/**
 * \defgroup ntapi_private NXP Touch Private API
 *
 * The functions documented in this module are the private ones used by the library
 * itself. All the API here is just documented and not designed to be used by the user.
 * \{
 */

/**
 * \defgroup system_private System
 / \ingroup ntapi_private
 *
 * The system private API and definitions.
 * \{
 */

/*! @brief Constant to pass as timeout value in order to wait indefinitely. */
#define OSA_WAIT_FOREVER  0xFFFFFFFFU

/** TODO: pointer of type tf-mutex_t should point to OS mutex structure
*/
typedef void* nt_mutex_t;

/*! @brief Defines the return status of OSA's functions */
typedef enum _nt_osa_status_t
{
    knt_Status_OSA_Success = 0U, /*!< Success */
    knt_Status_OSA_Error   = 1U, /*!< Failed */
    knt_Status_OSA_Timeout = 2U, /*!< Timeout occurs while waiting */
    knt_Status_OSA_Idle    = 3U  /*!< Used for bare metal only, the wait object is not ready
                                and timeout still not occur */
} nt_osa_status_t;

/** Internal Module function call identifier
 */
enum nt_system_module_call {
    NT_SYSTEM_MODULE_INIT       = 0,    ///< Do module initialization 
    NT_SYSTEM_MODULE_TRIGGER    = 1,    ///< Send trigger event to module 
    NT_SYSTEM_MODULE_PROCESS    = 2,    ///< Do process data in the module
    NT_SYSTEM_MODULE_CHECK_DATA = 3,    ///< Check the module data
};

/** Internal Controls function call identifier
 */
enum nt_system_control_call {
    NT_SYSTEM_CONTROL_INIT       = 0,   ///< Do control initialization
    NT_SYSTEM_CONTROL_PROCESS    = 1,   ///< Process the new data of control
    NT_SYSTEM_CONTROL_OVERRUN    = 2,   ///< Control data are overrun 
    NT_SYSTEM_CONTROL_DATA_READY = 3,   ///< Control data are ready
};

/**
 * System RAM structure used to store volatile parameters, counter, and system callback
 * functions. This is the only statically placed RAM variable in the whole NXP Touch library.
 */
struct nt_kernel {
    const struct nt_system      *rom;           /**< Pointer to the system parameters. */
    struct nt_control_data      **controls;     /**< Pointer to the list of controls. Can't be NULL. */
    struct nt_module_data       **modules;      /**< Pointer to the list of modules. Can't be NULL. */
    uint8_t                     controls_cnt;   /**< Count of the controls. */
    uint8_t                     modules_cnt;    /**< Count of the modules. */
    uint32_t                    time_counter;   /**< Time counter. */
    nt_system_callback          callback;       /**< System event handler. */
    struct nt_mem               memory;         /**< System Memory handler */
#if (NT_DEBUG != 0)
    nt_error_callback           error_callback; /**< User Error handler */
#endif
};

/**
 * \defgroup system_api_private API Functions
 * \ingroup system_private
 * General Private Function definition of system.
 *
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif
 
  
/**
 * \brief Obtain a pointer to the system.
 * \return A pointer to the system kernel structure.
 */
struct nt_kernel *_nt_system_get(void);

/**
 * \brief Increments the system counter.
 * \return None.
 */
void _nt_system_increment_time_counter(void);

/**
 * \brief Get time period.
 * \return Time period.
 */
uint32_t _nt_system_get_time_period(void);

/**
 * \brief Time offset by a defined period.
 * \param event_period Defined event period.
 * \return 0 if event_period period modulo current counter is equal to 0
 *         positive number otherwise.
 *
 *  This function is used to find out if an event can be invoked in its defined period.
 */
uint32_t _nt_system_get_time_offset_from_period(uint32_t event_period);

/**
 * \brief Invoke the module function based on the option parameter.
 * \param option One of the options from the nt_system_module_call enum
 * \return
 *   - NT_SUCCESS if the module's action was carried out succesfully,
 *   - NT_FAILURE if the module's action failed.
 */
int32_t _nt_system_module_function(uint32_t option);

/**
 * \brief Invoke the control function based on the option parameter.
 * \param option One of the options from nt_system_control_call enum
 * \return
 *   - NT_SUCCESS if the control's action was carried out succesfully,
 *   - NT_FAILURE if the control's action failed.
 */
int32_t _nt_system_control_function(uint32_t option);

/**
 * \brief Initialize system.
 * \param system Pointer to the user system parameters structure.
 * \return
 *   - NT_SUCCESS if the system data are set correctly,
 *   - NT_FAILURE if the system data check failed.
 */
int32_t _nt_system_init(const struct nt_system *system);

/**
 * \brief System callback invocation.
 * \param event  Callback event.
 * \return None.
 */
void _nt_system_invoke_callback(uint32_t event);

/**
 * \brief Function used internally to detect, whether new \ref modules data are available and
 * to set the same flag for the controls. This function also invokes the control callbacks.
 * \return None.
 */
void _nt_system_modules_data_ready(void);

/**
 * \brief Find the n-th instance of a module of a specified type.
 * \param interface_address Address to the module's interface (uniquely identifies module type).
 * \param instance Zero-based module instance index.
 * \return
 *   - valid pointer to module.
 *   - NULL if the module was not found
 */
const struct nt_module *_nt_system_get_module(uint32_t interface_address,
                                             uint32_t instance);

/**
 * \brief The NT error function that is invoked from NT asserts
 * \param file_name Pointer to the file name.
 * \param line Number of the line which was asserted.
 * \return none
 */
void nt_error(char *file_name, uint32_t line);

/**
 * \defgroup nt_osa NT_OSA
 / \ingroup system_api_private
 *
 * NT_OSA
 * \{
 */

#if defined(NT_OSA) || defined(NT_DOXYGEN)
/**
 * Initializes NT own OSA, user must define the function body according 
 * his OS.
 * \return none
 */
extern int32_t NT_OSA_Init();

/**
 * Start of critical section, critical code will not be preemted,
 * user must define the function body according his OS.
 * \return none
 */
extern int32_t NT_OSA_EnterCritical();

/**
 * End of critical section, critical code will not be preemted,
 * user must define the function body according his OS.
 * \return none
 */
extern int32_t NT_OSA_ExitCritical();

/**
 * Create mutex that handles shared resources,
 * user must define the function body according his OS.
 * \param ptr_mtx Pointer to mutex.
 * \return none
 */
extern int32_t NT_OSA_MutexCreate(nt_mutex_t *ptr_mtx);

/**
 * Lock mutex that handles shared resources,
 * user must define the function body according his OS.
 * \param mutex mutex structure.
 * \param timeout timeout for locking the mutex.
 * \return none
 */
extern int32_t NT_OSA_MutexLock(nt_mutex_t mutex, uint32_t timeout);

/**
 * Unlock mutex that handles shared resources,
 * user must define the function body according his OS.
 * \param mutex mutex structure.
 * \return none
 */
extern int32_t NT_OSA_MutexUnlock(nt_mutex_t mutex);
#else
    #define NT_OSA_Init()
    #define NT_OSA_EnterCritical()
    #define NT_OSA_ExitCritical()
    #define NT_OSA_MutexCreate(ptr_mtx)
    #define NT_OSA_MutexLock(mutex, timeout) knt_Status_OSA_Success
    #define NT_OSA_MutexUnlock(mutex)
#endif

/** \} end of nt_osa group */
#ifdef __cplusplus
}
#endif

/** \} end of system_api_private group */
/** \} end of system_private group */
/** \} end of ntapi_private group */

#endif
