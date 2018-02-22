/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __ETH_GLOBAL_INT_H__
#define __ETH_GLOBAL_INT_H__

#define ETH_CMP_EQUAL 0x9999
#define ETH_CMP_NOT_EQUAL 0x6666

#define ETH_GLOBAL_PIN_MINIMUM_LENGTH (0x4)
#define ETH_GLOBAL_PIN_MAXIMUM_LENGTH (0x20)

#define ETH_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE (0x00)
#define ETH_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE (0x03)

#define ETH_GLOBAL_SEED_MINIMUM_LENGTH (0x20)
#define ETH_GLOBAL_SEED_MAXIMUM_LENGTH (0x40)

#define ETH_GLOBAL_WALLET_STATE_INITIALIZATION (0x9999)
#define ETH_GLOBAL_WALLET_STATE_OPERATIONAL (0x6666)

#define ETH_GLOBAL_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS (10)
#define ETH_GLOBAL_MINIMAL_NUMBER_OF_KEY_DERIVATIONS (1)

#define ETH_GLOBAL_KECCAK_256_HASH_SIZE (32)

#define ETH_GLOBAL_SIGNATURE_SIZE (1 + 32 + 32)

#endif /* __ETH_GLOBAL_INT_H__ */
