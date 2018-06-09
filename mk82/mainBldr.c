/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82System.h"
#include "mk82BootInfo.h"
#include "mk82Usb.h"

#include "bldrGlobal.h"
#include "bldrCore.h"

int main(void) {
	
	
	mk82BootInfoInit();
	mk82SystemInit();
	mk82UsbInit();
	
	bldrCoreInit();
	
 	while(1)
	{
		uint8_t* data;
		uint32_t dataLength;
		uint16_t dataType;
		uint16_t newUsbCommandReceived;
		
		newUsbCommandReceived = mk82UsbCheckForNewCommand(MK82_GLOBAL_PROCESS_ALL_DATATYPES, &data, &dataLength, &dataType);
		
		if(newUsbCommandReceived == MK82_USB_COMMAND_RECEIVED)
		{
			if(dataType == MK82_GLOBAL_DATATYPE_CCID_APDU)
			{
				bldrCoreProcessAPDU(data, &dataLength);
			}
			else
			{
				mk82SystemFatalError();
			}
			
			mk82UsbSendResponse(dataLength, dataType);
		}
	}
}
