/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_KEYSAFE_H__
#define __MK82_KEYSAFE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MK82_KEYSAFE_SF_KEK_ID (0x9999)
#define MK82_KEYSAFE_OTP_KEK_ID (0x6666)
#define MK82_KEYSAFE_CCR_KEK_ID (0xCCCC)
#define MK82_KEYSAFE_OPGP_KEK_ID (0x3333)

#define MK82_KEYSAFE_KEK_LENGTH (16)
#define MK82_KEYSAFE_NONCE_LENGTH (12)
#define MK82_KEYSAFE_TAG_LENGTH (16)

    MK82_MAKE_PACKED(typedef struct)
    {
        uint16_t dataInitialized; /* MK82_FALSE */
        uint8_t sfKek[MK82_KEYSAFE_KEK_LENGTH];
        uint8_t sfKekNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t sfKekTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t otpKek[MK82_KEYSAFE_KEK_LENGTH];
        uint8_t otpKekNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t otpKekTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t ccrKek[MK82_KEYSAFE_KEK_LENGTH];
        uint8_t ccrKekNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t ccrKekTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t opgpKek[MK82_KEYSAFE_KEK_LENGTH];
        uint8_t opgpKekNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t opgpKekTag[MK82_KEYSAFE_TAG_LENGTH];
    }
    KEYSAFE_NVM_DATA;

    void mk82KeysafeInit(void);
    void mk82KeysafeWrapKey(uint16_t kekID, uint8_t* key, uint32_t keyLength, uint8_t* encryptedKey, uint8_t* appData,
                            uint32_t appDataLength, uint8_t* nonce, uint8_t* tag);
    uint16_t mk82KeysafeUnwrapKey(uint16_t kekID, uint8_t* encryptedKey, uint32_t keyLength, uint8_t* key,
                                  uint8_t* appData, uint32_t appDataLength, uint8_t* nonce, uint8_t* tag);
    void mk82KeysafeGetSfAttestationKey(uint8_t* key);
    void mk82KeysafeWipeout(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_KEYSAFE_H__ */
