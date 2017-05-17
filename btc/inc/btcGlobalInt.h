/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __BTC_GLOBAL_INT_H__
#define __BTC_GLOBAL_INT_H__

#define BTC_GLOBAL_PIN_MINIMUM_LENGTH (0x4)
#define BTC_GLOBAL_PIN_MAXIMUM_LENGTH (0x20)

#define BTC_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE (0x00)
#define BTC_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE (0x03)

#define BTC_GLOBAL_SEED_MINIMUM_LENGTH (0x20)
#define BTC_GLOBAL_SEED_MAXIMUM_LENGTH (0x40)

#define BTC_GLOBAL_WALLET_STATE_INITIALIZATION (0x9999)
#define BTC_GLOBAL_WALLET_STATE_OPERATIONAL (0x6666)

#define BTC_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS (10)
#define BTC_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS (1)

#define BTC_GLOBAL_HARDENED_KEY_MASK (0x80000000)

#define BTC_GLOBAL_MAXIMAL_SIGNATURE_LENGTH (72)

#endif /* __BTC_GLOBAL_INT_H__ */
