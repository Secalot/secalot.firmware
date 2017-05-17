/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SF_CCID_GLOBAL_INT_H__
#define __SF_CCID_GLOBAL_INT_H__

#ifdef __CC_ARM
#define CCID_MAKE_PACKED(x) __packed x
#elif __GNUC__
#define CCID_MAKE_PACKED(x) x __attribute__((packed))
#else
#error Unsupported platform
#endif

#ifndef NULL
#define NULL (0)
#endif

#endif /* __SF_CCID_GLOBAL_INT_H__ */
