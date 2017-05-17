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
#include "nt_controls.h"
#include "../source/controls/nt_controls_prv.h"
#include "nt_control_proxi.h"
#include "nt_control_proxi_prv.h"
#include "../source/electrodes/nt_electrodes_prv.h"
#include "../source/filters/nt_filters_prv.h"

/* Invoke callback based on the even has occured */
static void _nt_control_proxi_invoke_callback(struct nt_control_data *control,
                                              enum nt_control_proxi_event event,
                                              uint32_t index, uint32_t proximity);

/* The proxi control initialization function. */
static int32_t _nt_control_proxi_init(struct nt_control_data *control);
/* The proxi control process function. */
static int32_t _nt_control_proxi_process(struct nt_control_data *control);

/* The proxi control interface. */
const struct nt_control_interface nt_control_proxi_interface = {
    .init    = _nt_control_proxi_init,
    .process = _nt_control_proxi_process,
    .name    = "nt_control_proxi_interface",
};

void nt_control_proxi_register_callback(const struct nt_control *control,
                                        nt_control_proxi_callback callback)
{
    NT_ASSERT(control != NULL);
    NT_ASSERT(control->interface == &nt_control_proxi_interface);
    
    struct nt_control_data *control_data = _nt_control_get_data(control);
    NT_ASSERT(control_data != NULL);
    
    struct nt_control_proxi_data *ram = control_data->data.proxi;
    NT_ASSERT(ram != NULL);
    
    ram->callback = callback;
}

static void _nt_control_proxi_invoke_callback(struct nt_control_data *control,
                                              enum nt_control_proxi_event event, 
                                              uint32_t index, uint32_t proximity)
{
    struct nt_control_proxi_data *ram = control->data.proxi;
    NT_ASSERT(ram != NULL);
    
    if (ram->callback != NULL) 
    {
        ram->callback(control->rom, event, index, ram->proximity);
    }
}

static int32_t _nt_control_proxi_init(struct nt_control_data *control)
{
    NT_ASSERT(control != NULL);
    NT_ASSERT(control->rom->interface == &nt_control_proxi_interface);
    NT_ASSERT(control->rom->control_params.proxi != NULL);
    
    const struct nt_control_proxi *proxi = control->rom->control_params.proxi;
    
    if(proxi->range == 0)
    {
        return NT_FAILURE;
    }
    
    if(proxi->threshold == 0)
    {
        return NT_FAILURE;
    }
    
    control->data.proxi = _nt_mem_alloc(sizeof(struct nt_control_proxi_data));
    
    if(control->data.proxi == NULL)
    {
        return NT_OUT_OF_MEMORY;
    }
    
    if (_nt_control_check_data(control) != NT_SUCCESS) {
        return NT_FAILURE;
    }
    
    return NT_SUCCESS;
}

static int32_t _nt_control_proxi_get_data(struct nt_control_data *control,
                                          struct nt_control_proxi_temp_data 
                                              *temp_data)
{
    uint32_t elec_counter = control->electrodes_size; 
    int32_t delta;
    int32_t delta_max = 0;
    uint32_t max_delta_elec = 0;
    
    /* finding the electrode with maximum delta in the control */
    while (elec_counter--) 
    {
        delta = _nt_electrode_get_delta(control->electrodes[elec_counter]);
        
        if (delta > delta_max) 
        {
            max_delta_elec = elec_counter; /* Store the max delta electrode */
            delta_max = delta;
        }            
    }
    /* Store baseline value */ 
    temp_data->range = control->electrodes[max_delta_elec]->baseline; 
    temp_data->active_el_ix = max_delta_elec;
    temp_data->max_delta = delta_max;
    
    return NT_SUCCESS;
}

static uint32_t _nt_control_proxi_calculate_proximity(const struct nt_control_data *control,
                                                      struct nt_control_proxi_temp_data *temp_data)
{
    NT_ASSERT(control->rom->interface == &nt_control_proxi_interface);
    
    const struct nt_control_proxi *proxi_desc = control->rom->control_params.proxi;
    uint32_t temp_range = proxi_desc->range;    
    uint32_t real_prox = 0;
    
    /* check the denominator and calculate */
    if(temp_range != 0)
    { 
        real_prox = (temp_data->max_delta * proxi_desc->scale) / temp_range; 
    }
    
    if(real_prox > proxi_desc->scale) 
    {
        real_prox = proxi_desc->scale;
    }
    
    return real_prox;
}

static int32_t _nt_control_proxi_process(struct nt_control_data *control)
{
    NT_ASSERT(control != NULL);
    NT_ASSERT(control->rom->interface == &nt_control_proxi_interface);
    
    struct nt_control_proxi_data *ram = control->data.proxi;
    const struct nt_control_proxi *rom = control->rom->control_params.proxi;
    struct nt_control_proxi_temp_data computing_data;
    
    NT_ASSERT(ram != NULL);
    
    if (!_nt_control_get_flag(control, NT_CONTROL_EN_FLAG) ||
        !_nt_control_get_flag(control, NT_CONTROL_NEW_DATA_FLAG)) {
            return NT_FAILURE; /* control disabled or data not ready */
        }
    
    if (_nt_control_get_electrodes_state(control))
    {
        return NT_SUCCESS;  /* valid analog touch on the control's electrodes */
    }
    
    if (_nt_control_get_electrodes_digital_state(control)) 
    {
        return NT_SUCCESS; /* valid digital touch on the control's electrodes */
    }
    
    /* Get Proximity data */
    if (_nt_control_proxi_get_data(control, &computing_data) == NT_SUCCESS) 
    {
        uint32_t current_prox = _nt_control_proxi_calculate_proximity(control,
                                                                      &computing_data);
        /* If touch, check change and direction */
        if(_nt_control_get_flag(control, NT_PROXI_TOUCH_FLAG)) 
        {    
            if (ram->proximity != current_prox)
            {
                if(_nt_abs_int32(ram->proximity - current_prox) > rom->insensitivity)
                {
                    if (ram->proximity < current_prox) 
                    {
                        _nt_control_set_flag(control, NT_PROXI_DIRECTION_FLAG);
                    } 
                    else 
                    {
                        _nt_control_clear_flag(control, NT_PROXI_DIRECTION_FLAG);
                    }
                    
                    ram->proximity = current_prox;
                    ram->index = computing_data.active_el_ix;
                    _nt_control_set_flag(control, NT_PROXI_MOVEMENT_FLAG);
                    _nt_control_proxi_invoke_callback(control, NT_PROXI_MOVEMENT,
                                                      ram->index,
                                                      ram->proximity);
                }  
                else
                {
                    _nt_control_clear_flag(control, NT_PROXI_MOVEMENT_FLAG);    
                }
            }
        }
        
        ram->proximity = current_prox;
        ram->index = computing_data.active_el_ix;
        
        if (current_prox > rom->threshold)  /* Check Touch threshold */
        {
            if (!_nt_control_get_flag(control, NT_PROXI_TOUCH_FLAG)) 
            {
                _nt_control_set_flag(control, NT_PROXI_TOUCH_FLAG);
                _nt_control_clear_flag(control, NT_PROXI_RELEASE_FLAG);
                _nt_control_proxi_invoke_callback(control, NT_PROXI_TOUCH,
                                                  ram->index,
                                                  ram->proximity);
            }  
        } 
        else 
        {
            _nt_control_clear_flag(control, NT_PROXI_TOUCH_FLAG);   /* released */
            _nt_control_clear_flag(control, NT_PROXI_MOVEMENT_FLAG);/* clear flag */ 
            /* Call just once, if already released, no more calls*/
            if (!_nt_control_get_flag(control, NT_PROXI_RELEASE_FLAG))
            { 
                _nt_control_proxi_invoke_callback(control, NT_PROXI_RELEASE,
                                                  ram->index,
                                                  ram->proximity);
                
                _nt_control_set_flag(control, NT_PROXI_RELEASE_FLAG);
            }     
        }
    } 
    _nt_control_clear_flag(control, NT_CONTROL_NEW_DATA_FLAG); /* data processed */
    return NT_SUCCESS;
}
