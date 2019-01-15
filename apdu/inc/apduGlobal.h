/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __APDU_GLOBAL_H__
#define __APDU_GLOBAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define APDU_TRUE 0x9999
#define APDU_FALSE 0x6666

#define APDU_NO_ERROR 0x9999
#define APDU_GENERAL_ERROR 0x6666

#ifdef __cplusplus
}
#endif

#endif /* __APDU_GLOBAL_H__ */
