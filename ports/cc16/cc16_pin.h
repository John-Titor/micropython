// Pin primitives
//

#ifndef MICROPY_INCLUDED_CC16_PIN_H
#define MICROPY_INCLUDED_CC16_PIN_H

#include <stdbool.h>
#include <stdint.h>

enum {
    Pin_PortA,
    Pin_PortB,
    Pin_PortC,
    Pin_PortD,
    Pin_PortE,
    Pin_PortX
};
enum {
    Pin_Analog,
    Pin_GPIO,
    Pin_Function2,
    Pin_Function3,
    Pin_Function4,
    Pin_Function5,
    Pin_Function6,
    Pin_Function7,
};
enum {
    Pin_IN,
    Pin_OUT
};

typedef struct
{
    const uint32_t  port:3;
    const uint32_t  index:5;
    const uint32_t  mux:3;
    const uint32_t  direction:1;
    const uint32_t  initial:1;
    const uint32_t  adc:2;
    const uint32_t  ftm:3;
    const uint32_t  channel:4;
} Pin_t;

extern void     cc16_pin_init(void);
extern void     cc16_pin_configure(Pin_t pin);
extern void     cc16_pin_set(Pin_t pin, bool v);
extern bool     cc16_pin_get(Pin_t pin);
extern void     cc16_pin_toggle(Pin_t pin);

//
// Pins on the port expander.
//
#define DCDC_8V5        (Pin_t){ .port = Pin_PortX, .index = 0 }
#define DCDC_10V        (Pin_t){ .port = Pin_PortX, .index = 1 }
#define PD_A_IN5        (Pin_t){ .port = Pin_PortX, .index = 2 }
#define PD_A_IN4        (Pin_t){ .port = Pin_PortX, .index = 3 }
#define PD_A_IN3        (Pin_t){ .port = Pin_PortX, .index = 4 }
#define PD_A_IN2        (Pin_t){ .port = Pin_PortX, .index = 5 }
#define PD_A_IN1        (Pin_t){ .port = Pin_PortX, .index = 6 }
#define PD_A_IN0        (Pin_t){ .port = Pin_PortX, .index = 7 }
#define PU_A_IN5        (Pin_t){ .port = Pin_PortX, .index = 8 }
#define PU_A_IN4        (Pin_t){ .port = Pin_PortX, .index = 9 }
#define PU_A_IN3        (Pin_t){ .port = Pin_PortX, .index = 10 }
#define PU_A_IN2        (Pin_t){ .port = Pin_PortX, .index = 11 }
#define PU_A_IN1        (Pin_t){ .port = Pin_PortX, .index = 12 }
#define PU_A_IN0        (Pin_t){ .port = Pin_PortX, .index = 13 }

//
// Internal GPIO pins.
//
#define DI_KL15         (Pin_t){ .port = Pin_PortD, .index = 13, .mux = Pin_GPIO, .direction = Pin_IN }
#define DI_PGD          (Pin_t){ .port = Pin_PortA, .index = 17, .mux = Pin_GPIO, .direction = Pin_IN }
#define DO_CS_HSD1      (Pin_t){ .port = Pin_PortD, .index = 17, .mux = Pin_GPIO, .direction = Pin_OUT, .initial = 1 }
#define DO_CS_HSD2      (Pin_t){ .port = Pin_PortD, .index = 11, .mux = Pin_GPIO, .direction = Pin_OUT, .initial = 1 }
#define DO_POWER        (Pin_t){ .port = Pin_PortD, .index = 15, .mux = Pin_GPIO, .direction = Pin_OUT, .initial = 1 }
#define DO_RS0          (Pin_t){ .port = Pin_PortC, .index = 9,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_RS1          (Pin_t){ .port = Pin_PortE, .index = 7,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_RS2          (Pin_t){ .port = Pin_PortB, .index = 8,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_RS3          (Pin_t){ .port = Pin_PortC, .index = 8,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_RS4          (Pin_t){ .port = Pin_PortB, .index = 11, .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_RS5          (Pin_t){ .port = Pin_PortB, .index = 17, .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_SHIFT_IN_DS  (Pin_t){ .port = Pin_PortD, .index = 6,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_SHIFT_MR     (Pin_t){ .port = Pin_PortE, .index = 16, .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_SHIFT_OE     (Pin_t){ .port = Pin_PortE, .index = 15, .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_SHIFT_SH_CP  (Pin_t){ .port = Pin_PortD, .index = 0,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_SHIFT_ST_CP  (Pin_t){ .port = Pin_PortD, .index = 1,  .mux = Pin_GPIO, .direction = Pin_OUT }
#define DO_VREF_EN      (Pin_t){ .port = Pin_PortD, .index = 10, .mux = Pin_GPIO, .direction = Pin_OUT }
#define LIN_EN          (Pin_t){ .port = Pin_PortE, .index = 9,  .mux = Pin_GPIO, .direction = Pin_OUT }

//
// Peripheral and unused pins not requiring other setup.
//
#define MC_CAN_RXD1     (Pin_t){ .port = Pin_PortE, .index = 4,  .mux = Pin_Function5 }
#define MC_CAN_RXD2     (Pin_t){ .port = Pin_PortA, .index = 12, .mux = Pin_Function3 }
#define MC_CAN_TXD1     (Pin_t){ .port = Pin_PortE, .index = 5,  .mux = Pin_Function5 }
#define MC_CAN_TXD2     (Pin_t){ .port = Pin_PortA, .index = 13, .mux = Pin_Function3 }
#define MC_SCI_RXD      (Pin_t){ .port = Pin_PortA, .index = 8,  .mux = Pin_Function2 }
#define MC_SCI_TXD      (Pin_t){ .port = Pin_PortA, .index = 9,  .mux = Pin_Function2 }
#define Pin74           (Pin_t){ .port = Pin_PortE, .index = 2,  .mux = Pin_Analog }
#define Pin82           (Pin_t){ .port = Pin_PortE, .index = 10, .mux = Pin_Analog }
#define WD              (Pin_t){ .port = Pin_PortE, .index = 11, .mux = Pin_Analog }

//
// Internal analog pins.
//
#define DI_AI_INA_OUT0  (Pin_t){ .port = Pin_PortC, .index = 16, .mux = Pin_Analog, .adc = 0, .channel = 14 }
#define DI_AI_INA_OUT1  (Pin_t){ .port = Pin_PortC, .index = 17, .mux = Pin_Analog, .adc = 0, .channel = 15 }
#define DI_AI_INA_OUT2  (Pin_t){ .port = Pin_PortC, .index = 1,  .mux = Pin_Analog, .adc = 0, .channel =  9 }
#define DI_AI_INA_OUT3  (Pin_t){ .port = Pin_PortC, .index = 0,  .mux = Pin_Analog, .adc = 0, .channel =  8 }
#define DI_AI_INA_OUT4  (Pin_t){ .port = Pin_PortB, .index = 2,  .mux = Pin_Analog, .adc = 0, .channel =  6 }
#define DI_AI_INA_OUT5  (Pin_t){ .port = Pin_PortC, .index = 14, .mux = Pin_Analog, .adc = 0, .channel = 12 }
#define DI_AI_INA_OUT6  (Pin_t){ .port = Pin_PortC, .index = 15, .mux = Pin_Analog, .adc = 0, .channel = 13 }
#define DI_AI_INA_OUT7  (Pin_t){ .port = Pin_PortB, .index = 3,  .mux = Pin_Analog, .adc = 0, .channel =  7 }
#define DI_AI_KL30_1    (Pin_t){ .port = Pin_PortC, .index = 2,  .mux = Pin_Analog, .adc = 0, .channel = 10 }
#define DI_AI_KL30_2    (Pin_t){ .port = Pin_PortC, .index = 3,  .mux = Pin_Analog, .adc = 0, .channel = 11 }
#define DI_AI_OUT0      (Pin_t){ .port = Pin_PortE, .index = 6,  .mux = Pin_Analog, .adc = 1, .channel = 11 }
#define DI_AI_OUT1      (Pin_t){ .port = Pin_PortA, .index = 0,  .mux = Pin_Analog, .adc = 0, .channel =  0 }
#define DI_AI_OUT2      (Pin_t){ .port = Pin_PortA, .index = 15, .mux = Pin_Analog, .adc = 1, .channel = 12 }
#define DI_AI_OUT3      (Pin_t){ .port = Pin_PortA, .index = 16, .mux = Pin_Analog, .adc = 1, .channel = 13 }
#define DI_AI_OUT4      (Pin_t){ .port = Pin_PortA, .index = 1,  .mux = Pin_Analog, .adc = 0, .channel =  1 }
#define DI_AI_OUT5      (Pin_t){ .port = Pin_PortD, .index = 2,  .mux = Pin_Analog, .adc = 1, .channel =  2 }
#define DI_AI_OUT6      (Pin_t){ .port = Pin_PortA, .index = 2,  .mux = Pin_Analog, .adc = 1, .channel =  0 }
#define DI_AI_OUT7      (Pin_t){ .port = Pin_PortA, .index = 3,  .mux = Pin_Analog, .adc = 1, .channel =  1 }
#define DI_AI_SNS1      (Pin_t){ .port = Pin_PortB, .index = 13, .mux = Pin_Analog, .adc = 1, .channel =  8 }
#define DI_AI_SNS2      (Pin_t){ .port = Pin_PortB, .index = 14, .mux = Pin_Analog, .adc = 1, .channel =  9 }
#define DI_AI_SNS3      (Pin_t){ .port = Pin_PortD, .index = 4,  .mux = Pin_Analog, .adc = 1, .channel =  6 }
#define DI_AI_SNS4      (Pin_t){ .port = Pin_PortB, .index = 12, .mux = Pin_Analog, .adc = 1, .channel =  7 }
#define DI_AI_VARIANTE  (Pin_t){ .port = Pin_PortC, .index = 6,  .mux = Pin_Analog, .adc = 1, .channel =  4 }
#define DI_AI_VREF      (Pin_t){ .port = Pin_PortD, .index = 3,  .mux = Pin_Analog, .adc = 1, .channel =  3 }

//
// Peripheral pins on hold waiting for function implementation.
//
#define MC_FREQ_A_IN0   (Pin_t){ .port = Pin_PortC, .index = 10, .mux = Pin_Function2 }
#define MC_FREQ_A_IN1   (Pin_t){ .port = Pin_PortC, .index = 11, .mux = Pin_Function2 }
#define MC_FREQ_A_IN2   (Pin_t){ .port = Pin_PortC, .index = 12, .mux = Pin_Function2 }
#define MC_FREQ_A_IN3   (Pin_t){ .port = Pin_PortC, .index = 13, .mux = Pin_Function2 }
#define MC_FREQ_A_IN4   (Pin_t){ .port = Pin_PortB, .index = 9,  .mux = Pin_Function2 }
#define MC_FREQ_A_IN5   (Pin_t){ .port = Pin_PortB, .index = 10, .mux = Pin_Function2 }

//
// External input pins.
//
#define DI_AI_A_IN0     (Pin_t){ .port = Pin_PortB, .index = 1,  .mux = Pin_Analog }
#define DI_AI_A_IN1     (Pin_t){ .port = Pin_PortB, .index = 0,  .mux = Pin_Analog }
#define DI_AI_A_IN2     (Pin_t){ .port = Pin_PortA, .index = 7,  .mux = Pin_Analog }
#define DI_AI_A_IN3     (Pin_t){ .port = Pin_PortA, .index = 6,  .mux = Pin_Analog }
#define DI_AI_A_IN4     (Pin_t){ .port = Pin_PortB, .index = 16, .mux = Pin_Analog }
#define DI_AI_A_IN5     (Pin_t){ .port = Pin_PortB, .index = 15, .mux = Pin_Analog }
#define DI_AI_ID        (Pin_t){ .port = Pin_PortC, .index = 7,  .mux = Pin_Analog }
#define DI_INTERFACE2_A (Pin_t){ .port = Pin_PortD, .index = 7, .mux = Pin_GPIO, .direction = Pin_IN }
#define DI_INTERFACE2_B (Pin_t){ .port = Pin_PortD, .index = 8, .mux = Pin_GPIO, .direction = Pin_IN }

//
// High-side driver output pins.
//
// TODO: these need PWM support
//
#define DO_HSD1_OUT0    (Pin_t){ .port = Pin_PortD, .index = 14, .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 2, .channel = 5 }
#define DO_HSD1_OUT1    (Pin_t){ .port = Pin_PortB, .index = 4,  .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 0, .channel = 4 }
#define DO_HSD1_OUT2    (Pin_t){ .port = Pin_PortE, .index = 8,  .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 0, .channel = 6 }
#define DO_HSD1_OUT3    (Pin_t){ .port = Pin_PortB, .index = 5,  .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 0, .channel = 5 }
#define DO_HSD2_OUT4    (Pin_t){ .port = Pin_PortD, .index = 5,  .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 2, .channel = 3 }
#define DO_HSD2_OUT5    (Pin_t){ .port = Pin_PortD, .index = 12, .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 2, .channel = 2 }
#define DO_HSD2_OUT6    (Pin_t){ .port = Pin_PortD, .index = 9,  .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 1, .channel = 5 }
#define DO_HSD2_OUT7    (Pin_t){ .port = Pin_PortD, .index = 16, .mux = Pin_GPIO, .direction = Pin_OUT, .ftm = 0, .channel = 1 }

#endif // MICROPY_INCLUDED_CC16_PIN_H