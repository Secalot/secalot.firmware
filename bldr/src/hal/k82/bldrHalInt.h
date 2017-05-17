/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BLDR_HAL_K82_INT_H__
#define __BLDR_HAL_K82_INT_H__

#define BLDR_HAL_SHA256_LENGTH (32)
#define BLDR_HAL_R_AND_S_LENGTH (32)

#define BLDR_HAL_MK82_BOOTLOADER_ENABLED                                                               \
    {                                                                                                  \
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF \
    }

#define BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_ADDRESS (0x00)
#define BLDR_HAL_MK82_BOOTLOADER_SETTINGS_PAGE_OFFSET (0x400)

#endif /* __BLDR_HAL_K82_INT_H__ */
