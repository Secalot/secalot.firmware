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

int main(void) {
	
	MK82_BOOT_INFO bootInfo;
	
	mk82BootInfoInit();

	mk82BootInfoGetData(&bootInfo);

	if(bootInfo.bootTarget == MK82_BOOT_INFO_START_FIRMWARE)
	{
		if(bootInfo.firmwareValid == MK82_TRUE)
		{
			mk82BootInfoBootFirmware();
		}
		else if(bootInfo.bootloaderValid == MK82_TRUE)
		{
			mk82BootInfoBootBootloader();
		}
		else
		{
			mk82SystemFatalError();
		}
	}
	else
	{
		if(bootInfo.bootloaderValid == MK82_TRUE)
		{
			mk82BootInfoBootBootloader();
		}
		else if(bootInfo.firmwareValid == MK82_TRUE)
		{
			mk82BootInfoBootFirmware();
		}
		else
		{
			mk82SystemFatalError();
		}
	}
}
