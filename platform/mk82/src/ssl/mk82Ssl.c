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
#include "mk82Ssl.h"
#include "mk82SslInt.h"

#include <apduGlobal.h>
#include <apduCore.h>

#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/error.h"



uint8_t theKey[] = {
	0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0xF1, 0x2A, 0x13, 0x20, 0x76, 0x02, 0x70, 0xA8, 0x3C,
	0xBF, 0xFD, 0x53, 0xF6, 0x03, 0x1E, 0xF7, 0x6A, 0x5D, 0x86, 0xC8, 0xA2, 0x04, 0xF2, 0xC3, 0x0C,
	0xA9, 0xEB, 0xF5, 0x1F, 0x0F, 0x0E, 0xA7, 0xA0, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D,
	0x03, 0x01, 0x07, 0xA1, 0x44, 0x03, 0x42, 0x00, 0x04, 0x37, 0xCC, 0x56, 0xD9, 0x76, 0x09, 0x1E,
	0x5A, 0x72, 0x3E, 0xC7, 0x59, 0x2D, 0xFF, 0x20, 0x6E, 0xEE, 0x7C, 0xF9, 0x06, 0x91, 0x74, 0xD0,
	0xAD, 0x14, 0xB5, 0xF7, 0x68, 0x22, 0x59, 0x62, 0x92, 0x4E, 0xE5, 0x00, 0xD8, 0x23, 0x11, 0xFF,
	0xEA, 0x2F, 0xD2, 0x34, 0x5D, 0x5D, 0x16, 0xBD, 0x8A, 0x88, 0xC2, 0x6B, 0x77, 0x0D, 0x55, 0xCD,
	0x8A, 0x2A, 0x0E, 0xFA, 0x01, 0xC8, 0xB4, 0xED, 0xFF
};

uint8_t theCert[] = {
		0x30, 0x82, 0x02, 0x1F, 0x30, 0x82, 0x01, 0xA5, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x09,
		0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x3E, 0x31, 0x0B,
		0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x4E, 0x4C, 0x31, 0x11, 0x30, 0x0F, 0x06,
		0x03, 0x55, 0x04, 0x0A, 0x13, 0x08, 0x50, 0x6F, 0x6C, 0x61, 0x72, 0x53, 0x53, 0x4C, 0x31, 0x1C,
		0x30, 0x1A, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x13, 0x50, 0x6F, 0x6C, 0x61, 0x72, 0x73, 0x73,
		0x6C, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x45, 0x43, 0x20, 0x43, 0x41, 0x30, 0x1E, 0x17, 0x0D,
		0x31, 0x33, 0x30, 0x39, 0x32, 0x34, 0x31, 0x35, 0x35, 0x32, 0x30, 0x34, 0x5A, 0x17, 0x0D, 0x32,
		0x33, 0x30, 0x39, 0x32, 0x32, 0x31, 0x35, 0x35, 0x32, 0x30, 0x34, 0x5A, 0x30, 0x34, 0x31, 0x0B,
		0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x4E, 0x4C, 0x31, 0x11, 0x30, 0x0F, 0x06,
		0x03, 0x55, 0x04, 0x0A, 0x13, 0x08, 0x50, 0x6F, 0x6C, 0x61, 0x72, 0x53, 0x53, 0x4C, 0x31, 0x12,
		0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x09, 0x6C, 0x6F, 0x63, 0x61, 0x6C, 0x68, 0x6F,
		0x73, 0x74, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06,
		0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x37, 0xCC, 0x56,
		0xD9, 0x76, 0x09, 0x1E, 0x5A, 0x72, 0x3E, 0xC7, 0x59, 0x2D, 0xFF, 0x20, 0x6E, 0xEE, 0x7C, 0xF9,
		0x06, 0x91, 0x74, 0xD0, 0xAD, 0x14, 0xB5, 0xF7, 0x68, 0x22, 0x59, 0x62, 0x92, 0x4E, 0xE5, 0x00,
		0xD8, 0x23, 0x11, 0xFF, 0xEA, 0x2F, 0xD2, 0x34, 0x5D, 0x5D, 0x16, 0xBD, 0x8A, 0x88, 0xC2, 0x6B,
		0x77, 0x0D, 0x55, 0xCD, 0x8A, 0x2A, 0x0E, 0xFA, 0x01, 0xC8, 0xB4, 0xED, 0xFF, 0xA3, 0x81, 0x9D,
		0x30, 0x81, 0x9A, 0x30, 0x09, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x1D,
		0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x50, 0x61, 0xA5, 0x8F, 0xD4, 0x07, 0xD9,
		0xD7, 0x82, 0x01, 0x0C, 0xE5, 0x65, 0x7F, 0x8C, 0x63, 0x46, 0xA7, 0x13, 0xBE, 0x30, 0x6E, 0x06,
		0x03, 0x55, 0x1D, 0x23, 0x04, 0x67, 0x30, 0x65, 0x80, 0x14, 0x9D, 0x6D, 0x20, 0x24, 0x49, 0x01,
		0x3F, 0x2B, 0xCB, 0x78, 0xB5, 0x19, 0xBC, 0x7E, 0x24, 0xC9, 0xDB, 0xFB, 0x36, 0x7C, 0xA1, 0x42,
		0xA4, 0x40, 0x30, 0x3E, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x4E,
		0x4C, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x08, 0x50, 0x6F, 0x6C, 0x61,
		0x72, 0x53, 0x53, 0x4C, 0x31, 0x1C, 0x30, 0x1A, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x13, 0x50,
		0x6F, 0x6C, 0x61, 0x72, 0x73, 0x73, 0x6C, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x45, 0x43, 0x20,
		0x43, 0x41, 0x82, 0x09, 0x00, 0xC1, 0x43, 0xE2, 0x7E, 0x62, 0x43, 0xCC, 0xE8, 0x30, 0x0A, 0x06,
		0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x68, 0x00, 0x30, 0x65, 0x02, 0x31,
		0x00, 0x9A, 0x2C, 0x5C, 0xD7, 0xA6, 0xDB, 0xA2, 0xE5, 0x64, 0x0D, 0xF0, 0xB9, 0x4E, 0xDD, 0xD7,
		0x61, 0xD6, 0x13, 0x31, 0xC7, 0xAB, 0x73, 0x80, 0xBB, 0xD3, 0xD3, 0x73, 0x13, 0x54, 0xAD, 0x92,
		0x0B, 0x5D, 0xAB, 0xD0, 0xBC, 0xF7, 0xAE, 0x2F, 0xE6, 0xA1, 0x21, 0x29, 0x35, 0x95, 0xAA, 0x3E,
		0x39, 0x02, 0x30, 0x21, 0x36, 0x7F, 0x9D, 0xC6, 0x5D, 0xC6, 0x0B, 0xAB, 0x27, 0xF2, 0x25, 0x1D,
		0x3B, 0xF1, 0xCF, 0xF1, 0x35, 0x25, 0x14, 0xE7, 0xE5, 0xF1, 0x97, 0xB5, 0x59, 0xE3, 0x5E, 0x15,
		0x7C, 0x66, 0xB9, 0x90, 0x7B, 0xC7, 0x01, 0x10, 0x4F, 0x73, 0xC6, 0x00, 0x21, 0x52, 0x2A, 0x0E,
		0xF1, 0xC7, 0xD5

};

static void mk82SslReset(void);
static int mk82SslSend( void *ctx, const unsigned char *buf, size_t len );
static int mk82SslReceive( void *ctx, unsigned char *buf, size_t len );

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



void mk82SslInit(void)
{
	int ret;

	mbedtls_ssl_init( &mk82SslContext );
	mbedtls_ssl_config_init( &mk82SslConfig );
	mbedtls_x509_crt_init( &mk82SslCert );
	mbedtls_pk_init( &mk82SslPkContext );

	ret = mbedtls_x509_crt_parse( &mk82SslCert, (const unsigned char *) theCert, sizeof(theCert));

	if( ret != 0 )
	{
		mk82SystemFatalError();
	}

	ret =  mbedtls_pk_parse_key( &mk82SslPkContext, (const unsigned char *) theKey, sizeof(theKey), NULL, 0 );

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

