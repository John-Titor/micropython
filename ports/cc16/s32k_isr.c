
#include <stdint.h>
#include <string.h>

#include "py/mphal.h"
#include "cc16.h"

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;
volatile uint32_t systick_ms;

static void
Unhandled_Exception(void)
{
    mp_hal_stdout_tx_strn("\nUE!", 4);
    for (;;) {}
}

void Reset_Handler(void);
void NonMaskableInt_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void HardFault_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void MemoryManagement_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void BusFault_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void UsageFault_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void SVCall_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void DebugMonitor_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void PendSV_Handler(void) __attribute__((weak, alias("Unhandled_Exception")));
void SysTick_Handler(void);

static const
__attribute__((used, section(".exception_vectors")))
uint32_t _exception_vectors[] =
{
    (uint32_t)&_estack,
    (uint32_t)Reset_Handler,              // -15
    (uint32_t)NonMaskableInt_Handler,     // -14
    (uint32_t)HardFault_Handler,          // -13
    (uint32_t)MemoryManagement_Handler,   // -12
    (uint32_t)BusFault_Handler,           // -11
    (uint32_t)UsageFault_Handler,         // -10
    (uint32_t)Unhandled_Exception,        // -9
    (uint32_t)Unhandled_Exception,        // -8
    (uint32_t)Unhandled_Exception,        // -7
    (uint32_t)Unhandled_Exception,        // -6
    (uint32_t)SVCall_Handler,             // -5 
    (uint32_t)DebugMonitor_Handler,       // -4 
    (uint32_t)Unhandled_Exception,        // -3
    (uint32_t)PendSV_Handler,             // -2 
    (uint32_t)SysTick_Handler,            // -1 
};

// Entrypoint from the bootloader.
void __attribute__((naked)) Reset_Handler(void) {

    // fix the stack pointer
    volatile register void *sp  __asm__("%sp");
    sp = &_estack;
    (void)sp;

    // point VTOR back at our vector table
    S32_SCB->VTOR = (uint32_t)&_exception_vectors;

    // SCB->CCR: enforce 8-aligned stack per EABI
    S32_SCB->CCR |= S32_SCB_CCR_STKALIGN;

    // Enable the VFP
    S32_SCB->CPACR = S32_SCB_CPACR_CP10(3) | S32_SCB_CPACR_CP10(3);

    // Copy .data section from flash to RAM.
    memcpy(&_sdata, &_sidata, (char *)&_edata - (char *)&_sdata);

    // Zero out .bss section.
    memset(&_sbss, 0, (char *)&_ebss - (char *)&_sbss);

    // enable interrupts
    __enable_irq();

    // Initialise the cpu and peripherals.
    cc16_init();

    // call the main application
    main();

    // This function must not return.
    for (;;) {
    }
}

void SysTick_Handler(void) {
    systick_ms += 1;
}

static void
Unhandled_Interrupt(void)
{
    mp_hal_stdout_tx_strn("\nUI!", 4);
    for (;;) {}
}

void DMA0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA2_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA4_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA5_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA6_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA7_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA8_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA9_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA10_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA11_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA12_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA13_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA14_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA15_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void DMA_Error_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void MCM_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTFC_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void Read_Collision_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LVD_LVW_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTFC_Fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void WDOG_EWM_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void RCM_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPI2C0_Master_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPI2C0_Slave_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPSPI0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPSPI1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPSPI2_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPUART0_RxTx_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPUART1_RxTx_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPUART2_RxTx_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void ADC0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void ADC1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CMP0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void ERM_single_fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void ERM_double_fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void RTC_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void RTC_Seconds_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPIT0_Ch0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPIT0_Ch1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPIT0_Ch2_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPIT0_Ch3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PDB0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void SCG_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void LPTMR0_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PORTA_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PORTB_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PORTC_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PORTD_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PORTE_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void SWI_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void PDB1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FLEXIO_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN0_ORed_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN0_Error_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN0_Wake_Up_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN0_ORed_0_15_MB_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN0_ORed_16_31_MB_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN1_ORed_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN1_Error_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN1_ORed_0_15_MB_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN2_ORed_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN2_Error_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void CAN2_ORed_0_15_MB_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Ch0_Ch1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Ch2_Ch3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Ch4_Ch5_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Ch6_Ch7_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM0_Ovf_Reload_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Ch0_Ch1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Ch2_Ch3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Ch4_Ch5_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Ch6_Ch7_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM1_Ovf_Reload_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Ch0_Ch1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Ch2_Ch3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Ch4_Ch5_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Ch6_Ch7_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM2_Ovf_Reload_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Ch0_Ch1_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Ch2_Ch3_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Ch4_Ch5_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Ch6_Ch7_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Fault_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));
void FTM3_Ovf_Reload_Handler(void) __attribute__((weak, alias("Unhandled_Interrupt")));

static const __attribute__((used, section(".interrupt_vectors")))
uint32_t _interrupt_vectors[] =
{
    (uint32_t)DMA0_Handler,               // 0   DMA0
    (uint32_t)DMA1_Handler,               // 1   DMA1
    (uint32_t)DMA2_Handler,               // 2   DMA2
    (uint32_t)DMA3_Handler,               // 3   DMA3
    (uint32_t)DMA4_Handler,               // 4   DMA4
    (uint32_t)DMA5_Handler,               // 5   DMA5
    (uint32_t)DMA6_Handler,               // 6   DMA6
    (uint32_t)DMA7_Handler,               // 7   DMA7
    (uint32_t)DMA8_Handler,               // 8   DMA8
    (uint32_t)DMA9_Handler,               // 9   DMA9
    (uint32_t)DMA10_Handler,              // 10  DMA10
    (uint32_t)DMA11_Handler,              // 11  DMA11
    (uint32_t)DMA12_Handler,              // 12  DMA12
    (uint32_t)DMA13_Handler,              // 13  DMA13
    (uint32_t)DMA14_Handler,              // 14  DMA14
    (uint32_t)DMA15_Handler,              // 15  DMA15
    (uint32_t)DMA_Error_Handler,          // 16  DMA_Error
    (uint32_t)MCM_Handler,                // 17  MCM
    (uint32_t)FTFC_Handler,               // 18  FTFC
    (uint32_t)Read_Collision_Handler,     // 19  Read_Collision
    (uint32_t)LVD_LVW_Handler,            // 20  LVD_LVW
    (uint32_t)FTFC_Fault_Handler,         // 21  FTFC_Fault
    (uint32_t)WDOG_EWM_Handler,           // 22  WDOG_EWM
    (uint32_t)RCM_Handler,                // 23  RCM
    (uint32_t)LPI2C0_Master_Handler,      // 24  LPI2C0_Master
    (uint32_t)LPI2C0_Slave_Handler,       // 25  LPI2C0_Slave
    (uint32_t)LPSPI0_Handler,             // 26  LPSPI0
    (uint32_t)LPSPI1_Handler,             // 27  LPSPI1
    (uint32_t)LPSPI2_Handler,             // 28  LPSPI2
    (uint32_t)Unhandled_Interrupt,        // 29
    (uint32_t)Unhandled_Interrupt,        // 30
    (uint32_t)LPUART0_RxTx_Handler,       // 31  LPUART0_RxTx
    (uint32_t)Unhandled_Interrupt,        // 32
    (uint32_t)LPUART1_RxTx_Handler,       // 33  LPUART1_RxTx
    (uint32_t)Unhandled_Interrupt,        // 34
    (uint32_t)LPUART2_RxTx_Handler,       // 35  LPUART2_RxTx
    (uint32_t)Unhandled_Interrupt,        // 36
    (uint32_t)Unhandled_Interrupt,        // 37
    (uint32_t)Unhandled_Interrupt,        // 38
    (uint32_t)ADC0_Handler,               // 39  ADC0
    (uint32_t)ADC1_Handler,               // 40  ADC1
    (uint32_t)CMP0_Handler,               // 41  CMP0
    (uint32_t)Unhandled_Interrupt,        // 42
    (uint32_t)Unhandled_Interrupt,        // 43
    (uint32_t)ERM_single_fault_Handler,   // 44  ERM_single_fault
    (uint32_t)ERM_double_fault_Handler,   // 45  ERM_double_fault
    (uint32_t)RTC_Handler,                // 46  RTC
    (uint32_t)RTC_Seconds_Handler,        // 47  RTC_Seconds
    (uint32_t)LPIT0_Ch0_Handler,          // 48  LPIT0_Ch0
    (uint32_t)LPIT0_Ch1_Handler,          // 49  LPIT0_Ch1
    (uint32_t)LPIT0_Ch2_Handler,          // 50  LPIT0_Ch2
    (uint32_t)LPIT0_Ch3_Handler,          // 51  LPIT0_Ch3
    (uint32_t)PDB0_Handler,               // 52  PDB0
    (uint32_t)Unhandled_Interrupt,        // 53
    (uint32_t)Unhandled_Interrupt,        // 54
    (uint32_t)Unhandled_Interrupt,        // 55
    (uint32_t)Unhandled_Interrupt,        // 56
    (uint32_t)SCG_Handler,                // 57  SCG
    (uint32_t)LPTMR0_Handler,             // 58  LPTMR0
    (uint32_t)PORTA_Handler,              // 59  PORTA
    (uint32_t)PORTB_Handler,              // 60  PORTB
    (uint32_t)PORTC_Handler,              // 61  PORTC
    (uint32_t)PORTD_Handler,              // 62  PORTD
    (uint32_t)PORTE_Handler,              // 63  PORTE
    (uint32_t)SWI_Handler,                // 64  SWI
    (uint32_t)Unhandled_Interrupt,        // 65
    (uint32_t)Unhandled_Interrupt,        // 66
    (uint32_t)Unhandled_Interrupt,        // 67
    (uint32_t)PDB1_Handler,               // 68  PDB1
    (uint32_t)FLEXIO_Handler,             // 69  FLEXIO
    (uint32_t)Unhandled_Interrupt,        // 70
    (uint32_t)Unhandled_Interrupt,        // 71
    (uint32_t)Unhandled_Interrupt,        // 72
    (uint32_t)Unhandled_Interrupt,        // 73
    (uint32_t)Unhandled_Interrupt,        // 74
    (uint32_t)Unhandled_Interrupt,        // 75
    (uint32_t)Unhandled_Interrupt,        // 76
    (uint32_t)Unhandled_Interrupt,        // 77
    (uint32_t)CAN0_ORed_Handler,          // 78  CAN0_ORed
    (uint32_t)CAN0_Error_Handler,         // 79  CAN0_Error
    (uint32_t)CAN0_Wake_Up_Handler,       // 80  CAN0_Wake_Up
    (uint32_t)CAN0_ORed_0_15_MB_Handler,  // 81  CAN0_ORed_0_15_MB
    (uint32_t)CAN0_ORed_16_31_MB_Handler, // 82  CAN0_ORed_16_31_MB
    (uint32_t)Unhandled_Interrupt,        // 83
    (uint32_t)Unhandled_Interrupt,        // 84
    (uint32_t)CAN1_ORed_Handler,          // 85  CAN1_ORed
    (uint32_t)CAN1_Error_Handler,         // 86  CAN1_Error
    (uint32_t)Unhandled_Interrupt,        // 87
    (uint32_t)CAN1_ORed_0_15_MB_Handler,  // 88  CAN1_ORed_0_15_MB
    (uint32_t)Unhandled_Interrupt,        // 89
    (uint32_t)Unhandled_Interrupt,        // 90
    (uint32_t)Unhandled_Interrupt,        // 91
    (uint32_t)CAN2_ORed_Handler,          // 92  CAN2_ORed
    (uint32_t)CAN2_Error_Handler,         // 93  CAN2_Error
    (uint32_t)Unhandled_Interrupt,        // 94
    (uint32_t)CAN2_ORed_0_15_MB_Handler,  // 95  CAN2_ORed_0_15_MB
    (uint32_t)Unhandled_Interrupt,        // 96
    (uint32_t)Unhandled_Interrupt,        // 97
    (uint32_t)Unhandled_Interrupt,        // 98
    (uint32_t)FTM0_Ch0_Ch1_Handler,       // 99  FTM0_Ch0_Ch1
    (uint32_t)FTM0_Ch2_Ch3_Handler,       // 100 FTM0_Ch2_Ch3
    (uint32_t)FTM0_Ch4_Ch5_Handler,       // 101 FTM0_Ch4_Ch5
    (uint32_t)FTM0_Ch6_Ch7_Handler,       // 102 FTM0_Ch6_Ch7
    (uint32_t)FTM0_Fault_Handler,         // 103 FTM0_Fault
    (uint32_t)FTM0_Ovf_Reload_Handler,    // 104 FTM0_Ovf_Reload
    (uint32_t)FTM1_Ch0_Ch1_Handler,       // 105 FTM1_Ch0_Ch1
    (uint32_t)FTM1_Ch2_Ch3_Handler,       // 106 FTM1_Ch2_Ch3
    (uint32_t)FTM1_Ch4_Ch5_Handler,       // 107 FTM1_Ch4_Ch5
    (uint32_t)FTM1_Ch6_Ch7_Handler,       // 108 FTM1_Ch6_Ch7
    (uint32_t)FTM1_Fault_Handler,         // 109 FTM1_Fault
    (uint32_t)FTM1_Ovf_Reload_Handler,    // 110 FTM1_Ovf_Reload
    (uint32_t)FTM2_Ch0_Ch1_Handler,       // 111 FTM2_Ch0_Ch1
    (uint32_t)FTM2_Ch2_Ch3_Handler,       // 112 FTM2_Ch2_Ch3
    (uint32_t)FTM2_Ch4_Ch5_Handler,       // 113 FTM2_Ch4_Ch5
    (uint32_t)FTM2_Ch6_Ch7_Handler,       // 114 FTM2_Ch6_Ch7
    (uint32_t)FTM2_Fault_Handler,         // 115 FTM2_Fault
    (uint32_t)FTM2_Ovf_Reload_Handler,    // 116 FTM2_Ovf_Reload
    (uint32_t)FTM3_Ch0_Ch1_Handler,       // 117 FTM3_Ch0_Ch1
    (uint32_t)FTM3_Ch2_Ch3_Handler,       // 118 FTM3_Ch2_Ch3
    (uint32_t)FTM3_Ch4_Ch5_Handler,       // 119 FTM3_Ch4_Ch5
    (uint32_t)FTM3_Ch6_Ch7_Handler,       // 120 FTM3_Ch6_Ch7
    (uint32_t)FTM3_Fault_Handler,         // 121 FTM3_Fault
    (uint32_t)FTM3_Ovf_Reload_Handler,    // 122 FTM3_Ovf_Reload
};

