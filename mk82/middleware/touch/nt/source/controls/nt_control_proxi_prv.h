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
#ifndef NT_PROXI_PRV_H
#define NT_PROXI_PRV_H

/**
 * \defgroup proxi_private Proxi Control
 * \ingroup controls_private
 *
 * Proxi control enables the detection of a finger or object presence in the 
 * near field of the electrode, so that the approaching finger/hand can be detected
 * without direct electrode touch needed. Moreover the proximity position and 
 * direction of the movement can be evaluated.
 * it is represented by the nt_control structure.
 *
 * The Proxi control uses a single or a set of discrete electrodes to enable the calculation of finger
 * proximity position within a near field area.  If more electrodes are enabled for Proximity control,
 * the proxi detection algorithm localizes the electrode with the highest delta signal 
 * to detect the single active electrode from the group. Signals from this active electrode
 * is then used for evaluation of the proximity position, movement and its direction.
 *
 * The Proxi control provides Position, Direction, and Displacement values. It
 * is able to generate event callbacks when finger Movement, Initial-touch, or Release
 * is detected.
 *
 * The image below show proxi electrode functionality.
 * \image html proxi.png "Proxi Electrode"
 * \image latex proxi.png "Proxi Electrode"
 *
 * \{
 */

#include "nt_controls.h"
#include "nt_types.h"
/* forward declaration */
struct nt_control_proxi;

/** 
 * Proxi Proximity flags. 
 */
enum nt_control_prox_flags 
{
    NT_PROXI_DIRECTION_FLAG        = 1 << NT_FLAGS_SPECIFIC_SHIFT(0), /**< Proxi direction flag. */
    NT_PROXI_MOVEMENT_FLAG         = 1 << NT_FLAGS_SPECIFIC_SHIFT(1), /**< Proxi movement flag.*/
    NT_PROXI_TOUCH_FLAG            = 1 << NT_FLAGS_SPECIFIC_SHIFT(2), /**< Proxi touch flag. */
    NT_PROXI_RELEASE_FLAG          = 1 << NT_FLAGS_SPECIFIC_SHIFT(3), /**< Proxi release flag. */
};


/**
 *  The Proxi RAM structure used to store volatile parameters of the control.
 *
 *  You must allocate this structure and put a pointer into the nt_control_proxi
 *  structure when it is being registered in the system.
 */
struct nt_control_proxi_data {
    nt_control_proxi_callback   callback;     /**< Proxi callback handler. */
    int32_t                     proximity;    /**< Proximity position. */
    uint32_t                    index;        /**< Proximity active key index */
};

/**
 *  Key Proximity help structure to handle temporary values
 */
struct nt_control_proxi_temp_data {
    uint32_t    active_el_ix;  /**< Index of electrode with max delta */
    uint32_t    max_delta;     /**< max delta val. (signal - baseline). */
    uint32_t    range;         /**< signal baseline of electrode w. max delta */    
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

/** \} end of proxi_private group */

#endif
