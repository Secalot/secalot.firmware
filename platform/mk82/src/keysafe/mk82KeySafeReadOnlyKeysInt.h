/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_KEYSAFE_READONLYKEYS_INT_H__
#define __MK82_KEYSAFE_READONLYKEYS_INT_H__

#define MK82_KEYSAFE_MASTERKEY_LENGTH (0x10)
#define MK82_KEYSAFE_SF_ATTESTATION_PRIVATE_KEY_LENGTH (0x20)

typedef struct
{
    uint8_t masterKey[MK82_KEYSAFE_MASTERKEY_LENGTH];
    uint8_t sfAttestationPrivateKey[MK82_KEYSAFE_SF_ATTESTATION_PRIVATE_KEY_LENGTH];
} KEYSAFE_READONLY_KEYS;

#endif /* __MK82_KEYSAFE_READONLYKEYS_INT_H__ */
