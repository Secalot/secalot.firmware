/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_SYSTEM_INT_H__
#define __MK82_SYSTEM_INT_H__

#ifdef __CC_ARM
#define BEGIN_UNOPTIMIZED_FUNCTION _Pragma("push") _Pragma("O0")
#define END_UNOPTIMIZED_FUNCTION _Pragma("pop")
#elif __GNUC__
#define BEGIN_UNOPTIMIZED_FUNCTION __attribute__((optimize("O0")))
#define END_UNOPTIMIZED_FUNCTION ;
#else
#error Unsupported platform
#endif

#define MK82_SYSTEM_TICKER_INTERRUPT_PERIOD_IN_MS (10000)

#endif /* __MK82_SYSTEM_INT_H__ */
