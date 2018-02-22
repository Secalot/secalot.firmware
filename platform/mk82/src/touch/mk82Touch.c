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
#include "mk82Touch.h"
#include "mk82TouchInt.h"

#include <fsl_lptmr.h>
#include <fsl_port.h>

#include "nt.h"
#include "nt_drv_tsi_driver.h"

extern void TSI_DRV_IRQHandler(uint32_t instance);

static MK82_TOUCH_CALLBACK mk82TouchButton1PressedCallback = NULL;
static MK82_TOUCH_CALLBACK mk82TouchButton2PressedCallback = NULL;
static MK82_TOUCH_CALLBACK mk82TouchBothButtonsPressedCallback = NULL;

static void mk82TouchKeypadCallback(const struct nt_control *control, enum nt_control_keypad_event event,
                                    uint32_t index);

static uint8_t mk82TouchMemoryPool[MK82_TOUCH_MEMORY_POOL_SIZE] MK82_ALIGN(4);

static const struct nt_keydetector_afid mk82TouchKeyDetector = {
    .signal_filter = 1,
    .fast_signal_filter = {.cutoff = 6},
    .slow_signal_filter = {.cutoff = 2},
    .base_avrg = {.n2_order = 12},
    .reset_rate = 10,
    .asc =
        {
            .touch_treshold_fall_rate = 1000, .noise_resets_minimum = 256, .resets_for_touch = 5,
        },
};

static const struct nt_electrode mk82TouchElectrode1 = {
    .pin_input = MK82_TOUCH_ELECTRODE_1,
    .keydetector_interface = &nt_keydetector_afid_interface,
    .keydetector_params.afid = &mk82TouchKeyDetector,
};

static const struct nt_electrode mk82TouchElectrode2 = {
    .pin_input = MK82_TOUCH_ELECTRODE_2,
    .keydetector_interface = &nt_keydetector_afid_interface,
    .keydetector_params.afid = &mk82TouchKeyDetector,
};

static const struct nt_electrode *const mk82TouchElectrodes[] = {&mk82TouchElectrode1, &mk82TouchElectrode2, NULL};

static const tsi_config_t mk82TouchHardwareConfig = {
    .prescaler = kTSI_ElecOscPrescaler_64div,
    .extchrg = kTSI_ExtOscChargeCurrent_32uA,
    .refchrg = kTSI_RefOscChargeCurrent_32uA,
    .nscn = kTSI_ConsecutiveScansNumber_24time,
    .mode = kTSI_AnalogModeSel_Capacitive,
    .dvolt = kTSI_OscVolRailsOption_0,
    .thresh = 0,
    .thresl = 0,
    .filter = kTSI_FilterBits_0,
};

static const struct nt_module mk82TouchModule = {
    .interface = &nt_module_tsi_interface,
    .electrodes = &mk82TouchElectrodes[0],
    .config = (void *)&mk82TouchHardwareConfig,
    .instance = 0,
    .module_params = NULL,
};

static const struct nt_control_keypad mk82TouchKeypadParams = {
    .groups = NULL, .groups_size = 0,
};

static const struct nt_control mk82TouchKeypad = {
    .interface = &nt_control_keypad_interface,
    .electrodes = mk82TouchElectrodes,
    .control_params.keypad = &mk82TouchKeypadParams,
};

static const struct nt_control *const mk82TouchControls[] = {&mk82TouchKeypad, NULL};
static const struct nt_module *const mk82TouchModules[] = {&mk82TouchModule, NULL};

static const struct nt_system mk82TouchSystem = {
    .controls = &mk82TouchControls[0],
    .modules = &mk82TouchModules[0],
    .time_period = MK82_TOUCH_TIMER_TIMEOUT_IN_MS,
    .init_time = 50,
};

static uint16_t button1Pressed = MK82_FALSE;
static uint16_t button2Pressed = MK82_FALSE;

static void mk82TouchKeypadCallback(const struct nt_control *control, enum nt_control_keypad_event event,
                                    uint32_t index)
{
    switch (event)
    {
        case NT_KEYPAD_RELEASE:
            switch (index)
            {
                case 0:
                    if (button1Pressed == MK82_TRUE)
                    {
                        if (button2Pressed == MK82_TRUE)
                        {
                            if (mk82TouchBothButtonsPressedCallback != NULL)
                            {
                                mk82TouchBothButtonsPressedCallback();
                            }
                        }
                        else
                        {
                            if (mk82TouchButton1PressedCallback != NULL)
                            {
                                mk82TouchButton1PressedCallback();
                            }
                        }
                    }
                    break;
                case 1:
                    if (button2Pressed == MK82_TRUE)
                    {
                        if (button1Pressed == MK82_TRUE)
                        {
                            if (mk82TouchBothButtonsPressedCallback != NULL)
                            {
                                mk82TouchBothButtonsPressedCallback();
                            }

                            button1Pressed == MK82_FALSE;
                        }
                        else
                        {
                            if (mk82TouchButton2PressedCallback != NULL)
                            {
                                mk82TouchButton2PressedCallback();
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
            button1Pressed = MK82_FALSE;
            button2Pressed = MK82_FALSE;
            mk82LedBlueOff();
            mk82LedRedOff();
            break;
        case NT_KEYPAD_TOUCH:
            switch (index)
            {
                case 0:
                    mk82LedBlueOn();
                    button1Pressed = MK82_TRUE;
                    break;
                case 1:
                    mk82LedRedOn();
                    button2Pressed = MK82_TRUE;
                    break;
                default:
                    break;
            }
            break;
    }
}

void LPTMR0_LPTMR1_IRQHandler(void)
{
    uint32_t lptmr0StatusFlag;

    lptmr0StatusFlag = LPTMR_GetStatusFlags(LPTMR0);

    if (lptmr0StatusFlag != 0)
    {
        LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
        nt_trigger();
    }
}

void TSI0_IRQHandler(void) { TSI_DRV_IRQHandler(0); }

void mk82TouchInit(void)
{
    int32_t calleeRetVal;
    lptmr_config_t lptmrConfig;

    CLOCK_EnableClock(kCLOCK_PortB);
    PORT_SetPinMux(PORTB, MK82_TOUCH_ELECTRODE1_PORTB_PIN, kPORT_PinDisabledOrAnalog);
    PORT_SetPinMux(PORTB, MK82_TOUCH_ELECTRODE2_PORTB_PIN, kPORT_PinDisabledOrAnalog);

    calleeRetVal = nt_init(&mk82TouchSystem, mk82TouchMemoryPool, MK82_TOUCH_MEMORY_POOL_SIZE);

    if (calleeRetVal != NT_SUCCESS)
    {
        mk82SystemFatalError();
    }

    nt_electrode_enable(&mk82TouchElectrode1, 0);
    nt_electrode_enable(&mk82TouchElectrode2, 0);

    nt_module_recalibrate(&mk82TouchModule, NULL);
    nt_control_enable(&mk82TouchKeypad);
    nt_control_keypad_register_callback(&mk82TouchKeypad, &mk82TouchKeypadCallback);

    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(LPTMR0, &lptmrConfig);
    LPTMR_SetTimerPeriod(LPTMR0, MSEC_TO_COUNT(MK82_TOUCH_TIMER_TIMEOUT_IN_MS, CLOCK_GetFreq(kCLOCK_LpoClk)));
    LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);
    NVIC_EnableIRQ(LPTMR0_LPTMR1_IRQn);

    LPTMR_StartTimer(LPTMR0);
}

void mk82TouchEnable(void) { EnableIRQ(LPTMR0_LPTMR1_IRQn); }

void mk82TouchDisable(void) { DisableIRQ(LPTMR0_LPTMR1_IRQn); }

void mk82TouchTask(void) { nt_task(); }

void mk82TouchRegisterButton1PressedCallback(MK82_TOUCH_CALLBACK callback)
{
    mk82TouchButton1PressedCallback = callback;
}

void mk82TouchRegisterButton2PressedCallback(MK82_TOUCH_CALLBACK callback)
{
    mk82TouchButton2PressedCallback = callback;
}

void mk82TouchDeregisterButton2PressedCallback(void) { mk82TouchButton2PressedCallback = NULL; }

void mk82TouchRegisterBothButtonsPressedCallback(MK82_TOUCH_CALLBACK callback)
{
    mk82TouchBothButtonsPressedCallback = callback;
}
