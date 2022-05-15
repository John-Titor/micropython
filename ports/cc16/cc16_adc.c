
#include "cc16.h"

static void _adc_configure(volatile ADC_regs_t *adc) {

    // configure for /1 internal clock, 12bit output
    adc->CFG1 = ADC_CFG1_ADIV(0) | ADC_CFG1_MODE(1) | ADC_CFG1_ADICLK(0);

    // 128 clocks sampling time
    adc->CFG2 = ADC_CFG2_SMPLTS(127);

    // default reference, software-triggered conversion, 4-sample averaging
    adc->SC2 = 0;
    adc->SC3 = ADC_SC3_AVGS(0) | ADC_SC3_AVGE;

}

void cc16_adc_configure(void) {

    // turn on clock, select SOSCDIV2_CLK (8MHz)
    PCC->PCC_ADC0 |= PCC_PCC_ADC0_PCS(1) | PCC_PCC_ADC0_CGC;
    PCC->PCC_ADC1 |= PCC_PCC_ADC1_PCS(1) | PCC_PCC_ADC1_CGC;

    // configure ADCs
    _adc_configure(ADC0);
    _adc_configure(ADC1);
}

uint32_t cc16_adc_sample(Pin_t pin) {

    if (pin.mux != Pin_Analog) {
        return 0;
    }

    volatile ADC_regs_t  *regs = pin.adc ? ADC1 : ADC0;
    regs->SC1A = ADC_SC1A_ADCH(pin.channel);
    while (!(regs->SC1A & ADC_SC1A_COCO)) {
    }
    return ADC_RA_D_EXTRACT(regs->RA);
}
