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
#include "mk82Button.h"
#include "mk82ButtonInt.h"

#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_lptmr.h"
#include "fsl_common.h"

static MK82_BUTTON_ACTION_CALLBACK mk82ButtonClickedCallback = NULL;
static MK82_BUTTON_ACTION_CALLBACK mk82ButtonDoubleClickedCallback = NULL;
static MK82_BUTTON_ACTION_CALLBACK mk82ButtonLongPressedCallback = NULL;

static uint16_t mk82ButtonCountingTimePressed = MK82_FALSE;
static uint16_t mk82ButtonCountingTimeReleased = MK82_FALSE;
static uint16_t mk82ButtonGoingForADoubleClick = MK82_FALSE;
static uint16_t mk82ButtonDebouncing = MK82_FALSE;
static uint16_t mk82ButtonWaitingForRelease = MK82_FALSE;
static uint16_t mk82ButtonPinValue = 0;

void PORTA_IRQHandler(void)
{
    uint32_t pinValue;

    GPIO_ClearPinsInterruptFlags(GPIOA, 1 << MK82_BUTTON_PIN);

    if (mk82ButtonDebouncing == MK82_FALSE)
    {
        pinValue = GPIO_ReadPinInput(GPIOA, MK82_BUTTON_PIN);

        if (((pinValue == 0) && (mk82ButtonWaitingForRelease == MK82_FALSE)) ||
            ((pinValue != 0) && (mk82ButtonWaitingForRelease == MK82_TRUE)))
        {
            mk82ButtonPinValue = pinValue;
            mk82ButtonDebouncing = MK82_TRUE;
            LPTMR_StartTimer(LPTMR0);
        }
    }
}

void LPTMR0_LPTMR1_IRQHandler(void)
{
    uint32_t lptmr0StatusFlag;

    lptmr0StatusFlag = LPTMR_GetStatusFlags(LPTMR0);

    if (lptmr0StatusFlag != 0)
    {
        uint32_t pinValue;

        LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
        LPTMR_StopTimer(LPTMR0);

        mk82ButtonDebouncing = MK82_FALSE;

        pinValue = GPIO_ReadPinInput(GPIOA, MK82_BUTTON_PIN);

        if (pinValue == mk82ButtonPinValue)
        {
            LPTMR_StopTimer(LPTMR1);

            // Press
            if (pinValue == 0x00)
            {
                if (mk82ButtonCountingTimeReleased == MK82_TRUE)
                {
                    mk82ButtonCountingTimeReleased = MK82_FALSE;
                    mk82ButtonGoingForADoubleClick = MK82_TRUE;
                }
                else
                {
                    mk82ButtonCountingTimePressed = MK82_TRUE;
                    LPTMR_SetTimerPeriod(
                        LPTMR1, MSEC_TO_COUNT(MK82_BUTTON_LONG_PRESS_DURATION_MS, CLOCK_GetFreq(kCLOCK_LpoClk)));
                    LPTMR_StartTimer(LPTMR1);
                }

                mk82ButtonWaitingForRelease = MK82_TRUE;
            }
            // Release
            else
            {
                if (mk82ButtonGoingForADoubleClick == MK82_TRUE)
                {
                    if (mk82ButtonDoubleClickedCallback != NULL)
                    {
                        mk82ButtonDoubleClickedCallback();
                    }

                    mk82ButtonGoingForADoubleClick = MK82_FALSE;
                }

                if (mk82ButtonCountingTimePressed == MK82_TRUE)
                {
                    mk82ButtonCountingTimePressed = MK82_FALSE;
                    mk82ButtonCountingTimeReleased = MK82_TRUE;
                    LPTMR_SetTimerPeriod(
                        LPTMR1, MSEC_TO_COUNT(MK82_BUTTON_DOUBLE_CLICK_WINDOW_MS, CLOCK_GetFreq(kCLOCK_LpoClk)));
                    LPTMR_StartTimer(LPTMR1);
                }

                mk82ButtonWaitingForRelease = MK82_FALSE;
            }
        }
    }
    else
    {
        LPTMR_ClearStatusFlags(LPTMR1, kLPTMR_TimerCompareFlag);
        LPTMR_StopTimer(LPTMR1);

        if (mk82ButtonCountingTimePressed == MK82_TRUE)
        {
            if (mk82ButtonLongPressedCallback != NULL)
            {
                mk82ButtonLongPressedCallback();
            }

            mk82ButtonCountingTimePressed = MK82_FALSE;
        }

        if (mk82ButtonCountingTimeReleased == MK82_TRUE)
        {
            if (mk82ButtonClickedCallback != NULL)
            {
                mk82ButtonClickedCallback();
            }

            mk82ButtonCountingTimeReleased = MK82_FALSE;
        }
    }
}

void mk82ButtonInit(void)
{
    gpio_pin_config_t gpioConfig = {kGPIO_DigitalInput, 0};
    port_pin_config_t portConfig = {kPORT_PullUp,           kPORT_FastSlewRate,     kPORT_PassiveFilterEnable,
                                    kPORT_OpenDrainDisable, kPORT_LowDriveStrength, kPORT_MuxAsGpio,
                                    kPORT_UnlockRegister};
    lptmr_config_t lptmrConfig;

    GPIO_PinInit(GPIOA, MK82_BUTTON_PIN, &gpioConfig);

    CLOCK_EnableClock(kCLOCK_PortA);
    PORT_SetPinInterruptConfig(PORTA, MK82_BUTTON_PIN, kPORT_InterruptEitherEdge);
    PORT_SetPinConfig(PORTA, MK82_BUTTON_PIN, &portConfig);
    EnableIRQ(PORTA_IRQn);

    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(LPTMR0, &lptmrConfig);
    LPTMR_Init(LPTMR1, &lptmrConfig);
    LPTMR_SetTimerPeriod(LPTMR0, MSEC_TO_COUNT(MK82_BUTTON_DEBOUNCING_WINDOW_MS, CLOCK_GetFreq(kCLOCK_LpoClk)));
    LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);
    LPTMR_EnableInterrupts(LPTMR1, kLPTMR_TimerInterruptEnable);
    NVIC_EnableIRQ(LPTMR0_LPTMR1_IRQn);
}

void mk82ButtonRegisterButtonClickedCallback(MK82_BUTTON_ACTION_CALLBACK buttonClickedCallback)
{
    mk82ButtonClickedCallback = buttonClickedCallback;
}

void mk82ButtonRegisterButtonDoubleClickedCallback(MK82_BUTTON_ACTION_CALLBACK buttonDoubleClickedCallback)
{
    mk82ButtonDoubleClickedCallback = buttonDoubleClickedCallback;
}

void mk82ButtonDeregisterButtonDoubleClickedCallback(void) { mk82ButtonDoubleClickedCallback = NULL; }

void mk82ButtonRegisterButtonLongPressedCallback(MK82_BUTTON_ACTION_CALLBACK buttonLongPressedCallback)
{
    mk82ButtonLongPressedCallback = buttonLongPressedCallback;
}
