
#ifndef MICROPY_INCLUDED_CC16_H
#define MICROPY_INCLUDED_CC16_H

#include "py/runtime.h"

#include "cc16_pin.h"
#include "s32k144.h"

extern void s32k_can_configure(void);

extern void cc16_input_configure(void);
extern void cc16_output_configure(void);
extern void cc16_vref_configure(void);
extern void cc16_adc_configure(void);
extern uint32_t cc16_adc_sample(Pin_t pin);
extern void cc16_ftm_configure(void);
extern void cc16_ftm_set_pwm(Pin_t pin, uint32_t value);

extern const mp_obj_type_t cc16_output_type;
extern const mp_obj_type_t cc16_input_type;
extern const mp_obj_type_t cc16_vref_type;
extern const mp_obj_type_t cc16_can_type;

typedef struct {
    uint8_t adc : 1;
    uint8_t channel : 4;
} ADC_channel_t;

#endif // MICROPY_INCLUDED_CC16_H
