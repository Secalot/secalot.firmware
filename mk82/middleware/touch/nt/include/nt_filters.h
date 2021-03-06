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
#ifndef NT_FILTERS_H
#define NT_FILTERS_H

#include "nt_types.h"

/**
 * \defgroup filter Filters
 * \ingroup ntapi
 * The filters data structure that is used in the NXP Touch library.
 *
 * \{
 */

#define NT_FILTER_MOVING_AVERAGE_MAX_ORDER  15
   
/* forward declaration */

/**
 * The butterworth filter input parameters.
 *
 */
struct nt_filter_fbutt {
    int32_t cutoff; /**< The coeficient for the implemented butterworth filter polynomial. */
};

/**
 * The IIR filter input parameters.
 *
 */
struct nt_filter_iir {
    uint8_t coef1; /**< Scale of the current and previous signals. When the coef is higher, the current signal has less strength. */
};

/**
 * The DC tracker filter input parameters.
 *
 */
struct nt_filter_dctracker {
    uint8_t rate; /**< Rate of how fast the baseline is updated. The rate should be defined as a modulo of the system period. */
};

/**
 * The moving average filter input parameters.
 *
 */
struct nt_filter_moving_average {
  int32_t       n2_order;       /**< The order 2^n moving average filter */
};


/** \} */ // end of filters group

#endif
