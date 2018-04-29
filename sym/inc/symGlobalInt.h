/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __SYM_GLOBAL_INT_H__
#define __SYM_GLOBAL_INT_H__

#define SYM_CMP_EQUAL 0x9999
#define SYM_CMP_NOT_EQUAL 0x6666

#define SYM_GLOBAL_WALLET_STATE_INITIALIZATION (0x9999)
#define SYM_GLOBAL_WALLET_STATE_OPERATIONAL (0x6666)

#define SYM_GLOBAL_MINIMAL_TWEAK_LENGTH (1)
#define SYM_GLOBAL_MAXIMAL_TWEAK_LENGTH (10)

#define SYM_GLOBAL_ENCODED_FULL_POINT_SIZE (65)

#define ETH_GLOBAL_SIGNATURE_SIZE (32 + 32)

#endif /* __SYM_GLOBAL_INT_H__ */
