/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "fsl_device_registers.h"

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82Usb.h"
#include "mk82Fs.h"
#include "mk82As.h"
#include "mk82Keysafe.h"
#include "mk82BootInfo.h"
#include "mk82Led.h"

#ifdef USE_BUTTON
	#include "mk82Button.h"
#endif

#ifdef USE_TOUCH
	#include "mk82Touch.h"
#endif

#include "opgpGlobal.h"
#include "opgpCore.h"

#include "sfGlobal.h"
#include "sfCore.h"

#include "otpGlobal.h"
#include "otpCore.h"

#include "btcGlobal.h"
#include "btcCore.h"

#include "ethGlobal.h"
#include "ethCore.h"

#include "bldrGlobal.h"
#include "bldrCore.h"
#include "bldrHal.h"

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"


#define HTTP_RESPONSE \
    "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n" \
    "<h2>mbed TLS Test Server</h2>\r\n" \
    "<p>Successful connection using: %s</p>\r\n"

static uint16_t computeOTP = MK82_FALSE;

static void otpButtonPressed(void)
{
	computeOTP = MK82_TRUE;
}

static void checkForSpecialBootloaderActivation(uint8_t* data, uint32_t dataLength)
{
	if(dataLength == 4)
	{
		if( (data[0] == 0x5A) && (data[1] == 0xFE) && (data[2] == 0xB0) && (data[3] == 0x01) )
		{
			uint16_t bootloaderValid;

			bldrHalIsBootloaderValid(&bootloaderValid);

			if(bootloaderValid == BLDR_TRUE)
			{
				bldrHalSetBootloaderAsBootTarget();
			}
		}
		
	}
}

int main(void) {
	
	uint16_t firstCommandReceived = MK82_FALSE;
  
	
	mk82SystemInit();	
	mk82BootInfoInit();
	mk82UsbInit();
	mk82LedInit();
#ifdef USE_BUTTON
	mk82ButtonInit();
#endif
#ifdef USE_TOUCH
	mk82TouchInit();
#endif
	
#ifdef USE_BUTTON
	mk82ButtonRegisterButtonLongPressedCallback(otpButtonPressed);
#endif

#ifdef USE_TOUCH
	mk82TouchRegisterBothButtonsPressedCallback(otpButtonPressed);
#endif


	{
		int ret, len;
		mbedtls_net_context listen_fd, client_fd;
		unsigned char buf[1024];
		const char *pers = "ssl_server";

		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context ctr_drbg;
		mbedtls_ssl_context ssl;
		mbedtls_ssl_config conf;
		mbedtls_x509_crt srvcert;
		mbedtls_pk_context pkey;
#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_cache_context cache;
#endif

//		mbedtls_net_init( &listen_fd );
//		mbedtls_net_init( &client_fd );
		mbedtls_ssl_init( &ssl );
		mbedtls_ssl_config_init( &conf );
#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_cache_init( &cache );
#endif
		mbedtls_x509_crt_init( &srvcert );
		mbedtls_pk_init( &pkey );
		mbedtls_entropy_init( &entropy );
		mbedtls_ctr_drbg_init( &ctr_drbg );

#if defined(MBEDTLS_DEBUG_C)
		mbedtls_debug_set_threshold( DEBUG_LEVEL );
#endif

		/*
		 * 1. Load the certificates and private RSA key
		 */

		/*
		 * This demonstration program uses embedded test certificates.
		 * Instead, you may want to use mbedtls_x509_crt_parse_file() to read the
		 * server and CA certificates, as well as mbedtls_pk_parse_keyfile().
		 */
//		ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_srv_crt,
//				mbedtls_test_srv_crt_len );
		ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) NULL, 0);

		if( ret != 0 )
		{
			goto exit;
		}

//		ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) mbedtls_test_cas_pem,
//				mbedtls_test_cas_pem_len );

		ret = mbedtls_x509_crt_parse( &srvcert, (const unsigned char *) NULL, 0);

		if( ret != 0 )
		{
			goto exit;
		}

//		ret =  mbedtls_pk_parse_key( &pkey, (const unsigned char *) mbedtls_test_srv_key,
//				mbedtls_test_srv_key_len, NULL, 0 );
		ret =  mbedtls_pk_parse_key( &pkey, (const unsigned char *) NULL, 0, NULL, 0 );
		if( ret != 0 )
		{
			goto exit;
		}

		/*
		 * 2. Setup the listening TCP socket
		 */

//		if( ( ret = mbedtls_net_bind( &listen_fd, NULL, "4433", MBEDTLS_NET_PROTO_TCP ) ) != 0 )
//		{
//			goto exit;
//		}

		/*
		 * 3. Seed the RNG
		 */
		if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
				(const unsigned char *) pers,
				strlen( pers ) ) ) != 0 )
		{
			goto exit;
		}

		/*
		 * 4. Setup stuff
		 */

		if( ( ret = mbedtls_ssl_config_defaults( &conf,
				MBEDTLS_SSL_IS_SERVER,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
		{
			goto exit;
		}

		mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_conf_session_cache( &conf, &cache,
				mbedtls_ssl_cache_get,
				mbedtls_ssl_cache_set );
#endif

		mbedtls_ssl_conf_ca_chain( &conf, srvcert.next, NULL );
		if( ( ret = mbedtls_ssl_conf_own_cert( &conf, &srvcert, &pkey ) ) != 0 )
		{
			goto exit;
		}

		if( ( ret = mbedtls_ssl_setup( &ssl, &conf ) ) != 0 )
		{
			goto exit;
		}

		reset:
#ifdef MBEDTLS_ERROR_C
		if( ret != 0 )
		{
			char error_buf[100];
			mbedtls_strerror( ret, error_buf, 100 );
			mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
		}
#endif

//		mbedtls_net_free( &client_fd );

		mbedtls_ssl_session_reset( &ssl );

		/*
		 * 3. Wait until a client connects
		 */
//		if( ( ret = mbedtls_net_accept( &listen_fd, &client_fd,
//				NULL, 0, NULL ) ) != 0 )
//		{
//			goto exit;
//		}

//		mbedtls_ssl_set_bio( &ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL );
		mbedtls_ssl_set_bio( &ssl, &client_fd, NULL, NULL, NULL );

		/*
		 * 5. Handshake
		 */

		while( ( ret = mbedtls_ssl_handshake( &ssl ) ) != 0 )
		{
			if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
			{
				goto reset;
			}
		}

		/*
		 * 6. Read the HTTP Request
		 */

		do
		{
			len = sizeof( buf ) - 1;
			memset( buf, 0, sizeof( buf ) );
			ret = mbedtls_ssl_read( &ssl, buf, len );

			if( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE )
				continue;

			if( ret <= 0 )
			{
				switch( ret )
				{
				case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
					break;

				case MBEDTLS_ERR_NET_CONN_RESET:
					break;

				default:
					break;
				}

				break;
			}

			len = ret;

			if( ret > 0 )
				break;
		}
		while( 1 );

		/*
		 * 7. Write the 200 Response
		 */

//		len = sprintf( (char *) buf, HTTP_RESPONSE,
//				mbedtls_ssl_get_ciphersuite( &ssl ) );

		while( ( ret = mbedtls_ssl_write( &ssl, buf, len ) ) <= 0 )
		{
			if( ret == MBEDTLS_ERR_NET_CONN_RESET )
			{
				goto reset;
			}

			if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE )
			{
				goto exit;
			}
		}

		len = ret;

		while( ( ret = mbedtls_ssl_close_notify( &ssl ) ) < 0 )
		{
			if( ret != MBEDTLS_ERR_SSL_WANT_READ &&
					ret != MBEDTLS_ERR_SSL_WANT_WRITE )
			{
				goto reset;
			}
		}

		ret = 0;
		goto reset;

		exit:

#ifdef MBEDTLS_ERROR_C
		if( ret != 0 )
		{
			char error_buf[100];
			mbedtls_strerror( ret, error_buf, 100 );
			mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf );
		}
#endif

//		mbedtls_net_free( &client_fd );
//		mbedtls_net_free( &listen_fd );

		mbedtls_x509_crt_free( &srvcert );
		mbedtls_pk_free( &pkey );
		mbedtls_ssl_free( &ssl );
		mbedtls_ssl_config_free( &conf );
#if defined(MBEDTLS_SSL_CACHE_C)
		mbedtls_ssl_cache_free( &cache );
#endif
		mbedtls_ctr_drbg_free( &ctr_drbg );
		mbedtls_entropy_free( &entropy );

#if defined(_WIN32)
		mbedtls_printf( "  Press Enter to exit this program.\n" );
		fflush( stdout ); getchar();
#endif




	}

 	while(1)
	{
		uint8_t* data;
		uint32_t dataLength;
		uint16_t dataType;
		uint16_t newUsbCommandReceived;

#ifdef USE_TOUCH
		mk82TouchTask();
#endif
		newUsbCommandReceived = mk82UsbCheckForNewCommand(&data, &dataLength, &dataType);
		
		if( (newUsbCommandReceived == MK82_USB_COMMAND_RECEIVED) || (computeOTP == MK82_TRUE) )
		{
#ifdef USE_TOUCH
			mk82TouchDisable();
#endif

			if(firstCommandReceived == MK82_FALSE)
			{
					checkForSpecialBootloaderActivation(data, dataLength);
				
					mk82AsInit();
					mk82FsInit();
					mk82KeysafeInit();
				
					opgpCoreInit();
					sfCoreInit();
					otpCoreInit();
					btcCoreInit();
					ethCoreInit();
					bldrCoreInit();
				
					firstCommandReceived = MK82_TRUE;
			}

			if(newUsbCommandReceived == MK82_USB_COMMAND_RECEIVED)
			{
				if(dataType == MK82_USB_DATATYPE_CCID_APDU)
				{
					mk82AsProcessAPDU(data, &dataLength);
				}
				else if(dataType == MK82_USB_DATATYPE_U2F_MESSAGE)
				{
					sfCoreProcessAPDU(data, &dataLength);
				}
				else if(dataType == MK82_USB_DATATYPE_BTC_MESSAGE)
				{
					btcCoreProcessAPDU(data, &dataLength);
				}
				else
				{
					mk82SystemFatalError();
				}

				mk82UsbSendResponse(dataLength, dataType);
			}
			else if(computeOTP == MK82_TRUE)
			{
				uint16_t calleeRetVal = OTP_GENERAL_ERROR;
				uint8_t otp[OTP_GLOBAL_MAX_OTP_LENGTH];
				uint32_t otpLength;

				computeOTP = MK82_FALSE;

				calleeRetVal = otpCoreComputeOtp(otp, &otpLength);

				if(calleeRetVal != OTP_NO_ERROR)
				{
					if( (calleeRetVal != OTP_KEY_NOT_SET_ERROR) && (calleeRetVal != OTP_TIME_NOT_SET_ERROR) )
					{
						mk82SystemFatalError();
					}
				}
				else
				{
					mk82UsbTypeStringWithAKeyboard(otp, otpLength);
				}
			}

#ifdef USE_TOUCH
			mk82TouchEnable();
#endif
		}
	}
}
