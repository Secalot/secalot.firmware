/*
 * Secalot firmware.
 * Copyright (c) 2017 Matvey Mukha <matvey.mukha@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __MK82_LED_H__
#define __MK82_LED_H__

#ifdef __cplusplus
extern "C"
{
#endif

    void mk82LedInit(void);

    void mk82LedRedOn(void);
    void mk82LedRedOff(void);
    void mk82LedGreenOn(void);
    void mk82LedGreenOff(void);
    void mk82LedBlueOn(void);
    void mk82LedBlueOff(void);

#ifdef __cplusplus
}
#endif

#endif /* __MK82_LED_H__ */
