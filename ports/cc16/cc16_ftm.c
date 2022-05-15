// Basic FTM support
//

#include "cc16.h"

static void _ftm_configure_pwm(volatile FTM_regs_t *ftm) {

    // disable the FTM first
    ftm->SC = 0;
    ftm->CNT = 0;
    ftm->MOD = 499;     // 8 MHz / 128 / 500 = 125Hz

    // configure all channels for edge-aligned PWM
    ftm->C0SC = FTM_C0SC_MSB | FTM_C0SC_ELSB;
    ftm->C0V = 0;
    ftm->C1SC = FTM_C1SC_MSB | FTM_C1SC_ELSB;
    ftm->C1V = 0;
    ftm->C2SC = FTM_C2SC_MSB | FTM_C2SC_ELSB;
    ftm->C2V = 0;
    ftm->C3SC = FTM_C3SC_MSB | FTM_C3SC_ELSB;
    ftm->C3V = 0;
    ftm->C4SC = FTM_C4SC_MSB | FTM_C4SC_ELSB;
    ftm->C4V = 0;
    ftm->C5SC = FTM_C5SC_MSB | FTM_C5SC_ELSB;
    ftm->C5V = 0;
    ftm->C6SC = FTM_C6SC_MSB | FTM_C6SC_ELSB;
    ftm->C6V = 0;
    ftm->C7SC = FTM_C7SC_MSB | FTM_C7SC_ELSB;
    ftm->C7V = 0;

    // select PCC clock, /128, PWM enabled on all outputs
    ftm->SC = FTM_SC_PS(0x7) |
              FTM_SC_CLKS(0x3) |
              FTM_SC_PWMEN0 |
              FTM_SC_PWMEN1 |
              FTM_SC_PWMEN2 |
              FTM_SC_PWMEN3 |
              FTM_SC_PWMEN4 |
              FTM_SC_PWMEN5 |
              FTM_SC_PWMEN6 |
              FTM_SC_PWMEN7;
}

void cc16_ftm_configure(void) {

    // turn on clock, select SOSCDIV1_CLK (8MHz)
    PCC->PCC_FTM0 |= PCC_PCC_FTM0_PCS(1) | PCC_PCC_FTM0_CGC;
    PCC->PCC_FTM1 |= PCC_PCC_FTM1_PCS(1) | PCC_PCC_FTM1_CGC;
    PCC->PCC_FTM2 |= PCC_PCC_FTM2_PCS(1) | PCC_PCC_FTM2_CGC;

    // configure FTMs
    _ftm_configure_pwm(FTM0);
    _ftm_configure_pwm(FTM1);
    _ftm_configure_pwm(FTM2);
}

void cc16_ftm_set_pwm(Pin_t pin, uint32_t value) {

    // analog / GPIO pins are definitely not interesting
    if ((pin.mux == Pin_Analog) || (pin.mux == Pin_GPIO)) {
        return;
    }

    // which FTM is this pin associated with?
    volatile FTM_regs_t *regs = ((pin.ftm == 0) ? FTM0 :
                                 (pin.ftm == 1) ? FTM1 : 
                                 (pin.ftm == 2) ? FTM2 : 0);
    if (regs == 0) {
        return;
    }

    // which channel value register?
    volatile uint32_t *v = ((pin.channel == 0) ? &regs->C0V :
                            (pin.channel == 1) ? &regs->C1V :
                            (pin.channel == 2) ? &regs->C2V :
                            (pin.channel == 3) ? &regs->C3V :
                            (pin.channel == 4) ? &regs->C4V :
                            (pin.channel == 5) ? &regs->C5V :
                            (pin.channel == 6) ? &regs->C6V :
                            (pin.channel == 7) ? &regs->C7V : 0);
    if (v == 0) {
        return;
    }
    if (value > 100) {
        value = 100;
    }
    *v = value * 5;        // scale 0-500
}
