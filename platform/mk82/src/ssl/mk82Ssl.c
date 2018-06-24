/*
 * Secalot firmware.
 * Copyright (c) 2018 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82Fs.h"
#include "mk82KeySafe.h"
#include "mk82Ssl.h"
#include "mk82SslInt.h"

#include <apduGlobal.h>
#include <apduCore.h>

#include "mbedtls/ecdsa.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"



uint8_t privateKeyTemplate[] = {
		0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
		0x03, 0x01, 0x07, 0xA1, 0x44, 0x03, 0x42, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

uint8_t certificateTemplate[] = {
		0x30, 0x82, 0x01, 0x11, 0x30, 0x81, 0xB9, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30,
		0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x12, 0x31, 0x10, 0x30,
		0x0E, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x07, 0x53, 0x65, 0x63, 0x61, 0x6C, 0x6F, 0x74, 0x30,
		0x20, 0x17, 0x0D, 0x31, 0x38, 0x30, 0x36, 0x31, 0x37, 0x31, 0x32, 0x34, 0x34, 0x30, 0x30, 0x5A,
		0x18, 0x0F, 0x32, 0x30, 0x39, 0x39, 0x30, 0x36, 0x31, 0x37, 0x31, 0x32, 0x34, 0x34, 0x30, 0x30,
		0x5A, 0x30, 0x12, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x07, 0x53, 0x65,
		0x63, 0x61, 0x6C, 0x6F, 0x74, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
		0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x47, 0x00, 0x30,
		0x44, 0x02, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF

};

static void mk82SslReset(void);
static int mk82SslSend( void *ctx, const unsigned char *buf, size_t len );
static int mk82SslReceive( void *ctx, unsigned char *buf, size_t len );
static void mk82SslIsKeyInitialized(uint16_t* keyInitialized);
static void mk82SslGetPublicKey(uint8_t* publicKey);
static void mk82SslGetPrivateKey(uint8_t* privateKey);
static void mk82SllCheckPrivateKeyPresense(void);

static void mk82SslProcessGetPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void mk82SslProcessHandshake(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);
static void mk82SslProcessReset(APDU_CORE_COMMAND_APDU* commandAPDU, APDU_CORE_RESPONSE_APDU* responseAPDU);


static mbedtls_ssl_context mk82SslContext;
static mbedtls_ssl_config mk82SslConfig;
static mbedtls_x509_crt mk82SslCert;
static mbedtls_pk_context mk82SslPkContext;

static uint8_t* mk82SslReceivedData = NULL;
static uint32_t mk82SslReceivedDataLength = 0;

static uint8_t* mk82SslDataToBeSent = NULL;
static uint32_t mk82SslDataToBeSentLength = 0;
static uint32_t mk82SslDataToBeSentMaxLength = 0;

static uint16_t mk82SslHandshakePerformed = MK82_FALSE;

static void mk82SslReset(void)
{
	int ret;

	ret = mbedtls_ssl_session_reset( &mk82SslContext );

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	mbedtls_ssl_set_bio( &mk82SslContext, NULL, mk82SslSend, mk82SslReceive, NULL );

	mk82SslHandshakePerformed = MK82_FALSE;
}

static int mk82SslSend( void *ctx, const unsigned char *buf, size_t len )
{
	int retVal = MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;

	if(( mk82SslDataToBeSentLength + len) <= mk82SslDataToBeSentMaxLength)
	{
		mk82SystemMemCpy(mk82SslDataToBeSent, (uint8_t*)buf, len);

		mk82SslDataToBeSent += len;
		mk82SslDataToBeSentLength += len;

		retVal = len;
	}
	else
	{
		retVal = MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;
		goto END;
	}

	END:
	return retVal;
}

static int mk82SslReceive( void *ctx, unsigned char *buf, size_t len )
{
	int retVal = MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;

	if(mk82SslReceivedDataLength != 0)
	{
		if(len > mk82SslReceivedDataLength)
		{
			retVal = MBEDTLS_ERR_SSL_UNEXPECTED_MESSAGE;
			goto END;
		}

		mk82SystemMemCpy(buf, mk82SslReceivedData, len);

		mk82SslReceivedData += len;
		mk82SslReceivedDataLength -= len;

		retVal = len;
	}
	else
	{
		retVal = MBEDTLS_ERR_SSL_WANT_READ;
		goto END;
	}

	END:
	return retVal;
}

static void mk82SslIsKeyInitialized(uint16_t* keyInitialized)
{
    mk82FsReadFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, keyInitialized), (uint8_t*)keyInitialized,
                   sizeof(uint16_t));
}

static void mk82SslGetPublicKey(uint8_t* publicKey)
{
	uint16_t keyInitialized = MK82_FALSE;

	mk82SslIsKeyInitialized(&keyInitialized);

    if(keyInitialized != MK82_TRUE)
    {
    	mk82SystemFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, publicKey), (uint8_t*)publicKey,
    		MK82_SSL_PUBLIC_KEY_LENGTH);
}

static void mk82SslGetPrivateKey(uint8_t* privateKey)
{
    uint8_t encryptedPrivateKey[MK82_SSL_PRIVATE_KEY_LENGTH];
    uint8_t nonce[MK82_SSL_NONCE_LENGTH];
    uint8_t tag[MK82_SSL_TAG_LENGTH];
    uint16_t calleeRetVal;
    uint16_t keyInitialized = MK82_FALSE;

    mk82SslIsKeyInitialized(&keyInitialized);

    if (keyInitialized != MK82_TRUE)
    {
    	mk82SystemFatalError();
    }

    mk82FsReadFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKey), encryptedPrivateKey, sizeof(encryptedPrivateKey));
    mk82FsReadFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKeyNonce), nonce, sizeof(nonce));
    mk82FsReadFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKeyTag), tag, sizeof(tag));

    calleeRetVal = mk82KeysafeUnwrapKey(MK82_KEYSAFE_CCR_KEK_ID, encryptedPrivateKey, sizeof(encryptedPrivateKey), privateKey, NULL,
                                        0, nonce, tag);

    if (calleeRetVal != MK82_NO_ERROR)
    {
    	mk82SystemFatalError();
    }

}

static void mk82SllCheckPrivateKeyPresense(void)
{
	uint16_t keyInitialized = MK82_FALSE;
    int calleeRetVal;
    mbedtls_ecdsa_context ecsdaContext;
    uint8_t publicKey[MK82_SSL_PUBLIC_KEY_LENGTH];
    uint8_t privateKey[MK82_SSL_PRIVATE_KEY_LENGTH];
    uint8_t encryptedPrivateKey[MK82_SSL_PRIVATE_KEY_LENGTH];
    uint8_t nonce[MK82_SSL_NONCE_LENGTH];
    uint8_t tag[MK82_SSL_TAG_LENGTH];
    uint16_t trueFalse;
    uint32_t pointLength;

    mk82SslIsKeyInitialized(&keyInitialized);

    if(keyInitialized != MK82_TRUE)
    {
        mbedtls_ecdsa_init(&ecsdaContext);

        calleeRetVal = mbedtls_ecdsa_genkey(&ecsdaContext, MBEDTLS_ECP_DP_SECP256R1, mk82SystemGetRandomForTLS, NULL);

        if (calleeRetVal != 0)
        {
            mk82SystemFatalError();
        }

        calleeRetVal = mbedtls_mpi_write_binary(&ecsdaContext.d, privateKey, MK82_SSL_PRIVATE_KEY_LENGTH);

        if (calleeRetVal != 0)
        {
            mk82SystemFatalError();
        }

        mk82KeysafeWrapKey(MK82_KEYSAFE_CCR_KEK_ID, privateKey, sizeof(privateKey), encryptedPrivateKey, NULL, 0, nonce, tag);

        mk82SystemMemSet(privateKey, 0x00, sizeof(privateKey));

        calleeRetVal = mbedtls_ecp_point_write_binary(&ecsdaContext.grp, &ecsdaContext.Q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                                      (size_t*)&pointLength, publicKey, MK82_SSL_PUBLIC_KEY_LENGTH);

        if (calleeRetVal != 0)
        {
        	mk82SystemFatalError();
        }

        if (pointLength != MK82_SSL_PUBLIC_KEY_LENGTH)
        {
        	mk82SystemFatalError();
        }

        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, publicKey), publicKey, MK82_SSL_PUBLIC_KEY_LENGTH);
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKey), encryptedPrivateKey, MK82_SSL_PRIVATE_KEY_LENGTH);
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKeyNonce), nonce, MK82_SSL_NONCE_LENGTH);
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, privateKeyTag), tag, MK82_SSL_TAG_LENGTH);
        trueFalse = MK82_TRUE;
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, keyInitialized), (uint8_t*)&trueFalse,
                        sizeof(trueFalse));

        mk82FsCommitWrite(MK82_FS_FILE_ID_SSL_KEYS);


        mbedtls_ecdsa_free(&ecsdaContext);
    }
}

void mk82SslInit(void)
{
	int ret;
	uint8_t privateKeyBlob[MK82_SSL_PRIVATE_KEY_TEMPLATE_SIZE];
	uint8_t certificateBlob[MK82_SSL_CERTIFICATE_TEMPLATE_SIZE];

	mk82SllCheckPrivateKeyPresense();

	mk82SystemMemCpy(privateKeyBlob, privateKeyTemplate, MK82_SSL_PRIVATE_KEY_TEMPLATE_SIZE);
	mk82SslGetPrivateKey(privateKeyBlob+MK82_SSL_PRIVATE_KEY_TEMPLATE_PRIVATE_KEY_OFFSET);
	mk82SslGetPublicKey(privateKeyBlob+MK82_SSL_PRIVATE_KEY_TEMPLATE_PUBLIC_KEY_OFFSET);

	mk82SystemMemCpy(certificateBlob, certificateTemplate, MK82_SSL_CERTIFICATE_TEMPLATE_SIZE);
	mk82SslGetPublicKey(certificateBlob+MK82_SSL_CERTIFICATE_PUBLIC_KEY_OFFSET);

	mbedtls_ssl_init( &mk82SslContext );
	mbedtls_ssl_config_init( &mk82SslConfig );
	mbedtls_x509_crt_init( &mk82SslCert );
	mbedtls_pk_init( &mk82SslPkContext );

	ret = mbedtls_x509_crt_parse( &mk82SslCert, (const unsigned char *) certificateBlob, sizeof(certificateBlob));

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	ret =  mbedtls_pk_parse_key( &mk82SslPkContext, (const unsigned char *) privateKeyBlob, sizeof(privateKeyBlob), NULL, 0 );

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	ret = mbedtls_ssl_config_defaults( &mk82SslConfig, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT );

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	mbedtls_ssl_conf_rng( &mk82SslConfig, mk82SystemGetRandomForTLS, NULL );

	mbedtls_ssl_conf_ca_chain( &mk82SslConfig, mk82SslCert.next, NULL );

	ret = mbedtls_ssl_conf_own_cert( &mk82SslConfig, &mk82SslCert, &mk82SslPkContext );

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	ret = mbedtls_ssl_setup( &mk82SslContext, &mk82SslConfig );

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	mk82SslReset();
}


static void mk82SslProcessHandshake(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    int calleeRetVal;

    if (commandAPDU->lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != MK82_SSL_P1P2_HANDSHAKE)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if(mk82SslHandshakePerformed == MK82_TRUE)
    {
    	mk82SslReset();
    }

    mk82SslReceivedData = commandAPDU->data;
    mk82SslReceivedDataLength = commandAPDU->lc;

    mk82SslDataToBeSent = responseAPDU->data;
    mk82SslDataToBeSentLength= 0;
    mk82SslDataToBeSentMaxLength = MK82_MAX_PAYLOAD_SIZE;

    calleeRetVal = mbedtls_ssl_handshake( &mk82SslContext );

    if(calleeRetVal != 0)
    {
    	if( calleeRetVal != MBEDTLS_ERR_SSL_WANT_READ)
    	{
        	sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        	goto END;
    	}
    }
    else
    {
    	mk82SslHandshakePerformed = MK82_TRUE;
    }

	responseAPDU->dataLength = mk82SslDataToBeSentLength;


    sw = APDU_CORE_SW_NO_ERROR;

END:

	if(sw != APDU_CORE_SW_NO_ERROR)
	{
		mk82SslReset();
	}

    responseAPDU->sw = sw;
}

static void mk82SslProcessGetPublicKey(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    int calleeRetVal;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != MK82_SSL_P1P2_GET_PUBLIC_KEY)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    mk82SslGetPublicKey(responseAPDU->data);

    responseAPDU->dataLength = MK82_SSL_PUBLIC_KEY_LENGTH;

    sw = APDU_CORE_SW_NO_ERROR;

END:

    responseAPDU->sw = sw;
}

static void mk82SslProcessReset(APDU_CORE_COMMAND_APDU* commandAPDU,
                                                   APDU_CORE_RESPONSE_APDU* responseAPDU)
{
    uint16_t sw;
    int calleeRetVal;

    if (commandAPDU->lcPresent != APDU_FALSE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if (commandAPDU->p1p2 != MK82_SSL_P1P2_RESET)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    mk82SslReset();

    responseAPDU->dataLength = 0;

    sw = APDU_CORE_SW_NO_ERROR;

END:

    responseAPDU->sw = sw;
}

void mk82SslGetAID(uint8_t* aid, uint32_t* aidLength)
{
    uint8_t aidTemplate[] = MK82_SSL_AID;

    if ((aid == NULL) || (aidLength == NULL))
    {
        mk82SystemFatalError();
    }

    mk82SystemMemCpy(aid, aidTemplate, MK82_SSL_AID_LENGTH);

    *aidLength = MK82_SSL_AID_LENGTH;
}


void mk82SslProcessAPDU(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t calleeRetVal = APDU_GENERAL_ERROR;

    if ((apdu == NULL) || (apduLength == NULL))
    {
    	mk82SystemFatalError();
    }

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    calleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (calleeRetVal != APDU_NO_ERROR)
    {
        if (calleeRetVal == APDU_GENERAL_ERROR)
        {
            responseAPDU.sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else
        {
            mk82SystemFatalError();
        }
    }

    if (commandAPDU.cla != MK82_SSL_CLA)
    {
        responseAPDU.sw = APDU_CORE_SW_CLA_NOT_SUPPORTED;
        goto END;
    }

    switch (commandAPDU.ins)
    {
        case MK82_SSL_INS_HANDSHAKE:
        	mk82SslProcessHandshake(&commandAPDU, &responseAPDU);
            break;
        case MK82_SSL_INS_GET_PUBLIC_KEY:
        	mk82SslProcessGetPublicKey(&commandAPDU, &responseAPDU);
            break;
        case MK82_SSL_INS_RESET:
        	mk82SslProcessReset(&commandAPDU, &responseAPDU);
            break;
        default:
            responseAPDU.sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
            break;
    }

END:

    apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}

void mk82SslUnwrapAPDUCommand(uint8_t* apdu, uint32_t* apduLength, uint16_t* status)
{
    APDU_CORE_COMMAND_APDU commandAPDU;
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t sw;
    uint32_t totalLength;
    int calleeRetVal;

    if( (apdu == NULL) || (apduLength == 0) || (status == NULL) )
    {
    	mk82SystemFatalError();
    }

    *status = MK82_SSL_STATUS_ERROR_OCCURED;

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    calleeRetVal = apduCoreParseIncomingAPDU(apdu, *apduLength, &commandAPDU);

    if (calleeRetVal != APDU_NO_ERROR)
    {
        if (calleeRetVal == APDU_GENERAL_ERROR)
        {
            sw = APDU_CORE_SW_WRONG_LENGTH;
            goto END;
        }
        else
        {
            mk82SystemFatalError();
        }
    }

    if (commandAPDU.cla != MK82_SSL_WRAPPED_APDU_CLA)
    {
    	*status = MK82_SSL_STATUS_NOT_SSL;
    	goto END;
    }

    if (commandAPDU.lcPresent != APDU_TRUE)
    {
        sw = APDU_CORE_SW_WRONG_LENGTH;
        goto END;
    }

    if(commandAPDU.ins != MK82_SSL_INS_WRAPPED_COMMAND)
    {
        sw = APDU_CORE_SW_INS_NOT_SUPPORTED;
        goto END;
    }

    if(commandAPDU.p1p2 != MK82_SSL_P1P2_WRAPPED_COMMAND)
    {
        sw = APDU_CORE_SW_WRONG_P1P2;
        goto END;
    }

    if(mk82SslHandshakePerformed != MK82_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    mk82SslReceivedData = commandAPDU.data;
    mk82SslReceivedDataLength = commandAPDU.lc;

    mk82SslDataToBeSent = commandAPDU.data;
    mk82SslDataToBeSentLength = 0;
    mk82SslDataToBeSentMaxLength = MK82_MAX_PAYLOAD_SIZE;

    calleeRetVal = mbedtls_ssl_read( &mk82SslContext, commandAPDU.data,  MK82_MAX_PAYLOAD_SIZE);

    if(calleeRetVal < 0)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if(calleeRetVal > MK82_MAX_PAYLOAD_SIZE)
    {
    	mk82SystemFatalError();
    }

    mk82SystemMemCpy(apdu, commandAPDU.data, calleeRetVal);

    *apduLength = calleeRetVal;

    *status = MK82_SSL_STATUS_UNWRAPPED;

END:

	if(*status == MK82_SSL_STATUS_ERROR_OCCURED)
	{
		mk82SslReset();
		responseAPDU.sw = sw;
		apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
	}
}


void mk82SslWrapAPDUResponse(uint8_t* apdu, uint32_t* apduLength)
{
    APDU_CORE_RESPONSE_APDU responseAPDU;
    uint16_t sw;
    int calleeRetVal;
    int bytesProcessed = 0;
    uint16_t successfullyProcessed = MK82_FALSE;
    uint32_t apduResponseLength;

    if( (apdu == NULL) || (apduLength == 0) )
    {
    	mk82SystemFatalError();
    }

    apduResponseLength = *apduLength;

    apduCorePrepareResponseAPDUStructure(apdu, &responseAPDU);

    if(apduResponseLength < sizeof(responseAPDU.sw))
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

    if(mk82SslHandshakePerformed != MK82_TRUE)
    {
        sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
        goto END;
    }

	mk82SslReceivedData = apdu;
	mk82SslReceivedDataLength = apduResponseLength;

	mk82SslDataToBeSent = responseAPDU.data;
	mk82SslDataToBeSentLength= 0;
	mk82SslDataToBeSentMaxLength = MK82_MAX_PAYLOAD_SIZE;

	while(bytesProcessed < apduResponseLength)
	{
		calleeRetVal = mbedtls_ssl_write( &mk82SslContext, responseAPDU.data+bytesProcessed,  apduResponseLength-bytesProcessed);

		if(calleeRetVal < 0)
		{
			sw = APDU_CORE_SW_SECURITY_STATUS_NOT_SATISFIED;
			goto END;
		}

		bytesProcessed += calleeRetVal;
	}

	responseAPDU.sw = APDU_CORE_SW_NO_ERROR;
	responseAPDU.dataLength = mk82SslDataToBeSentLength;

    successfullyProcessed = MK82_TRUE;

END:

	if(successfullyProcessed != MK82_TRUE)
	{
		mk82SslReset();
		responseAPDU.sw = sw;
		responseAPDU.dataLength = 0;
	}

	apduCorePrepareOutgoingAPDU(apdu, apduLength, &responseAPDU);
}

void mk82SslWipeout(void)
{
    uint32_t bytesWritten = 0;
    uint8_t wipeoutBuffer[MK82_SSL_WIPEOUT_BUFFER_SIZE];
    uint16_t trueOrFalse;

    mk82SystemMemSet(wipeoutBuffer, 0x00, sizeof(wipeoutBuffer));

    while ((bytesWritten + sizeof(wipeoutBuffer)) < sizeof(SSL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, bytesWritten, wipeoutBuffer, sizeof(wipeoutBuffer));
        bytesWritten += sizeof(wipeoutBuffer);
    }

    if (bytesWritten < sizeof(SSL_NVM_KEYS))
    {
        mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, bytesWritten, wipeoutBuffer,
                        (sizeof(SSL_NVM_KEYS) - bytesWritten));
    }

    trueOrFalse = MK82_FALSE;
    mk82FsWriteFile(MK82_FS_FILE_ID_SSL_KEYS, offsetof(SSL_NVM_KEYS, keyInitialized), (uint8_t*)&trueOrFalse,
                    sizeof(trueOrFalse));

    mk82FsCommitWrite(MK82_FS_FILE_ID_SSL_KEYS);
}
