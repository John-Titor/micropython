// Pin primitives
//

#include <assert.h>

#include "cc16_pin.h"
#include "s32k144.h"

static uint16_t             _portX_state;
static void                 _portX_update(void);
static volatile PT_regs_t   *_pin_io(Pin_t pin);

void
cc16_pin_init()
{
    cc16_pin_configure(DCDC_8V5);
    cc16_pin_configure(DCDC_10V);
    cc16_pin_configure(PD_A_IN5);
    cc16_pin_configure(PD_A_IN4);
    cc16_pin_configure(PD_A_IN3);
    cc16_pin_configure(PD_A_IN2);
    cc16_pin_configure(PD_A_IN1);
    cc16_pin_configure(PD_A_IN0);
    cc16_pin_configure(PU_A_IN5);
    cc16_pin_configure(PU_A_IN4);
    cc16_pin_configure(PU_A_IN3);
    cc16_pin_configure(PU_A_IN2);
    cc16_pin_configure(PU_A_IN1);
    cc16_pin_configure(PU_A_IN0);
    cc16_pin_configure(DI_KL15);
    cc16_pin_configure(DI_PGD);
    cc16_pin_configure(DO_CS_HSD1);
    cc16_pin_configure(DO_CS_HSD2);
    cc16_pin_configure(DO_POWER);
    cc16_pin_configure(DO_RS0);
    cc16_pin_configure(DO_RS1);
    cc16_pin_configure(DO_RS2);
    cc16_pin_configure(DO_RS3);
    cc16_pin_configure(DO_RS4);
    cc16_pin_configure(DO_RS5);
    cc16_pin_configure(DO_SHIFT_IN_DS);
    cc16_pin_configure(DO_SHIFT_MR);
    cc16_pin_configure(DO_SHIFT_OE);
    cc16_pin_configure(DO_SHIFT_SH_CP);
    cc16_pin_configure(DO_SHIFT_ST_CP);
    cc16_pin_configure(DO_VREF_EN);
    cc16_pin_configure(LIN_EN);
    cc16_pin_configure(MC_CAN_RXD1);
    cc16_pin_configure(MC_CAN_RXD2);
    cc16_pin_configure(MC_CAN_TXD1);
    cc16_pin_configure(MC_CAN_TXD2);
    cc16_pin_configure(MC_SCI_RXD);
    cc16_pin_configure(MC_SCI_TXD);
    // cc16_pin_configure(Pin74);
    // cc16_pin_configure(Pin82);
    // cc16_pin_configure(WD);
    cc16_pin_configure(DI_AI_INA_OUT0);
    cc16_pin_configure(DI_AI_INA_OUT1);
    cc16_pin_configure(DI_AI_INA_OUT2);
    cc16_pin_configure(DI_AI_INA_OUT3);
    cc16_pin_configure(DI_AI_INA_OUT4);
    cc16_pin_configure(DI_AI_INA_OUT5);
    cc16_pin_configure(DI_AI_INA_OUT6);
    cc16_pin_configure(DI_AI_INA_OUT7);
    cc16_pin_configure(DI_AI_KL30_1);
    cc16_pin_configure(DI_AI_KL30_2);
    cc16_pin_configure(DI_AI_OUT0);
    cc16_pin_configure(DI_AI_OUT1);
    cc16_pin_configure(DI_AI_OUT2);
    cc16_pin_configure(DI_AI_OUT3);
    cc16_pin_configure(DI_AI_OUT4);
    cc16_pin_configure(DI_AI_OUT5);
    cc16_pin_configure(DI_AI_OUT6);
    cc16_pin_configure(DI_AI_OUT7);
    cc16_pin_configure(DI_AI_SNS1);
    cc16_pin_configure(DI_AI_SNS2);
    cc16_pin_configure(DI_AI_SNS3);
    cc16_pin_configure(DI_AI_SNS4);
    cc16_pin_configure(DI_AI_VARIANTE);
    cc16_pin_configure(DI_AI_VREF);
    cc16_pin_configure(MC_FREQ_A_IN0);
    cc16_pin_configure(MC_FREQ_A_IN1);
    cc16_pin_configure(MC_FREQ_A_IN2);
    cc16_pin_configure(MC_FREQ_A_IN3);
    cc16_pin_configure(MC_FREQ_A_IN4);
    cc16_pin_configure(MC_FREQ_A_IN5);
    cc16_pin_configure(DI_AI_A_IN0);
    cc16_pin_configure(DI_AI_A_IN1);
    cc16_pin_configure(DI_AI_A_IN2);
    cc16_pin_configure(DI_AI_A_IN3);
    cc16_pin_configure(DI_AI_A_IN4);
    cc16_pin_configure(DI_AI_A_IN5);
    cc16_pin_configure(DI_AI_ID);
    cc16_pin_configure(DI_INTERFACE2_A);
    cc16_pin_configure(DI_INTERFACE2_B);
    cc16_pin_configure(DO_HSD1_OUT0);
    cc16_pin_configure(DO_HSD1_OUT1);
    cc16_pin_configure(DO_HSD1_OUT2);
    cc16_pin_configure(DO_HSD1_OUT3);
    cc16_pin_configure(DO_HSD2_OUT4);
    cc16_pin_configure(DO_HSD2_OUT5);
    cc16_pin_configure(DO_HSD2_OUT6);
    cc16_pin_configure(DO_HSD2_OUT7);
}

void
cc16_pin_configure(Pin_t pin)
{
    assert(pin.port <= Pin_PortX);

    if (pin.port <= Pin_PortE) {
        assert(pin.index < 18);
        assert(pin.mux < Pin_Function7);
        assert((pin.direction == Pin_OUT) != (mux != Pin_GPIO));
        assert((pin.initial == 1) != (pin.mux != Pin_GPIO));

        // port clock on
        volatile uint32_t *pccr = pin.port == Pin_PortA ? &PCC->PCC_PORTA :
                                  pin.port == Pin_PortB ? &PCC->PCC_PORTB :
                                  pin.port == Pin_PortC ? &PCC->PCC_PORTC :
                                  pin.port == Pin_PortD ? &PCC->PCC_PORTD :
                                  &PCC->PCC_PORTE;
        *pccr = PCC_PCC_PORTA_CGC;

        // select function
        uint32_t mask = (uint32_t)1 << pin.index;
        volatile PORT_regs_t *cfg = pin.port == Pin_PortA ? PORTA :
                                    pin.port == Pin_PortB ? PORTB :
                                    pin.port == Pin_PortC ? PORTC :
                                    pin.port == Pin_PortD ? PORTD :
                                    PORTE;
        if (pin.index < 16) {
            cfg->GPCLR = PORT_GPCLR_GPWE(mask) | PORT_GPCLR_GPWD(pin.mux << 8);
        } else {
            cfg->GPCHR = PORT_GPCHR_GPWE(mask >> 16) | PORT_GPCLR_GPWD(pin.mux << 8);
        }

        // select GPIO direction
        if (pin.mux == Pin_GPIO) {
            volatile PT_regs_t *io = _pin_io(pin);
            if (pin.direction == Pin_OUT) {
                io->PDDR |= mask;
                cc16_pin_set(pin, pin.initial);
            } else {
                io->PDDR &= ~mask;
            }
        }
    } else if (pin.port == Pin_PortX) {
        assert(pin.index < 13);

        cc16_pin_set(pin, pin.initial);
    }
}

void
cc16_pin_set(Pin_t pin, bool v)
{
    assert(pin.port <= Pin_PortX);

    if (pin.port <= Pin_PortE) {
        assert(pin.index < 18);
        assert((pin.mux == Pin_GPIO));
        assert(pin.direction == Pin_OUT);

        uint32_t mask = (uint32_t)1 << pin.index;
        volatile PT_regs_t *io = _pin_io(pin);
        if (v) {
            io->PSOR = mask;
        } else {
            io->PCOR = mask;
        }
    } else if (pin.port == Pin_PortX) {
        assert(index < 13);

        if (v) {
            _portX_state |= (uint16_t)1 << pin.index;
        } else {
            _portX_state &= ~(uint16_t)1 << pin.index;
        }
        _portX_update();
    }
}

bool
cc16_pin_get(Pin_t pin)
{
    assert(pin.port <= Pin_PortE);

    if (pin.port <= Pin_PortE) {
        assert(pin.index < 18);
        assert((pin.mux == Pin_GPIO) || (pin.mux == Pin_Analog));
        assert(pin.direction == Pin_IN);

        uint32_t mask = (uint32_t)1 << pin.index;
        volatile PT_regs_t *io = _pin_io(pin);
        return io->PDIR & mask;
    }
    return false;
}

void
cc16_pin_toggle(Pin_t pin)
{
    if (pin.port <= Pin_PortE) {
        uint32_t mask = (uint32_t)1 << pin.index;
        volatile PT_regs_t *io = _pin_io(pin);
        io->PTOR = mask;
    }
}

static void
_portX_update(void)
{
    // ensure register is not in reset
    cc16_pin_set(DO_SHIFT_MR, true);

    // XXX need to avoid re-entering here - disable interrupts?
    // drop latch pin while shifting
    cc16_pin_set(DO_SHIFT_ST_CP, false);

    for (unsigned i = 0; i < 14; i++) {
        // clock low
        cc16_pin_set(DO_SHIFT_SH_CP, false);
        // load data
        cc16_pin_set(DO_SHIFT_IN_DS, _portX_state & (1U << i));
        // clock data on rising edge
        cc16_pin_set(DO_SHIFT_SH_CP, true);
    }
    // latch on rising edge
    cc16_pin_set(DO_SHIFT_ST_CP, true);

    // ensure outputs are on - should not be neccessary
    cc16_pin_set(DO_SHIFT_OE, false);
}

static volatile PT_regs_t *
_pin_io(Pin_t pin)
{
    return pin.port == Pin_PortA ? PTA :
           pin.port == Pin_PortB ? PTB :
           pin.port == Pin_PortC ? PTC :
           pin.port == Pin_PortD ? PTD :
           PTE;
}
