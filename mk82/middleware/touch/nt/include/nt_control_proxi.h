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
#ifndef NT_PROXI_H
#define NT_PROXI_H

/**
 * \defgroup proxi Proxi Control
 * \ingroup controls
 *
 * Proxi implements object presence detection in the near field (approaching finger or hand),
 * it is represented by the nt_control_proxi structure.
 *
 * The Proxi Control provides the proximity Key status values and is able to generate Proxi Touch,
 * Movement, and Release events.
 *
 * The figures below show simple and grouped Proxi electrode layouts.
 * \image html proxi.png "Proxi Electrodes"
 * \image latex proxi.png "Proxi Electrodes"
 *
 * \{
 */

#include "nt_controls.h"
#include "nt_types.h"

/** Proxi event types. */
enum nt_control_proxi_event {
    NT_PROXI_MOVEMENT        = 0, /**< Release event */
    NT_PROXI_RELEASE         = 1, /**< Key-touch event */
    NT_PROXI_TOUCH           = 2, /**< Proximity movement  event */
};

/**
 * Proxi event callback function pointer type.
 */
typedef void (* nt_control_proxi_callback)(const struct nt_control *control,
                                    enum nt_control_proxi_event event,        
                                    uint32_t index, uint32_t proximity);
     
/**
 *  The main structure representing the Proxi Control.
 *
 *  An instance of this data type represents the Proxi Control. You must initialize all the members before registerring the control in
 *  the system. This structure can be allocated in ROM.
 *
 */
struct nt_control_proxi {    
    uint32_t  range;          /**< Max signal delta level for max. prox (100%) value */
    uint32_t  scale;          /**< Proxi scale (i.e. 0-100% or 0-255) value. */
    uint32_t  threshold;      /**< Proxi Touch/Release threshold value. */
    uint32_t  insensitivity;  /**< Insensitivity for the callbacks invokes when the position is changed. */
};

/** An interface structure, which contains pointers to the entry points of the Proxi
 *  algorithms. A pointer to this structure must be assigned to any
 *  instance of the nt_control_proxi, to define the control behavior. */
extern const struct nt_control_interface nt_control_proxi_interface;
/**
 * \defgroup proxi_api Proxi Control API
 * \ingroup proxi
 *
 * These functions can be used to set or get the Proxi control properties.
 *
 * A common example defition of the Proxi control for all source code examples is as follows:
 * \code
 *  // definition of electrode array used by control (more info in electrodes )
 * const struct nt_electrode  * const control_0_electrodes[] = {&electrode_0, &electrode_1,
 *   NULL};
 *
 * const struct nt_control_proxi proxi_params =
 * {
 *   .range = 21000,      
 *   .scale = 255,        
 *   .threshold = 10,     
 *   .insensitivity = 1,  
 * };
 *
 *  // Definition of proxi control
 * const struct nt_control proxi_0 =
 * {
 *   .interface = &nt_control_proxi_interface,
 *   .electrodes = control_0_electrodes,
 *   .control_params.proxi = &proxi_params,
 * };
 *
 * \endcode
 * \{
 */

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \brief Registers the Proxi event handler function.
 * \param control  Pointer to the control.
 * \param callback Adress of function to be invoked.
 * \return none
 *
 * Register the specified callback function as the KeyPad event handler.
 * If the callback parameter is NULL, the callback is disabled.
 * Example:
 * \code
 *  
 *  //Create the callback function for proxi
 *  static void my_proxi_cb(const struct nt_control *control,
 *                           enum nt_control_proxi_event event,
 *                           uint32_t index)
 *  {
 *    (void)control;
 *     char* event_names[] = 
 *     {
 *      "NT_PROXI_MOVEMENT",
 *      "NT_PROXI_RELEASE",
 *      "NT_PROXI_TOUCH",
*      };
 *
 *    printf("New proxi control event %s on key: %d.", event_names[event], index);
 *  }
 *
 *  // register the callback function for proxi 
 *  nt_control_proxi_register_touch_callback(&my_proxi_control, my_proxi_touch_cb);
 * \endcode
 */
void nt_control_proxi_register_callback(const struct nt_control *control,
                                       nt_control_proxi_callback callback);

/** \} end of proxi_api group */
/** \} end of proxi group */

#ifdef __cplusplus
}
#endif

#endif
