/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_BIP32_H__
#define __MK82_BIP32_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MK82_BIP32_NO_ERROR 0x9999
#define MK82_BIP32_GENERAL_ERROR 0x6666
#define MK82_BIP32_KEY_DERIVATION_ERROR 0xCCCC

#define MK82_BIP32_PRIVATE_KEY_SIZE (32)
#define MK82_BIP32_ENCODED_COMPRESSED_POINT_SIZE (33)
#define MK82_BIP32_ENCODED_FULL_POINT_SIZE (65)
#define MK82_BIP32_MASTER_KEY_SIZE (0x40)
#define MK82_BIP32_SHA512_SIZE (64)
#define MK82_BIP32_CHAIN_CODE_SIZE (32)

#define MK82_BIP32_MASTER_KEY_PRIVATE_KEY_OFFSET (0x00)
#define MK82_BIP32_MASTER_KEY_CHAIN_CODE_OFFSET (0x20)

#define MK82_BIP32_MAXIMAL_NUMBER_OF_KEY_DERIVATIONS (10)
#define MK82_BIP32_MINIMAL_NUMBER_OF_KEY_DERIVATIONS (1)

#define MK82_BIP32_HARDENED_KEY_MASK (0x80000000)

#define MK82_BIP32_SHA512_LEFT_PART_OFFSET (0)
#define MK82_BIP32_SHA512_RIGHT_PART_OFFSET (32)

    typedef void (*MK82_BIP32_GET_MASTER_KEY_CALLBACK)(uint8_t* masterKey);

    uint16_t mk82Bip32DerivePrivateKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations,
                                       uint8_t* privateKey, uint8_t* chainCode,
                                       MK82_BIP32_GET_MASTER_KEY_CALLBACK callback);

    uint16_t mk82Bip32DerivePublicKey(uint32_t* derivationIndexes, uint32_t numberOfKeyDerivations,
                                      uint8_t* fullPublicKey, uint8_t* compressedPublicKey, uint8_t* chainCode,
                                      uint16_t computeFull, uint16_t computeCompressed,
                                      MK82_BIP32_GET_MASTER_KEY_CALLBACK callback);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_BIP32_H__ */
