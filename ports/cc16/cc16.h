
#ifndef MICROPY_INCLUDED_CC16_H
#define MICROPY_INCLUDED_CC16_H

#include "s32k144.h"

extern void main(void);
extern void cc16_init(void);
extern void s32k_can_early_init(void);

extern const mp_obj_type_t cc16_hsd_type;
extern const mp_obj_type_t cc16_input_type;

typedef struct {
	uint8_t		adc:1;
	uint8_t		channel:4;
} ADC_channel_t;

#endif // MICROPY_INCLUDED_CC16_H