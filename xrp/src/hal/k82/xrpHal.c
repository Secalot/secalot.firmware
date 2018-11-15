/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <xrpGlobal.h>
#include <xrpGlobalInt.h>
#include <xrpHal.h>
#include <hal/k82/xrpHalInt.h>

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82KeySafe.h"
#include "mk82Fs.h"

#ifdef USE_BUTTON
#include <mk82Button.h>
#endif
#ifdef USE_TOUCH
#include <mk82Touch.h>
#endif
#include "mk82SecApdu.h"
#include "mk82As.h"

#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/md.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

static uint16_t xrpHalIsPrivateKeyInitialized(void);
static void xrpHalGetPrivateKey(uint8_t* privateKey);
static void xrpHalButtonPressedCallback(void);

static mbedtls_sha512_context xrpHalHashContext;

static uint16_t xrpHalButtonPressed;

static uint16_t xrpHalConfirmationTimeOngoing;
static uint64_t xrpHAlInitialConfirmationTime;

void xrpHalInit(void) {

	xrpHalButtonPressed = XRP_FALSE;
	xrpHalConfirmationTimeOngoing = XRP_FALSE;
	xrpHAlInitialConfirmationTime = 0;
    
    mbedtls_sha512_init(&xrpHalHashContext);
}

void xrpHalDeinit(void) {}

uint16_t xrpHalMemCmp(uint8_t* array1, uint8_t* array2, uint16_t length)
{
    uint16_t comparisonResult;

    comparisonResult = mk82SystemMemCmp(array1, array2, length);

    if (comparisonResult != MK82_TRUE)
    {
        return XRP_CMP_NOT_EQUAL;
    }
    else
    {
        return XRP_CMP_EQUAL;
    }
}

void xrpHalMemSet(uint8_t* dst, uint8_t value, uint16_t length) { mk82SystemMemSet(dst, value, length); }

void xrpHalMemCpy(uint8_t* dst, uint8_t* src, uint16_t length) { mk82SystemMemCpy(dst, src, length); }

void xrpHalGetPinErrorCounter(uint8_t* errorCounter)
{
    if (errorCounter == NULL)
    {
        xrpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_COUNTERS, offsetof(XRP_HAL_NVM_COUNTERS, pinErrorCounter), errorCounter,
                   sizeof(uint8_t));
}

void xrpHalSetPinErrorCounter(uint8_t errorCounter)
{
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_COUNTERS, offsetof(XRP_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_COUNTERS);
}

void xrpHalGetPinHash(uint8_t* pinHash)
{
    if (pinHash == NULL)
    {
        xrpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, pinHash), pinHash, XRP_GLOBAL_PIN_HASH_LENGTH);
}

void xrpHalComputePinHash(uint8_t* pin, uint32_t pinLength, uint8_t* pinHash)
{
    if ((pin == NULL) || (pinHash == NULL))
    {
        xrpHalFatalError();
    }

    mbedtls_sha256(pin, pinLength, pinHash, 0);
}

void xrpHalSetPrivateKey(uint8_t* privateKey)
{
    uint8_t encryptedPrivateKey[XRP_GLOBAL_PRIVATE_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t trueFalse;

    if (privateKey == NULL)
    {
        xrpHalFatalError();
    }

    mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, privateKey, XRP_GLOBAL_PRIVATE_KEY_SIZE, encryptedPrivateKey, NULL, 0, nonce, tag);

    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKey), encryptedPrivateKey,
                    XRP_GLOBAL_PRIVATE_KEY_SIZE);
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyNonce), nonce,
                    MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);
    trueFalse = XRP_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS,privateKeyInitialized), (uint8_t*)&trueFalse,
                    sizeof(trueFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_KEYS);
}

void xrpHalWriteSetupInfoAndFinalizeSetup(uint8_t* pinHash)
{
    uint8_t errorCounter = XRP_GLOBAL_PIN_INITIAL_ERROR_COUNTER_VALUE;
    uint16_t walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;

    if (pinHash == NULL)
    {
        xrpHalFatalError();
    }

    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_COUNTERS, offsetof(XRP_HAL_NVM_COUNTERS, pinErrorCounter), &errorCounter,
                    sizeof(uint8_t));
    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_COUNTERS);

    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, pinHash), pinHash, XRP_GLOBAL_PIN_HASH_LENGTH);
    walletState = XRP_GLOBAL_WALLET_STATE_OPERATIONAL;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));

    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_DATA);
}

uint16_t xrpHalGetWalletState(void)
{
    uint16_t walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                   sizeof(walletState));

    return walletState;
}

uint16_t xrpHalIsWipeoutInProgress(void)
{
    uint16_t wipeoutInProgress = XRP_TRUE;

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, wipeoutInProgress),
                   (uint8_t*)&wipeoutInProgress, sizeof(wipeoutInProgress));

    return wipeoutInProgress;
}

void xrpHalGetRandom(uint8_t* buffer, uint32_t length)
{
    if (buffer == NULL)
    {
        xrpHalFatalError();
    }

    mk82SystemGetRandom(buffer, length);
}

static uint16_t xrpHalIsPrivateKeyInitialized(void)
{
    uint16_t privateKeyInitialized = XRP_FALSE;
    uint16_t retVal = XRP_FALSE;

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyInitialized),
                   (uint8_t*)&privateKeyInitialized, sizeof(privateKeyInitialized));

    if (privateKeyInitialized != XRP_TRUE)
    {
        retVal = XRP_FALSE;
    }
    else
    {
        retVal = XRP_TRUE;
    }

    return retVal;
}

static void xrpHalGetPrivateKey(uint8_t* privateKey)
{
    uint8_t encryptedPrivateKey[XRP_GLOBAL_PRIVATE_KEY_SIZE];
    uint8_t nonce[MK82_KEYSAFE_NONCE_LENGTH];
    uint8_t tag[MK82_KEYSAFE_TAG_LENGTH];
    uint16_t privateKeyInitialized = XRP_FALSE;

    privateKeyInitialized = xrpHalIsPrivateKeyInitialized();

    if (privateKeyInitialized != XRP_TRUE)
    {
        xrpHalFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKey), encryptedPrivateKey,
                   XRP_GLOBAL_PRIVATE_KEY_SIZE);
    mk82FsReadFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyNonce), nonce,
                   MK82_KEYSAFE_NONCE_LENGTH);
    mk82FsReadFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyTag), tag, MK82_KEYSAFE_TAG_LENGTH);

    mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedPrivateKey, XRP_GLOBAL_PRIVATE_KEY_SIZE, privateKey, NULL, 0,
                         nonce, tag);
}

void xrpHalDerivePrivateKey(uint8_t* secret, uint8_t* privateKey)
{
	uint8_t publicKey[XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE];
	uint8_t hash[XRP_GLOBAL_SHA512_SIZE];

	int tlsCalleeRetVal = -1;
	mbedtls_sha512_context hashContext;
	uint32_t counter = 0;
    mbedtls_ecp_group ecpGroup;
    mbedtls_ecp_point ecpPoint;
	mbedtls_mpi scalar;
	mbedtls_mpi scalar2;
	mbedtls_mpi mpiZero;
	uint8_t zero = 0;
	uint32_t pointLength;

	if( (secret == NULL) || (privateKey == NULL) )
	{
		xrpHalFatalError();
	}

	mbedtls_sha512_init(&hashContext);

    mbedtls_ecp_group_init(&ecpGroup);
    mbedtls_ecp_point_init(&ecpPoint);
	mbedtls_mpi_init(&scalar);
	mbedtls_mpi_init(&scalar2);
	mbedtls_mpi_init(&mpiZero);

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecpGroup, MBEDTLS_ECP_DP_SECP256K1);

    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&mpiZero, &zero, sizeof(zero));
    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

    counter = 0;

    while(true)
    {
    	uint8_t counterAsAnArray[4];

    	counterAsAnArray[0] = (uint8_t)(counter>>24);
    	counterAsAnArray[1] = (uint8_t)(counter>>16);
    	counterAsAnArray[2] = (uint8_t)(counter>>8);
    	counterAsAnArray[3] = (uint8_t)(counter);

    	mbedtls_sha512_starts(&hashContext, 0);
    	mbedtls_sha512_update(&hashContext, secret, XRP_GLOBAL_SECRET_SIZE);
    	mbedtls_sha512_update(&hashContext, counterAsAnArray, sizeof(counterAsAnArray));
    	mbedtls_sha512_finish(&hashContext, hash);

    	tlsCalleeRetVal = mbedtls_mpi_read_binary(&scalar, hash, XRP_GLOBAL_PRIVATE_KEY_SIZE);
        if (tlsCalleeRetVal != 0)
        {
        	xrpHalFatalError();
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&scalar, &mpiZero);

        if (tlsCalleeRetVal == 0)
        {
        	counter++;
        	continue;
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&scalar, &(ecpGroup.N));

        if (tlsCalleeRetVal != -1)
        {
        	counter++;
        	continue;
        }

        break;
    }

    tlsCalleeRetVal = mbedtls_ecp_mul(&ecpGroup, &ecpPoint, &scalar, &(ecpGroup.G), mk82SystemGetRandomForTLS, NULL);
    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

	pointLength = XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE;

	tlsCalleeRetVal =
		mbedtls_ecp_point_write_binary(&ecpGroup, &ecpPoint, MBEDTLS_ECP_PF_COMPRESSED, (size_t*)&pointLength,
				publicKey, XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE);
    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

	if (pointLength != XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE)
	{
		xrpHalFatalError();
	}

    counter = 0;

    while(true)
    {
    	uint8_t counterAsAnArray[4];
    	uint8_t indexNumber[4];

    	indexNumber[0] = 0x00;
    	indexNumber[1] = 0x00;
    	indexNumber[2] = 0x00;
    	indexNumber[3] = 0x00;

    	counterAsAnArray[0] = (uint8_t)(counter>>24);
    	counterAsAnArray[1] = (uint8_t)(counter>>16);
    	counterAsAnArray[2] = (uint8_t)(counter>>8);
    	counterAsAnArray[3] = (uint8_t)(counter);

    	mbedtls_sha512_starts(&hashContext, 0);
    	mbedtls_sha512_update(&hashContext, publicKey, XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE);
    	mbedtls_sha512_update(&hashContext, indexNumber, sizeof(indexNumber));
    	mbedtls_sha512_update(&hashContext, counterAsAnArray, sizeof(counterAsAnArray));
    	mbedtls_sha512_finish(&hashContext, hash);

    	tlsCalleeRetVal = mbedtls_mpi_read_binary(&scalar2, hash, XRP_GLOBAL_PRIVATE_KEY_SIZE);
        if (tlsCalleeRetVal != 0)
        {
        	xrpHalFatalError();
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&scalar2, &mpiZero);

        if (tlsCalleeRetVal == 0)
        {
        	counter++;
        	continue;
        }

        tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&scalar2, &(ecpGroup.N));

        if (tlsCalleeRetVal != -1)
        {
        	counter++;
        	continue;
        }

        break;
    }

    tlsCalleeRetVal = mbedtls_mpi_add_abs(&scalar, &scalar, &scalar2);
    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }
    tlsCalleeRetVal = mbedtls_mpi_mod_mpi(&scalar, &scalar, &(ecpGroup.N));
    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_write_binary(&scalar, privateKey,
                                            XRP_GLOBAL_PRIVATE_KEY_SIZE);
    if (tlsCalleeRetVal != 0)
    {
        mk82SystemFatalError();
    }


	END:
		mbedtls_ecp_group_free(&ecpGroup);
		mbedtls_ecp_point_free(&ecpPoint);
	    mbedtls_mpi_free(&scalar);
	    mbedtls_mpi_free(&scalar2);
	    mbedtls_mpi_free(&mpiZero);
}

void xrpHalDerivePublicKey(uint8_t* publicKey)
{
    int calleeRetVal;
    uint8_t privateKey[XRP_GLOBAL_PRIVATE_KEY_SIZE];
    uint32_t pointLength;
    mbedtls_ecp_group ecpGroup;
    mbedtls_ecp_point ecpPoint;
    mbedtls_mpi multiplier;

    mbedtls_ecp_group_init(&ecpGroup);
    mbedtls_ecp_point_init(&ecpPoint);
    mbedtls_mpi_init(&multiplier);

    calleeRetVal = mbedtls_ecp_group_load(&ecpGroup, MBEDTLS_ECP_DP_SECP256K1);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    xrpHalGetPrivateKey(privateKey);

    calleeRetVal = mbedtls_mpi_read_binary(&multiplier, privateKey, XRP_GLOBAL_PRIVATE_KEY_SIZE);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    calleeRetVal = mbedtls_ecp_mul(&ecpGroup, &ecpPoint, &multiplier, &(ecpGroup.G), mk82SystemGetRandomForTLS, NULL);
    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

	pointLength = XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE;

	calleeRetVal =
		mbedtls_ecp_point_write_binary(&ecpGroup, &ecpPoint, MBEDTLS_ECP_PF_COMPRESSED, (size_t*)&pointLength,
				publicKey, XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE);

	if (pointLength != XRP_GLOBAL_ENCODED_COMPRESSED_POINT_SIZE)
	{
		mk82SystemFatalError();
	}


    if (calleeRetVal != 0)
    {
        mk82SystemFatalError();
    }

    mk82SystemMemSet(privateKey, 0x00, XRP_GLOBAL_PRIVATE_KEY_SIZE);
    mbedtls_ecp_group_free(&ecpGroup);
    mbedtls_ecp_point_free(&ecpPoint);
    mbedtls_mpi_free(&multiplier);
}

void xrpHalHashInit(void)
{
    mbedtls_sha512_starts(&xrpHalHashContext, 0);
}

void xrpHalHashUpdate(uint8_t* data, uint32_t dataLength)
{
    if (data == NULL)
    {
        xrpHalFatalError();
    }

    mbedtls_sha512_update(&xrpHalHashContext, data, dataLength);
}

void xrpHalHashFinal(uint8_t* hash)
{
    uint8_t hashInternal[XRP_GLOBAL_SHA512_SIZE];

    if (hash == NULL)
    {
        xrpHalFatalError();
    }

    mbedtls_sha512_finish(&xrpHalHashContext, hashInternal);

    mk82SystemMemCpy(hash, hashInternal, XRP_GLOBAL_SHA256_SIZE);
}

void xrpHalSignHash(uint8_t* hash, uint8_t* signature, uint16_t* signatureLength)
{
    uint8_t privateKey[XRP_GLOBAL_PRIVATE_KEY_SIZE];
    int tlsCalleeRetVal = -1;
    mbedtls_ecdsa_context ecsdaContext;
    uint8_t signatureInternal[MBEDTLS_ECDSA_MAX_LEN];
    uint32_t signatureLengthInternal;
    mbedtls_mpi r;
    mbedtls_mpi s;
    mbedtls_mpi nDivBy2;
    uint8_t nDivBy2Array[] = {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                              0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x5D, 0x57, 0x6E, 0x73, 0x57, 0xA4,
                              0x50, 0x1D, 0xDF, 0xE9, 0x2F, 0x46, 0x68, 0x1B, 0x20, 0xA0};
    int rYSign;
   

    if ( (hash == NULL) || (signature == NULL) || (signatureLength == NULL) )
    {
        xrpHalFatalError();
    }

    mbedtls_ecdsa_init(&ecsdaContext);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    mbedtls_mpi_init(&nDivBy2);

    xrpHalGetPrivateKey(privateKey);

    tlsCalleeRetVal = mbedtls_ecp_group_load(&ecsdaContext.grp, MBEDTLS_ECP_DP_SECP256K1);

    if (tlsCalleeRetVal != 0)
    {
        xrpHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_read_binary(&ecsdaContext.d, privateKey, XRP_GLOBAL_PRIVATE_KEY_SIZE);

    if (tlsCalleeRetVal != 0)
    {
        xrpHalFatalError();
    }
    
    tlsCalleeRetVal = mbedtls_ecdsa_sign(&ecsdaContext.grp, &r, &s, &ecsdaContext.d, hash,
    		XRP_GLOBAL_SHA256_SIZE, &rYSign, mk82SystemGetRandomForTLS, NULL);

    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }


    tlsCalleeRetVal = mbedtls_mpi_read_binary(&nDivBy2, nDivBy2Array, sizeof(nDivBy2Array));

    if (tlsCalleeRetVal != 0)
    {
    	xrpHalFatalError();
    }

    tlsCalleeRetVal = mbedtls_mpi_cmp_abs(&s, &nDivBy2);

    if (tlsCalleeRetVal == 1)
    {
        tlsCalleeRetVal = mbedtls_mpi_sub_abs(&s, &(ecsdaContext.grp.N), &s);

        if (tlsCalleeRetVal != 0)
        {
        	xrpHalFatalError();
        }
    }

    tlsCalleeRetVal = ecdsa_signature_to_asn1(&r, &s, signatureInternal, (size_t*)&signatureLengthInternal );

	if (tlsCalleeRetVal != 0)
	{
		xrpHalFatalError();
	}

    if (signatureLengthInternal > MBEDTLS_ECDSA_MAX_LEN)
    {
    	xrpHalFatalError();
    }

    *signatureLength = (uint16_t)signatureLengthInternal;
    mk82SystemMemCpy(signature, signatureInternal, signatureLengthInternal);

    mbedtls_ecdsa_free(&ecsdaContext);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&nDivBy2);
    mk82SystemMemSet(privateKey, 0x00, XRP_GLOBAL_PRIVATE_KEY_SIZE);
}

static void xrpHalButtonPressedCallback(void) { xrpHalButtonPressed = XRP_TRUE; }

void xrpHalWaitForComfirmation(uint16_t* confirmed)
{
    uint64_t currentTime;
    uint16_t currentDataType;

    xrpHalButtonPressed = XRP_FALSE;

    currentDataType = mk82SecApduGetPrimaryDataType();

    xrpHalConfirmationTimeOngoing = XRP_TRUE;

#ifdef USE_BUTTON
    mk82ButtonRegisterButtonDoubleClickedCallback(xrpHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchRegisterButton2PressedCallback(xrpHalButtonPressedCallback);
#endif

#ifdef USE_TOUCH
    mk82TouchEnable();
#endif

    mk82SystemTickerGetMsPassed(&xrpHAlInitialConfirmationTime);

    while (1)
    {
#ifdef USE_TOUCH
        mk82TouchTask();
#endif

        if (xrpHalButtonPressed == XRP_TRUE)
        {
            *confirmed = XRP_TRUE;
            break;
        }

        mk82SystemTickerGetMsPassed(&currentTime);

        if ((currentTime - xrpHAlInitialConfirmationTime) > XRP_HAL_CONFIRMATION_TIMEOUT_IN_MS)
        {
            *confirmed = XRP_FALSE;
            break;
        }

        if(currentDataType == MK82_GLOBAL_DATATYPE_U2F_MESSAGE)
        {
        	mk82SecApduProcessCommandIfAvailable(MK82_GLOBAL_PROCESS_CCID_APDU, MK82_AS_ALLOW_XRP_COMMANDS | MK82_AS_ALLOW_SSL_COMMANDS);
        }
    }

    xrpHalConfirmationTimeOngoing = XRP_FALSE;

#ifdef USE_TOUCH
    mk82TouchDisable();
#endif

#ifdef USE_BUTTON
    mk82ButtonDeregisterButtonDoubleClickedCallback();
#endif

#ifdef USE_TOUCH
    mk82TouchDeregisterButton2PressedCallback();
#endif
}

uint64_t xrpHalGetRemainingConfirmationTime(void)
{
	uint64_t currentTime;

	if(xrpHalConfirmationTimeOngoing != XRP_TRUE)
	{
		xrpHalFatalError();
	}

	mk82SystemTickerGetMsPassed(&currentTime);

	return (XRP_HAL_CONFIRMATION_TIMEOUT_IN_MS - (currentTime - xrpHAlInitialConfirmationTime));
}


void xrpHalWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[XRP_HAL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;
    uint16_t walletState;
    XRP_HAL_NVM_COUNTERS counters = {XRP_GLOBAL_PIN_BLOCKED_ERROR_COUNTER_VALUE};

    trueOrFalse = XRP_TRUE;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_DATA);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(XRP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(XRP_HAL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(XRP_HAL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = XRP_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_KEYS, offsetof(XRP_HAL_NVM_KEYS, privateKeyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_KEYS);

    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_COUNTERS, 0, (uint8_t*)&counters, sizeof(counters));
    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_COUNTERS);

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));
    bytesWritten = 0;

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(XRP_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(XRP_HAL_NVM_DATA))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, bytesWritten, wipeoutBuffer,
                        (sizeof(XRP_HAL_NVM_DATA) - bytesWritten));
    }

    walletState = XRP_GLOBAL_WALLET_STATE_INITIALIZATION;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, walletState), (uint8_t*)&walletState,
                    sizeof(walletState));
    trueOrFalse = XRP_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_XRP_DATA, offsetof(XRP_HAL_NVM_DATA, wipeoutInProgress), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_XRP_DATA);
}

void xrpHalFatalError(void) { mk82SystemFatalError(); }

#pragma GCC pop_options
