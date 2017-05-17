/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mk82Global.h"
#include "mk82GlobalInt.h"
#include "mk82System.h"
#include "mk82Led.h"
#include "mk82LedInt.h"

#include <fsl_port.h>
#include <fsl_gpio.h>

void mk82LedInit(void)
{
    CLOCK_EnableClock(kCLOCK_PortC);
    PORT_SetPinMux(PORTC, MK82_LED_RED_PORTC_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(PORTC, MK82_LED_GREEN_PORTC_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(PORTC, MK82_LED_BLUE_PORTC_PIN, kPORT_MuxAsGpio);

    GPIO_PinInit(GPIOC, MK82_LED_RED_PORTC_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
    GPIO_PinInit(GPIOC, MK82_LED_GREEN_PORTC_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
    GPIO_PinInit(GPIOC, MK82_LED_BLUE_PORTC_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
}

void mk82LedRedOn(void) { GPIO_ClearPinsOutput(GPIOC, 1U << MK82_LED_RED_PORTC_PIN); }

void mk82LedRedOff(void) { GPIO_SetPinsOutput(GPIOC, 1U << MK82_LED_RED_PORTC_PIN); }

void mk82LedGreenOn(void) { GPIO_ClearPinsOutput(GPIOC, 1U << MK82_LED_GREEN_PORTC_PIN); }

void mk82LedGreenOff(void) { GPIO_SetPinsOutput(GPIOC, 1U << MK82_LED_GREEN_PORTC_PIN); }

void mk82LedBlueOn(void) { GPIO_ClearPinsOutput(GPIOC, 1U << MK82_LED_BLUE_PORTC_PIN); }

void mk82LedBlueOff(void) { GPIO_SetPinsOutput(GPIOC, 1U << MK82_LED_BLUE_PORTC_PIN); }
