/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "nt_types.h"

#if NT_FREEMASTER_SUPPORT != 0

// System includes
#include "nt_system.h"
#include "nt_system.h"
#include "../source/system/nt_system_prv.h"
#include "../source/system/nt_system_mem_prv.h"

//Controls includes
#include "nt_controls.h"
#include "../source/controls/nt_controls_prv.h"
#include "nt_control_arotary.h"
#include "../source/controls/nt_control_arotary_prv.h"
#include "nt_control_aslider.h"
#include "../source/controls/nt_control_aslider_prv.h"
#include "nt_control_keypad.h"
#include "../source/controls/nt_control_keypad_prv.h"
#include "nt_control_rotary.h"
#include "../source/controls/nt_control_rotary_prv.h"
#include "nt_control_slider.h"
#include "../source/controls/nt_control_slider_prv.h"
#include "nt_control_proxi.h"
#include "../source/controls/nt_control_proxi_prv.h"

//Electrodes includes
#include "nt_electrodes.h"
#include "../source/electrodes/nt_electrodes_prv.h"

//Modules includes
#include "nt_modules.h"
#include "../source/modules/nt_modules_prv.h"
#include "nt_module_tsi.h"
#include "../source/modules/nt_module_tsi_prv.h"
#include "nt_module_gpio.h"
#include "../source/modules/nt_module_gpio_prv.h"
#include "nt_module_gpioint.h"
#include "../source/modules/nt_module_gpioint_prv.h"

//Keydetectors includes
#include "../source/keydetectors/nt_keydetector_safa_prv.h"
#include "../source/keydetectors/nt_keydetector_afid_prv.h"


/* If the error occur that the compiler can't find this header file. So it means 
 * that the FreeMASTER is not part of the project and there is two possible solution:
 *  - Add FreeMASTER into project and it can be use to debug
 *  - remove this file from project.
*/
#include "freemaster.h" 

#if FMSTR_DISABLE != 0
  #error The FreeMASTER is disabled, In this case the FT FreeMASTER support (NT_FREEMASTER_SUPPORT) should be also disabled.
#endif

#if FMSTR_USE_TSA == 0
  #error The FreeMASTER TSA tables are disabled, the FT required this functionality, enable FMSTR_USE_TSA option.
#endif

#ifndef FMSTR_USE_TSA_DYNAMIC
  #error The FreeMASTER TSA dynamic tables are disabled, the FT required this functionality, enable FMSTR_USE_TSA_DYNAMIC option.
#elif FMSTR_USE_TSA_DYNAMIC == 0
  #error The FreeMASTER TSA dynamic tables are disabled, the FT required this functionality, enable FMSTR_USE_TSA_DYNAMIC option.
#endif

extern struct nt_kernel nt_kernel_data;

FMSTR_TSA_TABLE_BEGIN(nt_frmstr_tsa_table)

  FMSTR_TSA_RO_VAR(nt_kernel_data, FMSTR_TSA_USERTYPE(struct nt_kernel))

/*******************************************************************
*                       SYSTEM types
*******************************************************************/

/******************* nt_system_prv.h ******************************/
// nt_kernel
  FMSTR_TSA_STRUCT(struct nt_kernel)
  FMSTR_TSA_MEMBER(struct nt_kernel, rom,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_kernel, controls,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_kernel, modules,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_kernel, controls_cnt,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_kernel, modules_cnt,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_kernel, time_counter,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_kernel, memory,   FMSTR_TSA_USERTYPE(struct nt_mem))

/*************** nt_system_mem_prv.h ******************************/
// nt_mem
  FMSTR_TSA_STRUCT(struct nt_mem)
  FMSTR_TSA_MEMBER(struct nt_mem, pool,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_mem, pool_size,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_mem, free_pointer,   FMSTR_TSA_POINTER)
    

/*******************************************************************
*                       MODULES types
*******************************************************************/

/************************ nt_modules.h *****************************/
// nt_module
  FMSTR_TSA_STRUCT(struct nt_module)
  FMSTR_TSA_MEMBER(struct nt_module, interface,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module, electrodes,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module, module_params,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module, config,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module, instance,   FMSTR_TSA_UINT8)

/********************* nt_module_tsi.h ***************************/
// nt_module_tsi_params
  FMSTR_TSA_STRUCT(struct nt_module_tsi_params)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_params, noise,   FMSTR_TSA_USERTYPE(nt_module_tsi_noise))

// nt_module_tsi_noise
  FMSTR_TSA_STRUCT(struct nt_module_tsi_noise)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_noise, noise_filter,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_noise, update_rate,   FMSTR_TSA_UINT8)

/********************** nt_modules_prv.h ***************************/
// nt_module_data
  FMSTR_TSA_STRUCT(struct nt_module_data)
  FMSTR_TSA_MEMBER(struct nt_module_data, rom,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module_data, electrodes,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_module_data, active_mode,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_module_data, flags,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_module_data, electrodes_cnt,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_module_data, special_data,   FMSTR_TSA_POINTER)    

/******************** nt_module_tsi_prv.h *************************/
// nt_module_tsi_noise_data
  FMSTR_TSA_STRUCT(struct nt_module_tsi_noise_data)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_noise_data, filter_state,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_noise_data, noise,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_noise_data, touch_threshold,   FMSTR_TSA_UINT8)

// nt_module_tsi_data
  FMSTR_TSA_STRUCT(struct nt_module_tsi_data)
  FMSTR_TSA_MEMBER(struct nt_module_tsi_data, tsi_state,   FMSTR_TSA_UINT32)  

/******************** nt_module_gpioint_prv.h *************************/
// nt_module_gpioint_data
  FMSTR_TSA_STRUCT(struct nt_module_gpioint_data)
  FMSTR_TSA_MEMBER(struct nt_module_gpioint_data, pen,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_module_gpioint_data, measured_pin,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_module_gpioint_data, scan_status,   FMSTR_TSA_UINT8)

/********************* nt_module_gpio_prv.h ***************************/
// nt_module_gpio_data
  FMSTR_TSA_STRUCT(struct nt_module_gpio_data)
  FMSTR_TSA_MEMBER(struct nt_module_gpio_data, pen,   FMSTR_TSA_UINT32)

/*******************************************************************
*                       ELECTRODES types
*******************************************************************/

/********************** nt_electrodes.h ***************************/
// nt_electrode_status
  FMSTR_TSA_STRUCT(struct nt_electrode_status)
  FMSTR_TSA_MEMBER(struct nt_electrode_status, time_stamp,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_electrode_status, state,   FMSTR_TSA_UINT8)

// nt_electrode
  FMSTR_TSA_STRUCT(struct nt_electrode)
  FMSTR_TSA_MEMBER(struct nt_electrode, shielding_electrode,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode, multiplier,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_electrode, divider,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_electrode, pin_input,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_electrode, keydetector_interface,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode, keydetector_params,   FMSTR_TSA_POINTER)

/***************** nt_electrodes_prv.h ****************************/
// nt_electrode_data
  FMSTR_TSA_STRUCT(struct nt_electrode_data)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, rom,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, module_data,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, signal,   FMSTR_TSA_UINT16)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, raw_signal,   FMSTR_TSA_UINT16)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, baseline,   FMSTR_TSA_UINT16)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, status,   FMSTR_TSA_USERTYPE(nt_electrode_status))
  FMSTR_TSA_MEMBER(struct nt_electrode_data, status_index,   FMSTR_TSA_UINT8)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, flags,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, keydetector_data,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, special_data,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_electrode_data, shielding_electrode,   FMSTR_TSA_POINTER)

// nt_electrode_special_data
  FMSTR_TSA_STRUCT(union nt_electrode_special_data)
  FMSTR_TSA_MEMBER(union nt_electrode_special_data, tsi_noise,   FMSTR_TSA_UINT32)  //todo


/*******************************************************************
*                       KEYDETECTORS types
*******************************************************************/
/************** nt_keydetector_safa_prv.h *************************/
// nt_keydetector_safa_data
  FMSTR_TSA_STRUCT(struct nt_keydetector_safa_data)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, filter_state,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, predicted_signal_avrg,   FMSTR_TSA_UINT16)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, noise,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, predicted_signal,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, entry_event_cnt,   FMSTR_TSA_SINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_safa_data, deadband_cnt,   FMSTR_TSA_SINT32)
    
/************** nt_keydetector_afid_prv.h *************************/
// nt_keydetector_afid_data
  FMSTR_TSA_STRUCT(struct nt_keydetector_afid_data)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, slow_signal,   FMSTR_TSA_UINT16)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, asc,   FMSTR_TSA_USERTYPE(nt_keydetector_afid_asc_data))
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, filter_state,   FMSTR_TSA_UINT8)  
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, integration_value,   FMSTR_TSA_SINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, touch_threshold,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, touch_reset_counter,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_data, release_reset_counter,   FMSTR_TSA_UINT32)

// nt_keydetector_afid_asc_data
  FMSTR_TSA_STRUCT(struct nt_keydetector_afid_asc_data)
  FMSTR_TSA_MEMBER(struct nt_keydetector_afid_asc_data, max_resets,   FMSTR_TSA_SINT32) 

/*******************************************************************
*                       FILTERS types
*******************************************************************/

/*******************************************************************
*                       CONTROLS types
*******************************************************************/
    
/************************* nt_controls.h ***************************/
// nt_control
  FMSTR_TSA_STRUCT(struct nt_control)
  FMSTR_TSA_MEMBER(struct nt_control, interface,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control, electrodes,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control, control_params,   FMSTR_TSA_POINTER)

/***************** nt_controls_prv.h ******************************/
// nt_control_data
  FMSTR_TSA_STRUCT(struct nt_control_data)
  FMSTR_TSA_MEMBER(struct nt_control_data, rom,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control_data, data,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control_data, electrodes,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control_data, flags,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_control_data, electrodes_size,   FMSTR_TSA_UINT8)

/********************* nt_control_arotary.h ***************************/
// nt_control_arotary
  FMSTR_TSA_STRUCT(struct nt_control_arotary)
  FMSTR_TSA_MEMBER(struct nt_control_arotary, range,   FMSTR_TSA_UINT8)

/********************* nt_control_arotary_prv.h ***************************/
// nt_control_arotary
  FMSTR_TSA_STRUCT(struct nt_control_arotary_data)
  FMSTR_TSA_MEMBER(struct nt_control_arotary_data, position,   FMSTR_TSA_UINT8)
   
/********************* nt_control_aslider.h ***************************/
// nt_control_aslider
  FMSTR_TSA_STRUCT(struct nt_control_aslider)
  FMSTR_TSA_MEMBER(struct nt_control_aslider, range,   FMSTR_TSA_UINT8)

/********************* nt_control_arotary_prv.h ***************************/
// nt_control_arotary
  FMSTR_TSA_STRUCT(struct nt_control_aslider_data)
  FMSTR_TSA_MEMBER(struct nt_control_aslider_data, position,   FMSTR_TSA_UINT8)
    
/********************* nt_control_rotary_prv.h ***************************/
// nt_control_arotary
  FMSTR_TSA_STRUCT(struct nt_control_rotary_data)
  FMSTR_TSA_MEMBER(struct nt_control_rotary_data, position,   FMSTR_TSA_UINT8)
    
/********************* nt_control_keypad.h ***************************/
// nt_control_keypad
  FMSTR_TSA_STRUCT(struct nt_control_keypad)
  FMSTR_TSA_MEMBER(struct nt_control_keypad, groups,   FMSTR_TSA_POINTER)
  FMSTR_TSA_MEMBER(struct nt_control_keypad, groups_size,   FMSTR_TSA_UINT8)

/********************* nt_control_keypad_prv.h ***************************/
// nt_control_keypad_data
  FMSTR_TSA_STRUCT(struct nt_control_keypad_data)
  FMSTR_TSA_MEMBER(struct nt_control_keypad_data, last_state,   FMSTR_TSA_UINT32)
  FMSTR_TSA_MEMBER(struct nt_control_keypad_data, autorepeat_rate,   FMSTR_TSA_UINT16)

/********************* nt_control_slider_prv.h ***************************/
// nt_control_slider
  FMSTR_TSA_STRUCT(struct nt_control_slider_data)
  FMSTR_TSA_MEMBER(struct nt_control_slider_data, position,   FMSTR_TSA_UINT8)

/********************* nt_control_proxi_prv.h ***************************/
// nt_control_proxi
  FMSTR_TSA_STRUCT(struct nt_control_proxi)
  FMSTR_TSA_MEMBER(struct nt_control_proxi, range, FMSTR_TSA_UINT32)    
  FMSTR_TSA_STRUCT(struct nt_control_proxi_data)
  FMSTR_TSA_MEMBER(struct nt_control_proxi_data, proximity, FMSTR_TSA_SINT32) 
  FMSTR_TSA_MEMBER(struct nt_control_proxi_data, index, FMSTR_TSA_UINT8)

FMSTR_TSA_TABLE_END()
    
int32_t _nt_freemaster_init(void)
{
  // This is workaround, here could be code that recognise count of really used interfaces
  #define NT_MAXIMAL_COUNT_OF_INTERFACES        12
  
  uint8_t * p_freemaster_tsa_buffer = _nt_mem_alloc(NT_MAXIMAL_COUNT_OF_INTERFACES * sizeof(FMSTR_TSA_ENTRY));
  
  if(p_freemaster_tsa_buffer == NULL)
  {
    return NT_OUT_OF_MEMORY;
  }
  
  FMSTR_SetUpTsaBuff(p_freemaster_tsa_buffer, NT_MAXIMAL_COUNT_OF_INTERFACES * sizeof(FMSTR_TSA_ENTRY));
  
  return NT_SUCCESS;
}
 
int32_t _nt_freemaster_add_variable(const char * name, const char * type_name, void* address, uint32_t size)
{
  if(FMSTR_TsaAddVar(name, type_name, address, size, FMSTR_TSA_INFO_RO_VAR) == 0)
  {
    return NT_FAILURE;
  }
  return NT_SUCCESS;
}

#else
/* Section of FreeMASTER support function for case that FreeMASTER is not enebled */

int32_t _nt_freemaster_init(void)
{
  return NT_SUCCESS;
}

int32_t _nt_freemaster_add_variable(const char * name, const char * type_name, void* address, uint32_t size)
{
  (void)name;
  (void)type_name;
  (void)address;
  (void)size;
  
  return NT_SUCCESS;
}

#endif
