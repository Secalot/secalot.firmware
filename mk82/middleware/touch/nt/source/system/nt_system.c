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
#include "../source/system/nt_system_prv.h"
#include "../source/system/nt_system_mem_prv.h"
#include "../source/modules/nt_modules_prv.h"
#include "../source/controls/nt_controls_prv.h"
#include "../include/nt_system.h"
#include "nt_types.h"

struct nt_kernel nt_kernel_data;

/* System data check */
static int32_t _nt_system_check_data(const struct nt_system *system);
/* Get the count of pointer array ended by NULL pointer. */
static uint32_t _nt_system_count_pointer_array(const void** pointer_array);

/* System data check */
static int32_t _nt_system_check_data(const struct nt_system *system)
{
  if (system->modules == NULL)
  {
    return NT_FAILURE;
  }
  if (system->controls == NULL)
  {
    return NT_FAILURE;
  }
  if (system->time_period == 0U)
  {
    return NT_FAILURE;
  }
  return NT_SUCCESS;
}

/* Get the count of pointer array terminated by NULL pointer. */
static uint32_t _nt_system_count_pointer_array(const void** pointer_array)
{
  void** array = (void**)pointer_array;
  uint32_t count = 0;

  if(array == NULL)
  {
    return 0;
  }

  while(*array++)
  {
    count++;
  }

  return count;
}


/* internal function */
int32_t _nt_system_init(const struct nt_system *system)
{
    NT_ASSERT(system != NULL);
    if (_nt_system_check_data(system) < NT_SUCCESS) {
        return NT_FAILURE;
    }

    nt_kernel_data.controls_cnt = (uint8_t)_nt_system_count_pointer_array((const void**)system->controls);
    nt_kernel_data.modules_cnt = (uint8_t)_nt_system_count_pointer_array((const void**)system->modules);

    nt_kernel_data.controls = _nt_mem_alloc(sizeof(void*) * nt_kernel_data.controls_cnt);

    if(nt_kernel_data.controls == NULL)
    {
      return NT_OUT_OF_MEMORY;
    }

    nt_kernel_data.modules = _nt_mem_alloc(sizeof(void*) * nt_kernel_data.modules_cnt);

    if(nt_kernel_data.modules == NULL)
    {
      return NT_OUT_OF_MEMORY;
    }

    nt_kernel_data.rom = system;
    nt_kernel_data.time_counter = 0U;
    return NT_SUCCESS;
}

/* private function */
struct nt_kernel *_nt_system_get(void)
{
    return &nt_kernel_data;
}

/* private function */
void _nt_system_increment_time_counter(void)
{
  nt_kernel_data.time_counter += nt_kernel_data.rom->time_period;
}

/* private function */
uint32_t _nt_system_get_time_period(void)
{
  return (uint32_t)nt_kernel_data.rom->time_period;
}

/* public function */
uint32_t nt_system_get_time_offset(uint32_t event_stamp)
{
    uint32_t time_counter = nt_system_get_time_counter();
    if (time_counter > event_stamp) {
        return (time_counter - event_stamp);
    } else {
        return (event_stamp - time_counter);
    }
}

/* private function */
uint32_t _nt_system_get_time_offset_from_period(uint32_t event_period)
{
    if (event_period) {
        uint32_t time_period = _nt_system_get_time_period();
        return (uint32_t)((nt_system_get_time_counter() / time_period) % event_period);
    } else {
        return 1U; /* returns positive number, event should not be triggered */
    }
}

/* public function */
uint32_t nt_system_get_time_counter(void)
{
    return nt_kernel_data.time_counter;
}

/* internal function */
int32_t _nt_system_module_function(uint32_t option)
{
    int32_t result = NT_SUCCESS;
    struct nt_module_data *module;
    uint32_t i;

    /* steps through all control pointers */
    for(i=0;i<nt_kernel_data.modules_cnt;i++)
    {
      module = nt_kernel_data.modules[i];
        switch (option)
        {
            case NT_SYSTEM_MODULE_INIT:
             nt_kernel_data.modules[i] = _nt_module_init(nt_kernel_data.rom->modules[i]);
             if (nt_kernel_data.modules[i] == NULL)
              {
                  return NT_OUT_OF_MEMORY; /* failure stops the entire init phase */
              }
              break;
            case NT_SYSTEM_MODULE_TRIGGER:
                if (_nt_module_trigger(module) == NT_SCAN_IN_PROGRESS)
                {
                    result = NT_FAILURE; /* module not ready, triggering continues */
                }
                break;
            case NT_SYSTEM_MODULE_PROCESS:
                _nt_module_process(module);
                break;
            case NT_SYSTEM_MODULE_CHECK_DATA:
                if (!_nt_module_get_flag(module, NT_MODULE_NEW_DATA_FLAG)) {
                    return NT_FAILURE; /* module has not processed all data yet */
                }
                break;
            default:
                break;
        }
        module++;
    }
    return result;
}

/* internal function */
int32_t _nt_system_control_function(uint32_t option)
{
    struct nt_control_data *control;
    int32_t result = NT_SUCCESS;
    uint32_t i;

    /* steps through all control pointers */
    for(i=0;i<nt_kernel_data.controls_cnt;i++)
    {
      control = nt_kernel_data.controls[i];

      switch (option) {
        case NT_SYSTEM_CONTROL_INIT:
          nt_kernel_data.controls[i] = _nt_control_init(nt_kernel_data.rom->controls[i]);
          if (nt_kernel_data.controls[i]== NULL) {
              return NT_OUT_OF_MEMORY; /* failure stops the entire init phase */
          }
          break;
        case NT_SYSTEM_CONTROL_OVERRUN:
          if (_nt_control_overrun(control) < NT_SUCCESS) {
              result = NT_FAILURE; /* overrun error, triger others anyway */
          }
          break;
        case NT_SYSTEM_CONTROL_PROCESS:
         {
            const struct nt_control_interface *interface = (const struct nt_control_interface *)
                                                       control->rom->interface;
            if(interface->process != NULL)
            {
              if (interface->process(control) < NT_SUCCESS)
              {
                result = NT_FAILURE; /* data not ready */
              }
            }
         }
          break;
        case NT_SYSTEM_CONTROL_DATA_READY:
          _nt_control_set_flag(control, NT_CONTROL_NEW_DATA_FLAG);
          break;
        default:
          break;
      }
    }
    return result;
}

void _nt_system_invoke_callback(uint32_t event)
{
  struct nt_kernel *system = _nt_system_get();

  if (system->callback == NULL)
  {
    return;
  }

  switch (event)
  {
    case NT_SYSTEM_EVENT_OVERRUN:
        system->callback(NT_SYSTEM_EVENT_OVERRUN);
        break;
    case NT_SYSTEM_EVENT_DATA_READY:
        system->callback(NT_SYSTEM_EVENT_DATA_READY);
        break;
    case NT_SYSTEM_EVENT_DATA_OVERFLOW:
        system->callback(NT_SYSTEM_EVENT_DATA_OVERFLOW);
        break;
    default:
        break;
  }
}

void nt_system_register_callback(nt_system_callback callback)
{
    nt_kernel_data.callback = callback;
}

void nt_error_register_callback(nt_error_callback callback)
{
  #if (NT_DEBUG != 0)  
    nt_kernel_data.error_callback = callback;
  #else
    (void)callback;
  #endif
}

void _nt_system_modules_data_ready(void)
{
    if (_nt_system_module_function(NT_SYSTEM_MODULE_CHECK_DATA) == NT_SUCCESS) {
        /* all modules have been processed, set data ready for all controls */
        _nt_system_control_function(NT_SYSTEM_CONTROL_DATA_READY);
        _nt_system_invoke_callback(NT_SYSTEM_EVENT_DATA_READY);
    }
}

const struct nt_module *_nt_system_get_module(uint32_t interface_address,
                                             uint32_t instance)
{
    const struct nt_module * const * module = nt_kernel_data.rom->modules;

    while (*module) {
        if ((uint32_t)((*module)->interface) == interface_address) {
            if (((*module)->instance) == instance) {
                return *module;
            }
        }
        module++;
    }

    return NULL;
}
