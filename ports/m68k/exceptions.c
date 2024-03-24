//
// M68K architecture support.
//

#include <stdio.h>
#include <string.h>
#include <py/mpconfig.h>

#include "mphalport.h"

#ifndef M68K_MIN_IPL
#define M68K_MIN_IPL   0
#endif

// exception handler function prototype
typedef void (*handler_func_t)(void);

// vector table
#define M68K_NUM_VECTORS 256
static const handler_func_t m68k_vectors[M68K_NUM_VECTORS];

// write VBR
static inline void
m68k_set_vbr(const void *vector_base) {
    uintptr_t value = (uintptr_t)vector_base;
    __asm__ volatile (
        "movec %0, %%vbr"
        :
        : "d" (value)
        : "memory"
        );
}

// halt forever
__attribute__((noreturn))
static inline void
m68k_halt(void) {
    for (;;) {
        __asm__ volatile (
            "stop #0x2700"
            :
            :
            : "memory"
            );
    }
}

// soft-reset
__attribute__((noreturn))
static inline void
m68k_reset(void) {
    for (;;) {
        __asm__ volatile (
            "reset"
            :
            :
            : "memory"
            );
    }
}

#ifndef M68K_EARLY_STARTUP_HOOK
#define M68K_EARLY_STARTUP_HOOK m68k_init
#endif
void m68k_start(void);
__asm__ (
    "   .section .text.startup                  \n"
    "   .align 2                                \n"
    "   .type m68k_start @function              \n"
    "   .global m68k_start                      \n"
    "m68k_start:                                \n" // reset entrypoint
    "   move.w  #0x2700, %sr                    \n" // disable interrupts immediately
    "   lea.l   _estack, %sp                    \n" // load the intended stack pointer
    "   bra " __XSTR(M68K_EARLY_STARTUP_HOOK)  "\n" // call early startup hook
    );

// symbols from sections.ld
extern handler_func_t m68k_live_vectors[M68K_NUM_VECTORS];
extern char _etext, _sdata, _edata, _sbss, _ebss, _sstack, _estack, _sheap, _eheap;

// runtime init, chained from m68k_board_early_init
__attribute__((noreturn, section(".text.startup")))
void m68k_init(void) {
    // set VBR or copy vectors to zero if required
    #if defined(__mc68010__) || defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__) || defined(__cpu32__)
    m68k_set_vbr(&m68k_vectors);
    #else /* m68000 */
    if (&m68k_live_vectors != &m68k_vectors) {
        // don't copy initial stack / reset vectors as they're not needed and some
        // hardware won't allow it
        for (int i = 2; i < M68K_NUM_VECTORS; i++) {
            m68k_live_vectors[i] = m68k_vectors[i];
        }
    }
    #endif /* m68000 */

    // move / copy DATA if required
    memmove(&_sdata, &_etext, &_edata - &_sdata);

    // zero out the .bss section
    memset(&_sbss, 0, &_ebss - &_sbss);

    // do board init
    m68k_board_init();

    // enable interrupts
    m68k_interrupt_enable(M68K_MIN_IPL << 8);

    // call main in a loop forever
    for (;;) {
        m68k_main();
    }
}

//
// Exception handlers
//
#if defined(__mc68010__) || defined(__mc68020__) || defined(__mc68030__) || defined(__mc68040__) || defined(__mc68060__) || defined(__cpu32__)
typedef struct __attribute__((packed)) {
    uint16_t sr;
    uint32_t pc;
    uint16_t format:4;
    uint16_t vector:12;
    union {
        struct {
            uint32_t address;
        } format_0x2;
        struct {
            uint32_t effective_address;
        } format_0x3;
        struct {
            uint32_t effective_address;
            uint32_t faulting_pc;
        } format_0x4;
        struct {
            uint32_t effective_address;
            uint16_t ssw;
            uint16_t writeback_3_status;
            uint16_t writeback_2_status;
            uint16_t writeback_1_status;
            uint32_t fault_address;
            uint32_t writeback_3_address;
            uint32_t writeback_3_data;
            uint32_t writeback_2_address;
            uint32_t writeback_2_data;
            uint32_t writeback_1_address;
            uint32_t writeback_1_data;
            uint32_t push_data_1;
            uint32_t push_data_2;
            uint32_t push_data_3;
        } format_0x7;
        struct {
            uint16_t ssw;
            uint32_t fault_address;
            uint16_t :16;
            uint16_t output_buffer;
            uint16_t :16;
            uint16_t input_buffer;
            uint16_t :16;
            uint16_t instruction_buffer;
            uint16_t internal[16];
        } format_0x8;
        struct {
            uint32_t instruction_address;
            uint16_t internal[4];
        } format_0x9;
        struct {
            uint16_t internal_0;
            uint16_t ssw;
            uint16_t instruction_pipe_c;
            uint16_t instruction_pipe_b;
            uint32_t data_fault_address;
            uint16_t internal_1;
            uint16_t internal_2;
            uint32_t data_output_buffer;
            uint16_t internal_3;
            uint16_t internal_4;
        } format_0xa;
        struct {
            uint16_t internal_0;
            uint16_t ssw;
            uint16_t instruction_pipe_c;
            uint16_t instruction_pipe_b;
            uint32_t data_fault_address;
            uint16_t internal_1;
            uint16_t internal_2;
            uint32_t data_output_buffer;
            uint16_t internal_3[4];
            uint32_t stage_b_address;
            uint16_t internal_4[2];
            uint32_t data_input_buffer;
            uint16_t internal_5[3];
            uint16_t version:4;
            uint16_t internal_6:12;
            uint16_t internal_7[18];
        } format_0xb;
        struct {
            uint32_t faulted_address;
            uint32_t data_buffer;
            uint32_t current_pc;
            uint16_t internal_xfer_count;
            uint16_t subformat:2;
            uint16_t ssw:14;
        } format_0xc;

    };
} m68k_common_frame_t;

void m68k_fleh(void);
__asm__(
    "   .section .text                      \n"
    "   .type m68k_fleh @function           \n"
    "m68k_fleh:                             \n"
    "   movem.l %d0-%d1/%a0-%a1,%sp@-       \n" /* save caller-saved registers    */    \
    "   move.l  %sp,%d0                     \n" /* get stack pointer              */    \
    "   add.l   #24,%d0                     \n" /* index past saved regs          */    \
    "   move.l  %d0,%sp@-                   \n" /* push address of hardware frame */    \
    "   jsr     m68k_sleh                   \n" /* m68k_sleh(frameptr)            */    \
    "   addq.l  #4, %sp                     \n" /* fix stack                      */    \
    "   movem.l %sp@+,%d0-%d1/%a0-%a1       \n" /* restore caller-saved registers */    \
    "   rte                                 \n"                                         \
    );

void m68k_sleh(m68k_common_frame_t *frame) {
    printf("FATAL: unhandled exception");
    m68k_halt();
}

#define EXCEPTION_VECTOR(_v) m68k_fleh

#else // __mc68000__ only

typedef struct __attribute__((packed)) {
    uint16_t ignored : 11;
    uint16_t rw : 1;
    uint16_t in : 1;
    uint16_t fc : 3;
    uint32_t addr;
    uint16_t ins;
    uint16_t sr;
    uint32_t pc;
} m68k_68000_frame_group_0_t;

typedef struct __attribute__((packed)) {
    uint16_t sr;
    uint32_t pc;
} m68k_68000_frame_group_1_2_t;

// m68000 group 0 (bus/address error) handler
__attribute__((used))
static void m68k_68000_exception_group_0(uint32_t vector, m68k_68000_frame_group_0_t *info) {
    printf("FATAL: %s error @ %p %s %p\n",
        ((vector == 2) ? "bus" : "address"),
        (void *)info->pc,
        (info->rw ? "reading" : "writing"),
        (void *)info->addr);

    m68k_halt();
}

// m68000 unhandled synchronous exception handler
__attribute__((used))
static void m68k_68000_exception_group_1_2(uint32_t vector, m68k_68000_frame_group_1_2_t *info) {
    printf("FATAL: unhandled exception %lu @ 0x%08lx\n", vector, info->pc - 2);
    m68k_halt();
}

#define EXCEPTION_HANDLER(_v, _k)                                                                   \
    void m68k_exception_handler_##_v(void);                                                         \
    __asm__ (                                                                                       \
    "    .section .text                                 \n"                                         \
    "    .align 2                                       \n"                                         \
    "    .type m68k_exception_handler_" #_v " @function \n"                                         \
    "m68k_exception_handler_" #_v ":                    \n"                                         \
    "    movem.l   %d0-%d1/%a0-%a1,%sp@-                \n" /* save caller-saved registers    */    \
    "    move.l    %sp,%d0                              \n" /* get stack pointer              */    \
    "    add.l     #24,%d0                              \n" /* index past saved regs          */    \
    "    move.l    %d0,%sp@-                            \n" /* push address of hardware frame */    \
    "    pea       " #_v "                              \n" /* push vector number             */    \
    "    jsr       " #_k "                              \n" /* handler(vector, frameptr)      */    \
    "    addq.l    #8, %sp                              \n" /* fix stack                      */    \
    "    movem.l   %sp@+,%d0-%d1/%a0-%a1                \n" /* restore caller-saved registers */    \
    "    rte                                            \n"                                         \
    );

#define EXCEPTION_VECTOR(_v) m68k_exception_handler_##_v

EXCEPTION_HANDLER(2, m68k_68000_exception_group_0);
EXCEPTION_HANDLER(3, m68k_68000_exception_group_0);
EXCEPTION_HANDLER(4, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(5, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(6, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(7, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(8, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(9, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(10, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(11, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(13, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(14, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(15, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(24, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(48, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(49, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(50, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(51, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(52, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(53, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(54, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(55, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(56, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(60, m68k_68000_exception_group_1_2);
EXCEPTION_HANDLER(61, m68k_68000_exception_group_1_2);

#endif // __mc68000__ only

// m68000 unhandled trap handler
__attribute__((used))
static void m68k_trap(uint32_t trap, uint32_t d0, uint32_t d1, void *a0, void *a1) {
    printf("FATAL: unhandled trap %lu\n", trap);
    m68k_halt();
}

#define TRAP_HANDLER(_v)                                                                        \
    void m68k_trap_handler_##_v(void);                                                          \
    __asm__ (                                                                                   \
    "    .section .text                             \n"                                         \
    "    .align 2                                   \n"                                         \
    "    .type m68k_trap_handler_" #_v " @function  \n"                                         \
    "m68k_trap_handler_" #_v ":                     \n"                                         \
    "    movem.l   %d0-%d1/%a0-%a1,%sp@-            \n" /* save caller-saved registers    */    \
    "    pea       " #_v "                          \n" /* push trap number               */    \
    "    jsr       m68k_trap                        \n" /* trap(_v, d0, d1, a0, a1)       */    \
    "    movem.l   %sp@+,%d0-%d1/%a0-%a1            \n" /* restore caller-saved registers */    \
    "    rte                                        \n"                                         \
    );

#define TRAP_VECTOR(_v) m68k_trap_handler_##_v

TRAP_HANDLER(0);
TRAP_HANDLER(1);
TRAP_HANDLER(2);
TRAP_HANDLER(3);
TRAP_HANDLER(4);
TRAP_HANDLER(5);
TRAP_HANDLER(6);
TRAP_HANDLER(7);
TRAP_HANDLER(8);
TRAP_HANDLER(9);
TRAP_HANDLER(10);
TRAP_HANDLER(11);
TRAP_HANDLER(12);
TRAP_HANDLER(13);
TRAP_HANDLER(14);
TRAP_HANDLER(15);

// m68000 unhandled asynchronous exception (interrupt) handler
static void m68k_interrupt(unsigned int vector) {
    printf("FATAL: unhandled interrupt %u\n", vector);
    m68k_halt();
}

#define INTERRUPT_HANDLER(_v)                                           \
    __attribute__((interrupt))                                          \
    void m68k_interrupt_trampoline_##_v(void) { m68k_interrupt(_v); }   \
    void m68k_interrupt_handler_##_v(void) __attribute__((weak, alias("m68k_interrupt_trampoline_" #_v)))

#define INTERRUPT_VECTOR(_v) m68k_interrupt_handler_##_v

INTERRUPT_HANDLER(12);
INTERRUPT_HANDLER(25);
INTERRUPT_HANDLER(26);
INTERRUPT_HANDLER(27);
INTERRUPT_HANDLER(28);
INTERRUPT_HANDLER(29);
INTERRUPT_HANDLER(30);
INTERRUPT_HANDLER(31);
INTERRUPT_HANDLER(64);
INTERRUPT_HANDLER(65);
INTERRUPT_HANDLER(66);
INTERRUPT_HANDLER(67);
INTERRUPT_HANDLER(68);
INTERRUPT_HANDLER(69);
INTERRUPT_HANDLER(70);
INTERRUPT_HANDLER(71);
INTERRUPT_HANDLER(72);
INTERRUPT_HANDLER(73);
INTERRUPT_HANDLER(74);
INTERRUPT_HANDLER(75);
INTERRUPT_HANDLER(76);
INTERRUPT_HANDLER(77);
INTERRUPT_HANDLER(78);
INTERRUPT_HANDLER(79);
INTERRUPT_HANDLER(80);
INTERRUPT_HANDLER(81);
INTERRUPT_HANDLER(82);
INTERRUPT_HANDLER(83);
INTERRUPT_HANDLER(84);
INTERRUPT_HANDLER(85);
INTERRUPT_HANDLER(86);
INTERRUPT_HANDLER(87);
INTERRUPT_HANDLER(88);
INTERRUPT_HANDLER(89);
INTERRUPT_HANDLER(90);
INTERRUPT_HANDLER(91);
INTERRUPT_HANDLER(92);
INTERRUPT_HANDLER(93);
INTERRUPT_HANDLER(94);
INTERRUPT_HANDLER(95);
INTERRUPT_HANDLER(96);
INTERRUPT_HANDLER(97);
INTERRUPT_HANDLER(98);
INTERRUPT_HANDLER(99);
INTERRUPT_HANDLER(100);
INTERRUPT_HANDLER(101);
INTERRUPT_HANDLER(102);
INTERRUPT_HANDLER(103);
INTERRUPT_HANDLER(104);
INTERRUPT_HANDLER(105);
INTERRUPT_HANDLER(106);
INTERRUPT_HANDLER(107);
INTERRUPT_HANDLER(108);
INTERRUPT_HANDLER(109);
INTERRUPT_HANDLER(110);
INTERRUPT_HANDLER(111);
INTERRUPT_HANDLER(112);
INTERRUPT_HANDLER(113);
INTERRUPT_HANDLER(114);
INTERRUPT_HANDLER(115);
INTERRUPT_HANDLER(116);
INTERRUPT_HANDLER(117);
INTERRUPT_HANDLER(118);
INTERRUPT_HANDLER(119);
INTERRUPT_HANDLER(120);
INTERRUPT_HANDLER(121);
INTERRUPT_HANDLER(122);
INTERRUPT_HANDLER(123);
INTERRUPT_HANDLER(124);
INTERRUPT_HANDLER(125);
INTERRUPT_HANDLER(126);
INTERRUPT_HANDLER(127);
INTERRUPT_HANDLER(128);
INTERRUPT_HANDLER(129);
INTERRUPT_HANDLER(130);
INTERRUPT_HANDLER(131);
INTERRUPT_HANDLER(132);
INTERRUPT_HANDLER(133);
INTERRUPT_HANDLER(134);
INTERRUPT_HANDLER(135);
INTERRUPT_HANDLER(136);
INTERRUPT_HANDLER(137);
INTERRUPT_HANDLER(138);
INTERRUPT_HANDLER(139);
INTERRUPT_HANDLER(140);
INTERRUPT_HANDLER(141);
INTERRUPT_HANDLER(142);
INTERRUPT_HANDLER(143);
INTERRUPT_HANDLER(144);
INTERRUPT_HANDLER(145);
INTERRUPT_HANDLER(146);
INTERRUPT_HANDLER(147);
INTERRUPT_HANDLER(148);
INTERRUPT_HANDLER(149);
INTERRUPT_HANDLER(150);
INTERRUPT_HANDLER(151);
INTERRUPT_HANDLER(152);
INTERRUPT_HANDLER(153);
INTERRUPT_HANDLER(154);
INTERRUPT_HANDLER(155);
INTERRUPT_HANDLER(156);
INTERRUPT_HANDLER(157);
INTERRUPT_HANDLER(158);
INTERRUPT_HANDLER(159);
INTERRUPT_HANDLER(160);
INTERRUPT_HANDLER(161);
INTERRUPT_HANDLER(162);
INTERRUPT_HANDLER(163);
INTERRUPT_HANDLER(164);
INTERRUPT_HANDLER(165);
INTERRUPT_HANDLER(166);
INTERRUPT_HANDLER(167);
INTERRUPT_HANDLER(168);
INTERRUPT_HANDLER(169);
INTERRUPT_HANDLER(170);
INTERRUPT_HANDLER(171);
INTERRUPT_HANDLER(172);
INTERRUPT_HANDLER(173);
INTERRUPT_HANDLER(174);
INTERRUPT_HANDLER(175);
INTERRUPT_HANDLER(176);
INTERRUPT_HANDLER(177);
INTERRUPT_HANDLER(178);
INTERRUPT_HANDLER(179);
INTERRUPT_HANDLER(180);
INTERRUPT_HANDLER(181);
INTERRUPT_HANDLER(182);
INTERRUPT_HANDLER(183);
INTERRUPT_HANDLER(184);
INTERRUPT_HANDLER(185);
INTERRUPT_HANDLER(186);
INTERRUPT_HANDLER(187);
INTERRUPT_HANDLER(188);
INTERRUPT_HANDLER(189);
INTERRUPT_HANDLER(190);
INTERRUPT_HANDLER(191);
INTERRUPT_HANDLER(192);
INTERRUPT_HANDLER(193);
INTERRUPT_HANDLER(194);
INTERRUPT_HANDLER(195);
INTERRUPT_HANDLER(196);
INTERRUPT_HANDLER(197);
INTERRUPT_HANDLER(198);
INTERRUPT_HANDLER(199);
INTERRUPT_HANDLER(200);
INTERRUPT_HANDLER(201);
INTERRUPT_HANDLER(202);
INTERRUPT_HANDLER(203);
INTERRUPT_HANDLER(204);
INTERRUPT_HANDLER(205);
INTERRUPT_HANDLER(206);
INTERRUPT_HANDLER(207);
INTERRUPT_HANDLER(208);
INTERRUPT_HANDLER(209);
INTERRUPT_HANDLER(210);
INTERRUPT_HANDLER(211);
INTERRUPT_HANDLER(212);
INTERRUPT_HANDLER(213);
INTERRUPT_HANDLER(214);
INTERRUPT_HANDLER(215);
INTERRUPT_HANDLER(216);
INTERRUPT_HANDLER(217);
INTERRUPT_HANDLER(218);
INTERRUPT_HANDLER(219);
INTERRUPT_HANDLER(220);
INTERRUPT_HANDLER(221);
INTERRUPT_HANDLER(222);
INTERRUPT_HANDLER(223);
INTERRUPT_HANDLER(224);
INTERRUPT_HANDLER(225);
INTERRUPT_HANDLER(226);
INTERRUPT_HANDLER(227);
INTERRUPT_HANDLER(228);
INTERRUPT_HANDLER(229);
INTERRUPT_HANDLER(230);
INTERRUPT_HANDLER(231);
INTERRUPT_HANDLER(232);
INTERRUPT_HANDLER(233);
INTERRUPT_HANDLER(234);
INTERRUPT_HANDLER(235);
INTERRUPT_HANDLER(236);
INTERRUPT_HANDLER(237);
INTERRUPT_HANDLER(238);
INTERRUPT_HANDLER(239);
INTERRUPT_HANDLER(240);
INTERRUPT_HANDLER(241);
INTERRUPT_HANDLER(242);
INTERRUPT_HANDLER(243);
INTERRUPT_HANDLER(244);
INTERRUPT_HANDLER(245);
INTERRUPT_HANDLER(246);
INTERRUPT_HANDLER(247);
INTERRUPT_HANDLER(248);
INTERRUPT_HANDLER(249);
INTERRUPT_HANDLER(250);
INTERRUPT_HANDLER(251);
INTERRUPT_HANDLER(252);
INTERRUPT_HANDLER(253);
INTERRUPT_HANDLER(254);
INTERRUPT_HANDLER(255);

// Vector table
//
// Populated with all 000/010/020/030/040/060/CPU32 vectors.
//
extern char _estack;
__attribute__((used, section(".vectors")))
static const handler_func_t m68k_vectors[M68K_NUM_VECTORS] = {
    [  0] = (handler_func_t)&_estack,
    [  1] = m68k_start,
    [  2] = EXCEPTION_VECTOR(2),        // bus error
    [  3] = EXCEPTION_VECTOR(3),        // address error
    [  4] = EXCEPTION_VECTOR(4),        // illegal instruction
    [  5] = EXCEPTION_VECTOR(5),        // zero divide
    [  6] = EXCEPTION_VECTOR(6),        // chk
    [  7] = EXCEPTION_VECTOR(7),        // trapv
    [  8] = EXCEPTION_VECTOR(8),        // privilege violation
    [  9] = EXCEPTION_VECTOR(9),        // trace
    [ 10] = EXCEPTION_VECTOR(10),       // line a
    [ 11] = EXCEPTION_VECTOR(11),       // line f
    [ 12] = INTERRUPT_VECTOR(12),       // emulator interrupt
    [ 13] = EXCEPTION_VECTOR(13),       // coprocessor protocol violation
    [ 14] = EXCEPTION_VECTOR(14),       // format error
    [ 15] = EXCEPTION_VECTOR(15),       // uninitialized interrupt
    // ...
    [ 24] = EXCEPTION_VECTOR(24),       // spurious interrupt
    [ 25] = INTERRUPT_VECTOR(25),       // level 1 autovector
    [ 26] = INTERRUPT_VECTOR(26),       // level 2 autovector
    [ 27] = INTERRUPT_VECTOR(27),       // level 3 autovector
    [ 28] = INTERRUPT_VECTOR(28),       // level 4 autovector
    [ 29] = INTERRUPT_VECTOR(29),       // level 5 autovector
    [ 30] = INTERRUPT_VECTOR(30),       // level 6 autovector
    [ 31] = INTERRUPT_VECTOR(31),       // level 7 autovector
    [ 32] = TRAP_VECTOR(0),             // trap 0 vector
    [ 33] = TRAP_VECTOR(1),             // trap 1 vector
    [ 34] = TRAP_VECTOR(2),             // trap 2 vector
    [ 35] = TRAP_VECTOR(3),             // trap 3 vector
    [ 36] = TRAP_VECTOR(4),             // trap 4 vector
    [ 37] = TRAP_VECTOR(5),             // trap 5 vector
    [ 38] = TRAP_VECTOR(6),             // trap 6 vector
    [ 39] = TRAP_VECTOR(7),             // trap 7 vector
    [ 40] = TRAP_VECTOR(8),             // trap 8 vector
    [ 41] = TRAP_VECTOR(9),             // trap 9 vector
    [ 42] = TRAP_VECTOR(10),            // trap 10 vector
    [ 43] = TRAP_VECTOR(11),            // trap 11 vector
    [ 44] = TRAP_VECTOR(12),            // trap 12 vector
    [ 45] = TRAP_VECTOR(13),            // trap 13 vector
    [ 46] = TRAP_VECTOR(14),            // trap 14 vector
    [ 47] = TRAP_VECTOR(15),            // trap 15 vector
    [ 48] = EXCEPTION_VECTOR(48),       // FP branch or set on unordered condition
    [ 49] = EXCEPTION_VECTOR(49),       // FP inexact result
    [ 50] = EXCEPTION_VECTOR(50),       // FP divide by zero
    [ 51] = EXCEPTION_VECTOR(51),       // FP underflow
    [ 52] = EXCEPTION_VECTOR(52),       // FP operand error
    [ 53] = EXCEPTION_VECTOR(53),       // FP overflow
    [ 54] = EXCEPTION_VECTOR(54),       // FP signaling NAN
    [ 55] = EXCEPTION_VECTOR(55),       // FP unimplemented data type
    [ 56] = EXCEPTION_VECTOR(56),       // MMU configuration error
    // ...
    [ 60] = EXCEPTION_VECTOR(60),       // unimplemented effective address
    [ 61] = EXCEPTION_VECTOR(61),       // unimplemented integer instruction
    // ...
    [ 64] = INTERRUPT_VECTOR(64),       // interrupt vector 64
    [ 65] = INTERRUPT_VECTOR(65),       // interrupt vector 65
    [ 66] = INTERRUPT_VECTOR(66),       // interrupt vector 66
    [ 67] = INTERRUPT_VECTOR(67),       // interrupt vector 67
    [ 68] = INTERRUPT_VECTOR(68),       // interrupt vector 68
    [ 69] = INTERRUPT_VECTOR(69),       // interrupt vector 69
    [ 70] = INTERRUPT_VECTOR(70),       // interrupt vector 70
    [ 71] = INTERRUPT_VECTOR(71),       // interrupt vector 71
    [ 72] = INTERRUPT_VECTOR(72),       // interrupt vector 72
    [ 73] = INTERRUPT_VECTOR(73),       // interrupt vector 73
    [ 74] = INTERRUPT_VECTOR(74),       // interrupt vector 74
    [ 75] = INTERRUPT_VECTOR(75),       // interrupt vector 75
    [ 76] = INTERRUPT_VECTOR(76),       // interrupt vector 76
    [ 77] = INTERRUPT_VECTOR(77),       // interrupt vector 77
    [ 78] = INTERRUPT_VECTOR(78),       // interrupt vector 78
    [ 79] = INTERRUPT_VECTOR(79),       // interrupt vector 79
    [ 80] = INTERRUPT_VECTOR(80),       // interrupt vector 80
    [ 81] = INTERRUPT_VECTOR(81),       // interrupt vector 81
    [ 82] = INTERRUPT_VECTOR(82),       // interrupt vector 82
    [ 83] = INTERRUPT_VECTOR(83),       // interrupt vector 83
    [ 84] = INTERRUPT_VECTOR(84),       // interrupt vector 84
    [ 85] = INTERRUPT_VECTOR(85),       // interrupt vector 85
    [ 86] = INTERRUPT_VECTOR(86),       // interrupt vector 86
    [ 87] = INTERRUPT_VECTOR(87),       // interrupt vector 87
    [ 88] = INTERRUPT_VECTOR(88),       // interrupt vector 88
    [ 89] = INTERRUPT_VECTOR(89),       // interrupt vector 89
    [ 90] = INTERRUPT_VECTOR(90),       // interrupt vector 90
    [ 91] = INTERRUPT_VECTOR(91),       // interrupt vector 91
    [ 92] = INTERRUPT_VECTOR(92),       // interrupt vector 92
    [ 93] = INTERRUPT_VECTOR(93),       // interrupt vector 93
    [ 94] = INTERRUPT_VECTOR(94),       // interrupt vector 94
    [ 95] = INTERRUPT_VECTOR(95),       // interrupt vector 95
    [ 96] = INTERRUPT_VECTOR(96),       // interrupt vector 96
    [ 97] = INTERRUPT_VECTOR(97),       // interrupt vector 97
    [ 98] = INTERRUPT_VECTOR(98),       // interrupt vector 98
    [ 99] = INTERRUPT_VECTOR(99),       // interrupt vector 99
    [100] = INTERRUPT_VECTOR(100),      // interrupt vector 100
    [101] = INTERRUPT_VECTOR(101),      // interrupt vector 101
    [102] = INTERRUPT_VECTOR(102),      // interrupt vector 102
    [103] = INTERRUPT_VECTOR(103),      // interrupt vector 103
    [104] = INTERRUPT_VECTOR(104),      // interrupt vector 104
    [105] = INTERRUPT_VECTOR(105),      // interrupt vector 105
    [106] = INTERRUPT_VECTOR(106),      // interrupt vector 106
    [107] = INTERRUPT_VECTOR(107),      // interrupt vector 107
    [108] = INTERRUPT_VECTOR(108),      // interrupt vector 108
    [109] = INTERRUPT_VECTOR(109),      // interrupt vector 109
    [110] = INTERRUPT_VECTOR(110),      // interrupt vector 110
    [111] = INTERRUPT_VECTOR(111),      // interrupt vector 111
    [112] = INTERRUPT_VECTOR(112),      // interrupt vector 112
    [113] = INTERRUPT_VECTOR(113),      // interrupt vector 113
    [114] = INTERRUPT_VECTOR(114),      // interrupt vector 114
    [115] = INTERRUPT_VECTOR(115),      // interrupt vector 115
    [116] = INTERRUPT_VECTOR(116),      // interrupt vector 116
    [117] = INTERRUPT_VECTOR(117),      // interrupt vector 117
    [118] = INTERRUPT_VECTOR(118),      // interrupt vector 118
    [119] = INTERRUPT_VECTOR(119),      // interrupt vector 119
    [120] = INTERRUPT_VECTOR(120),      // interrupt vector 120
    [121] = INTERRUPT_VECTOR(121),      // interrupt vector 121
    [122] = INTERRUPT_VECTOR(122),      // interrupt vector 122
    [123] = INTERRUPT_VECTOR(123),      // interrupt vector 123
    [124] = INTERRUPT_VECTOR(124),      // interrupt vector 124
    [125] = INTERRUPT_VECTOR(125),      // interrupt vector 125
    [126] = INTERRUPT_VECTOR(126),      // interrupt vector 126
    [127] = INTERRUPT_VECTOR(127),      // interrupt vector 127
    [128] = INTERRUPT_VECTOR(128),      // interrupt vector 128
    [129] = INTERRUPT_VECTOR(129),      // interrupt vector 129
    [130] = INTERRUPT_VECTOR(130),      // interrupt vector 130
    [131] = INTERRUPT_VECTOR(131),      // interrupt vector 131
    [132] = INTERRUPT_VECTOR(132),      // interrupt vector 132
    [133] = INTERRUPT_VECTOR(133),      // interrupt vector 133
    [134] = INTERRUPT_VECTOR(134),      // interrupt vector 134
    [135] = INTERRUPT_VECTOR(135),      // interrupt vector 135
    [136] = INTERRUPT_VECTOR(136),      // interrupt vector 136
    [137] = INTERRUPT_VECTOR(137),      // interrupt vector 137
    [138] = INTERRUPT_VECTOR(138),      // interrupt vector 138
    [139] = INTERRUPT_VECTOR(139),      // interrupt vector 139
    [140] = INTERRUPT_VECTOR(140),      // interrupt vector 140
    [141] = INTERRUPT_VECTOR(141),      // interrupt vector 141
    [142] = INTERRUPT_VECTOR(142),      // interrupt vector 142
    [143] = INTERRUPT_VECTOR(143),      // interrupt vector 143
    [144] = INTERRUPT_VECTOR(144),      // interrupt vector 144
    [145] = INTERRUPT_VECTOR(145),      // interrupt vector 145
    [146] = INTERRUPT_VECTOR(146),      // interrupt vector 146
    [147] = INTERRUPT_VECTOR(147),      // interrupt vector 147
    [148] = INTERRUPT_VECTOR(148),      // interrupt vector 148
    [149] = INTERRUPT_VECTOR(149),      // interrupt vector 149
    [150] = INTERRUPT_VECTOR(150),      // interrupt vector 150
    [151] = INTERRUPT_VECTOR(151),      // interrupt vector 151
    [152] = INTERRUPT_VECTOR(152),      // interrupt vector 152
    [153] = INTERRUPT_VECTOR(153),      // interrupt vector 153
    [154] = INTERRUPT_VECTOR(154),      // interrupt vector 154
    [155] = INTERRUPT_VECTOR(155),      // interrupt vector 155
    [156] = INTERRUPT_VECTOR(156),      // interrupt vector 156
    [157] = INTERRUPT_VECTOR(157),      // interrupt vector 157
    [158] = INTERRUPT_VECTOR(158),      // interrupt vector 158
    [159] = INTERRUPT_VECTOR(159),      // interrupt vector 159
    [160] = INTERRUPT_VECTOR(160),      // interrupt vector 160
    [161] = INTERRUPT_VECTOR(161),      // interrupt vector 161
    [162] = INTERRUPT_VECTOR(162),      // interrupt vector 162
    [163] = INTERRUPT_VECTOR(163),      // interrupt vector 163
    [164] = INTERRUPT_VECTOR(164),      // interrupt vector 164
    [165] = INTERRUPT_VECTOR(165),      // interrupt vector 165
    [166] = INTERRUPT_VECTOR(166),      // interrupt vector 166
    [167] = INTERRUPT_VECTOR(167),      // interrupt vector 167
    [168] = INTERRUPT_VECTOR(168),      // interrupt vector 168
    [169] = INTERRUPT_VECTOR(169),      // interrupt vector 169
    [170] = INTERRUPT_VECTOR(170),      // interrupt vector 170
    [171] = INTERRUPT_VECTOR(171),      // interrupt vector 171
    [172] = INTERRUPT_VECTOR(172),      // interrupt vector 172
    [173] = INTERRUPT_VECTOR(173),      // interrupt vector 173
    [174] = INTERRUPT_VECTOR(174),      // interrupt vector 174
    [175] = INTERRUPT_VECTOR(175),      // interrupt vector 175
    [176] = INTERRUPT_VECTOR(176),      // interrupt vector 176
    [177] = INTERRUPT_VECTOR(177),      // interrupt vector 177
    [178] = INTERRUPT_VECTOR(178),      // interrupt vector 178
    [179] = INTERRUPT_VECTOR(179),      // interrupt vector 179
    [180] = INTERRUPT_VECTOR(180),      // interrupt vector 180
    [181] = INTERRUPT_VECTOR(181),      // interrupt vector 181
    [182] = INTERRUPT_VECTOR(182),      // interrupt vector 182
    [183] = INTERRUPT_VECTOR(183),      // interrupt vector 183
    [184] = INTERRUPT_VECTOR(184),      // interrupt vector 184
    [185] = INTERRUPT_VECTOR(185),      // interrupt vector 185
    [186] = INTERRUPT_VECTOR(186),      // interrupt vector 186
    [187] = INTERRUPT_VECTOR(187),      // interrupt vector 187
    [188] = INTERRUPT_VECTOR(188),      // interrupt vector 188
    [189] = INTERRUPT_VECTOR(189),      // interrupt vector 189
    [190] = INTERRUPT_VECTOR(190),      // interrupt vector 190
    [191] = INTERRUPT_VECTOR(191),      // interrupt vector 191
    [192] = INTERRUPT_VECTOR(192),      // interrupt vector 192
    [193] = INTERRUPT_VECTOR(193),      // interrupt vector 193
    [194] = INTERRUPT_VECTOR(194),      // interrupt vector 194
    [195] = INTERRUPT_VECTOR(195),      // interrupt vector 195
    [196] = INTERRUPT_VECTOR(196),      // interrupt vector 196
    [197] = INTERRUPT_VECTOR(197),      // interrupt vector 197
    [198] = INTERRUPT_VECTOR(198),      // interrupt vector 198
    [199] = INTERRUPT_VECTOR(199),      // interrupt vector 199
    [200] = INTERRUPT_VECTOR(200),      // interrupt vector 200
    [201] = INTERRUPT_VECTOR(201),      // interrupt vector 201
    [202] = INTERRUPT_VECTOR(202),      // interrupt vector 202
    [203] = INTERRUPT_VECTOR(203),      // interrupt vector 203
    [204] = INTERRUPT_VECTOR(204),      // interrupt vector 204
    [205] = INTERRUPT_VECTOR(205),      // interrupt vector 205
    [206] = INTERRUPT_VECTOR(206),      // interrupt vector 206
    [207] = INTERRUPT_VECTOR(207),      // interrupt vector 207
    [208] = INTERRUPT_VECTOR(208),      // interrupt vector 208
    [209] = INTERRUPT_VECTOR(209),      // interrupt vector 209
    [210] = INTERRUPT_VECTOR(210),      // interrupt vector 210
    [211] = INTERRUPT_VECTOR(211),      // interrupt vector 211
    [212] = INTERRUPT_VECTOR(212),      // interrupt vector 212
    [213] = INTERRUPT_VECTOR(213),      // interrupt vector 213
    [214] = INTERRUPT_VECTOR(214),      // interrupt vector 214
    [215] = INTERRUPT_VECTOR(215),      // interrupt vector 215
    [216] = INTERRUPT_VECTOR(216),      // interrupt vector 216
    [217] = INTERRUPT_VECTOR(217),      // interrupt vector 217
    [218] = INTERRUPT_VECTOR(218),      // interrupt vector 218
    [219] = INTERRUPT_VECTOR(219),      // interrupt vector 219
    [220] = INTERRUPT_VECTOR(220),      // interrupt vector 220
    [221] = INTERRUPT_VECTOR(221),      // interrupt vector 221
    [222] = INTERRUPT_VECTOR(222),      // interrupt vector 222
    [223] = INTERRUPT_VECTOR(223),      // interrupt vector 223
    [224] = INTERRUPT_VECTOR(224),      // interrupt vector 224
    [225] = INTERRUPT_VECTOR(225),      // interrupt vector 225
    [226] = INTERRUPT_VECTOR(226),      // interrupt vector 226
    [227] = INTERRUPT_VECTOR(227),      // interrupt vector 227
    [228] = INTERRUPT_VECTOR(228),      // interrupt vector 228
    [229] = INTERRUPT_VECTOR(229),      // interrupt vector 229
    [230] = INTERRUPT_VECTOR(230),      // interrupt vector 230
    [231] = INTERRUPT_VECTOR(231),      // interrupt vector 231
    [232] = INTERRUPT_VECTOR(232),      // interrupt vector 232
    [233] = INTERRUPT_VECTOR(233),      // interrupt vector 233
    [234] = INTERRUPT_VECTOR(234),      // interrupt vector 234
    [235] = INTERRUPT_VECTOR(235),      // interrupt vector 235
    [236] = INTERRUPT_VECTOR(236),      // interrupt vector 236
    [237] = INTERRUPT_VECTOR(237),      // interrupt vector 237
    [238] = INTERRUPT_VECTOR(238),      // interrupt vector 238
    [239] = INTERRUPT_VECTOR(239),      // interrupt vector 239
    [240] = INTERRUPT_VECTOR(240),      // interrupt vector 240
    [241] = INTERRUPT_VECTOR(241),      // interrupt vector 241
    [242] = INTERRUPT_VECTOR(242),      // interrupt vector 242
    [243] = INTERRUPT_VECTOR(243),      // interrupt vector 243
    [244] = INTERRUPT_VECTOR(244),      // interrupt vector 244
    [245] = INTERRUPT_VECTOR(245),      // interrupt vector 245
    [246] = INTERRUPT_VECTOR(246),      // interrupt vector 246
    [247] = INTERRUPT_VECTOR(247),      // interrupt vector 247
    [248] = INTERRUPT_VECTOR(248),      // interrupt vector 248
    [249] = INTERRUPT_VECTOR(249),      // interrupt vector 249
    [250] = INTERRUPT_VECTOR(250),      // interrupt vector 250
    [251] = INTERRUPT_VECTOR(251),      // interrupt vector 251
    [252] = INTERRUPT_VECTOR(252),      // interrupt vector 252
    [253] = INTERRUPT_VECTOR(253),      // interrupt vector 253
    [254] = INTERRUPT_VECTOR(254),      // interrupt vector 254
    [255] = INTERRUPT_VECTOR(255),      // interrupt vector 255
};
