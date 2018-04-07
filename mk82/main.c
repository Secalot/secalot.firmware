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
#include "mk82Ssl.h"

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

	volatile uint32_t test = 0;
	
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
					mk82SslInit();
				
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
					uint16_t sslStatus;

					mk82SslUnwrapAPDUCommand(data, &dataLength, &sslStatus);

					if( (sslStatus == MK82_SSL_STATUS_UNWRAPPED) || (sslStatus == MK82_SSL_STATUS_NOT_SSL) )
					{
						mk82AsProcessAPDU(data, &dataLength);
					}
					else
					{
						if(sslStatus != MK82_SSL_STATUS_ERROR_OCCURED)
						{
							mk82SystemFatalError();
						}
					}

					if(sslStatus == MK82_SSL_STATUS_UNWRAPPED)
					{
						mk82SslWrapAPDUResponse(data, &dataLength);
					}
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
