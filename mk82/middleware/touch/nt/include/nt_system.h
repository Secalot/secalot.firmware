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
#ifndef NT_SYSTEM_H
#define NT_SYSTEM_H

#include "nt_electrodes.h"
#include "nt_modules.h"
#include "nt_controls.h"

/**
 * \defgroup system System
 * \ingroup ntapi
 * The System structure represents the NXP Touch Library in the user application;
 * it is represented by the \ref nt_system structure, which contains further references to
 * all other application objects like: \ref electrodes, \ref detectors, \ref modules, and
 * \ref controls.
 *
 * The nt_system structure is allocated and initialized by the user, in order to define
 * library configuration, including low-level electrode hardware channels and high-level
 * control parameters. Just like all other structure types, it is up to the user, whether
 * an instance of this structure is allocated statically in compile-time or dynamically.
 * The examples provided with the NXP Touch library show the static allocation
 * and initialization of nt_system along with all other related data structures.
 *
 * \{
 */

/** System callbacks events.
 */
enum nt_system_event {
    NT_SYSTEM_EVENT_OVERRUN    = 0,     ///< Data has been overrun
    NT_SYSTEM_EVENT_DATA_READY = 1,     ///< New data are available
    NT_SYSTEM_EVENT_DATA_OVERFLOW = 2,  ///< Measured data overflow (HW out of range)
};

/* forward declaration */
struct nt_module;
struct nt_control;

/** System event callback function pointer type.
 * \param event  Event type \ref nt_system_event that caused the callback function call.
 * \return      None.
 */
typedef void (* nt_system_callback)(uint32_t event);

/** Error callback function pointer type.
 * \param file  The name of the file where the error occurs.
 * \param line  The line index in the file where the error occurs.
 * \return      None.
 */
typedef void (* nt_error_callback)(char *file_name, uint32_t line);

/**
 * The main structure representing the NXP Touch library;
 * The structure contains pointer lists referring to all other objects used in the
 * application, such as \ref electrodes, \ref detectors, \ref modules, and \ref controls.
 *
 * The nt_system structure and all referred structures are allocated and initialized by
 * the user code, in order to define the library configuration. This configuration affects
 * all library layers from the low-level electrode parameters (for example hardware pins and channels)
 * up to the high-level control parameters (for example slider range or keypad multiplexing).
 *
 * Just like with all other structure types, it is up to the user, whether the instance of
 * this structure is allocated statically in the compile-time, or dynamically. Examples
 * provided with the NXP Touch library show the static allocation and initialization
 * of the nt_system, along with all other related data structures.
 *
 * This structure can be allocated in ROM.
 */
struct nt_system {
    const struct nt_control * const * controls; /**< A pointer to the list of controls. Can't be NULL. */
    const struct nt_module  * const * modules;  /**< A pointer to the list of modules. Can't be NULL. */
    uint16_t                time_period;        /**< Defined time period (triggering period). Can't be 0.*/
    uint16_t                init_time;          /**< Initialization time for the system. */
};

/**
 * \defgroup system_api API Functions
 * \ingroup system
 * General Function definition of the system.
 *
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif   
  
/**
 * \brief Register the system callback function.
 * \param callback Pointer to the callback function, which will receive the system event notifications.
 * \return none
 * This is an example of installing and using the parameters of the FT library system events handler:
 * \code
 *  static void my_nt_system_callback(uint32_t event);
 *  
 *  // To catch the system events, install the system handler
 *  nt_system_register_callback(my_nt_system_callback)
 *
 *  // The FT system events handling routine
 *  static void my_nt_system_callback(uint32_t event)
 *  {
 *    if(event == NT_SYSTEM_EVENT_OVERRUN)
 *    {  
 *      printf("\nThe measured data has been overrun. Call more frequently nt_task();");
 *    }
 *    else if(event == NT_SYSTEM_EVENT_DATA_READY)
 *    {  
 *      printf("\nThere is new data in the FT library.);
 *    }
 *  }
 *
 * \endcode
 */
void nt_system_register_callback(nt_system_callback callback);

/**
 * \brief Register the system error callback function.
 * \param callback Pointer to the callback function, which will receive the error event notifications.
 * \return none
 * After this callback finishes, the driver falls to a never ending loop.
 * This is an example of installing and using the parameters of the FT library error handler:
 * \code
 *  static void my_nt_error_callback(char *file_name, uint32_t line);
 *  
 *  // For library debugging only, install the error handler
 *  nt_error_register_callback(my_nt_error_callback)
 *
 *  // The FT error-handling routine
 *  static void my_nt_error_callback(char *file_name, uint32_t line)
 *  {
 *    printf("\nError occured in the FT library. File: %s, Line: %d.\n", file_name, line);
 *  }
 *
 * \endcode
 */
void nt_error_register_callback(nt_error_callback callback);
/**
 * \brief Returns the system time counter.
 * \return Time counter value.
 * This is an example of getting the current time of the FT library:
 * \code
 *  // Printing the current NXP Touch library time
 *  printf("The current FT library time is: &d ms since start.\n", nt_system_get_time_counter());
 *
 * \endcode
 */
uint32_t nt_system_get_time_counter(void);

/**
 * \brief Returns the system time counter offset.
 * \return Time counter offset value.
 * This is an example of getting the current time of the FT library:
 * \code
 *  // Printing the current NXP Touch library time offset
 *  printf("The FT library time offset is: &d ms since start.\n", nt_system_get_time_offset());
 *
 * \endcode
 */
uint32_t nt_system_get_time_offset(uint32_t event_stamp);

/**
 * \brief Returns the free memory size in the FT memory pool
 * \return size of unused memory in the FT memory pool
 *
 * This can be used in debugging of the driver to specify the exact size of the 
 * NXP Touch memory pool needed.
 * This is an example of initializing the FT library and checking the final size:
 * \code
 *  uint8_t nt_memory_pool[512];
 *  
 *  if(nt_init(&my_nt_system_params, nt_memory_pool, sizeof(nt_memory_pool)) == NT_FAILURE)
 *  {
 *    printf("Initialization of the FT failed. There may be a problem with the memory size,
 *    or invalid parameters in the componented parameter structures.");
 *  }
 *  // The FT is successfuly initialized
 *
 *  printf("The unused memory size is: &d Bytes. The memory pool can be reduced 
 *  by this size.", nt_mem_get_free_size());
 *
 * \endcode
 */
uint32_t nt_mem_get_free_size(void);

#ifdef __cplusplus
}
#endif

/** \} */ // end of system_api group
/** \} */ // end of system group

#endif
