/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __OPGP_HAL_H__
#define __OPGP_HAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"
#include "opgpGlobal.h"

#include "mk82Global.h"
#include "mk82KeySafe.h"

#define OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH (0x04)
#define OPGP_GLOBAL_MODULUS_LENGTH (0x100)
#define OPGP_GLOBAL_PRIME_LENGTH (0x80)

    OPGP_MAKE_PACKED(typedef struct)
    {
        uint16_t keyInitialized; /* OPGP_FALSE16 */
        uint8_t e[OPGP_GLOBAL_PUBLIC_EXPONENT_LENGTH];
        uint8_t p[OPGP_GLOBAL_PRIME_LENGTH];
        uint8_t pNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t pTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t q[OPGP_GLOBAL_PRIME_LENGTH];
        uint8_t qNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t qTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t pq[OPGP_GLOBAL_PRIME_LENGTH];
        uint8_t pqNonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t pqTag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t dp1[OPGP_GLOBAL_PRIME_LENGTH];
        uint8_t dp1Nonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t dp1Tag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t dq1[OPGP_GLOBAL_PRIME_LENGTH];
        uint8_t dq1Nonce[MK82_KEYSAFE_NONCE_LENGTH];
        uint8_t dq1Tag[MK82_KEYSAFE_TAG_LENGTH];
        uint8_t n[OPGP_GLOBAL_MODULUS_LENGTH];
    }
    OPGP_HAL_KEY;

    OPGP_MAKE_PACKED(typedef struct)
    {
        uint8_t pw1ErrorCounter;   /* 3 */
        uint8_t pw3ErrorCounter;   /* 3 */
        uint8_t rcErrorCounter;    /* 0 */
        uint32_t signatureCounter; /* 0 */
    }
    OPGP_HAL_NVM_COUNTERS;

    OPGP_MAKE_PACKED(typedef struct)
    {
        uint32_t doCertificateLength;
        uint8_t doCertificate[OPGP_GLOBAL_DO_CERTIFICATE_MAX_LENGTH];
    }
    OPGP_HAL_NVM_CERTIFICATES;

    OPGP_MAKE_PACKED(typedef struct)
    {
        OPGP_HAL_KEY signatureKey;
        OPGP_HAL_KEY decryptionKey;
        OPGP_HAL_KEY authenticationKey;
    }
    OPGP_HAL_NVM_KEYS;

    OPGP_MAKE_PACKED(typedef struct)
    {
        uint32_t pw1Length;                       /* 6 */
        uint8_t pw1[OPGP_GLOBAL_PIN_HASH_LENGTH]; /* 123456 */
        uint32_t pw3Length;                       /* 8 */
        uint8_t pw3[OPGP_GLOBAL_PIN_HASH_LENGTH]; /* 12345678 */
        uint32_t rcLength;                        /* 0 */
        uint8_t rc[OPGP_GLOBAL_PIN_HASH_LENGTH];  /* -- */

        uint32_t doPrivateUse1Length;
        uint8_t doPrivateUse1[OPGP_GLOBAL_DO_PRIVATE_USE_1_MAX_LENGTH];
        uint32_t doPrivateUse2Length;
        uint8_t doPrivateUse2[OPGP_GLOBAL_DO_PRIVATE_USE_2_MAX_LENGTH];
        uint32_t doPrivateUse3Length;
        uint8_t doPrivateUse3[OPGP_GLOBAL_DO_PRIVATE_USE_3_MAX_LENGTH];
        uint32_t doPrivateUse4Length;
        uint8_t doPrivateUse4[OPGP_GLOBAL_DO_PRIVATE_USE_4_MAX_LENGTH];

        uint32_t doLoginDataLength;
        uint8_t doLoginData[OPGP_GLOBAL_DO_LOGIN_DATA_MAX_LENGTH];
        uint32_t doUrlLength;
        uint8_t doUrl[OPGP_GLOBAL_DO_URL_MAX_LENGTH];
        uint32_t doNameLength;
        uint8_t doName[OPGP_GLOBAL_DO_NAME_MAX_LENGTH];

        uint8_t doLanguagePreference[OPGP_GLOBAL_DO_LANGUAGE_PREFERENCE_MAX_LENGTH];
        uint32_t doLanguagePreferenceLength;
        uint8_t doSex[OPGP_GLOBAL_DO_SEX_LENGTH];

        uint8_t doSignatureFingerprint[OPGP_GLOBAL_DO_FINGERPRINT_LENGTH];
        uint8_t doDecryptionFingerprint[OPGP_GLOBAL_DO_FINGERPRINT_LENGTH];
        uint8_t doAuthenticationFingerprint[OPGP_GLOBAL_DO_FINGERPRINT_LENGTH];
        uint8_t do1stCaFingerprint[OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH];
        uint8_t do2ndCaFingerprint[OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH];
        uint8_t do3rdCaFingerprint[OPGP_GLOBAL_DO_CA_FINGERPRINT_LENGTH];

        uint8_t doGenerationDateTimeSignatureKey[OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH];
        uint8_t doGenerationDateTimeDecryptionKey[OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH];
        uint8_t doGenerationDateTimeAuthenticationKey[OPGP_GLOBAL_DO_GENERATION_DATE_TIME_LENGTH];

        uint8_t doPW1_81_Status; /* OPGP_CORE_PW1_81_VALID_FOR_MULTIPLE_COMMANDS */

        uint8_t cardState; /* OPGP_GLOBAL_CARD_STATE_OPERATIONAL */
    }
    OPGP_HAL_NVM_DATA;

    void opgpHalInit(void);
    void opgpHalDeinit(void);

    void opgpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length);
    void opgpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length);
    uint16_t opgpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length);
    void opgpHalWipeoutBuffer(uint8_t* buffer, uint16_t length);

    void opgpHalGetSerialNumber(uint8_t* serialNumber);

    void opgpHalGetPinErrorCounter(uint8_t pinID, uint8_t* errorCounter);
    void opgpHalSetPinErrorCounter(uint8_t pinID, uint8_t errorCounter);
    void opgpHalGetPinHashAndLength(uint8_t pinID, uint8_t* pinHash, uint32_t* pinLength);
    void opgpHalGetPinLength(uint8_t pinID, uint32_t* pinLength);
    void opgpHalSetPinHashAndLength(uint8_t pinID, uint8_t* pinHash, uint32_t pinLength);
    void opgpHalSetPW1HashAndLengthAndUnblock(uint8_t* pinHash, uint32_t pinLength);
    void opgpHalSetRCHashAndLengthAndSetErrorCounter(uint8_t* pinHash, uint32_t pinLength, uint8_t errorCounter);
    void opgpHalCalculatePinHash(uint8_t* pinValue, uint32_t pinLength, uint8_t* pinHash);
    void opgpHalGetDataObject(uint16_t tag, uint8_t* data);
    void opgpHalGetDataObjectWithLength(uint16_t tag, uint8_t* data, uint32_t* length);
    void opgpHalSetDataObject(uint16_t tag, uint8_t* data);
    void opgpHalSetDataObjectWithLength(uint16_t tag, uint8_t* data, uint32_t length);
    void opgpHalIncrementSignatureCounter(void);
    void opgpHalGetSignatureCounter(uint32_t* signatureCounter);
    void opgpHalResetSignatureCounter(void);
    uint16_t opgpHalImportKey(uint16_t keyType, uint8_t* e, uint8_t* p, uint8_t* q);
    void opgpHalIsKeyInitialized(uint16_t keyType, uint16_t* result);
    void opgpHalSign(uint8_t* dataToSign, uint32_t dataToSignLength, uint8_t* signature);
    uint16_t opgpHalDecipher(uint8_t* dataToDecipher, uint8_t* decipheredData, uint32_t* decipheredDataLength);
    void opgpHalInternalAuthenticate(uint8_t* authenticationInput, uint32_t authenticationInputLength,
                                     uint8_t* signature);
    void opgpHalGenerateKeyPair(uint16_t keyType);
    void opgpHalGetPublicKey(uint16_t keyType, uint8_t* modulus, uint8_t* publicExponent);
    void opgpHalGetRandom(uint8_t* buffer, uint32_t length);

    void opgpHalGetCardState(uint8_t* cardState);
    void opgpHalSetCardState(uint8_t cardState);
    void opgpHalWipeout(void);

    void opgpHalFatalError(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPGP_HAL_H__ */
