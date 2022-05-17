// CAN support for the CC16
//
// Mailbox allocation:
//
// 0-7: RX FIFO and 8 FIFO mask registers
// 8: console RX mailbox
// 9: console TX mailbox
// 10-15 / 10-31: free for buffered tx?

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "s32k144.h"

typedef union {
    struct {
        uint32_t    timestamp : 16;
        uint32_t    dlc : 4;
        uint32_t    rtr : 1;
        uint32_t    ide : 1;
        uint32_t    srr : 1;
        uint32_t    : 1;
        uint32_t    code : 4;
        uint32_t    : 1;
        uint32_t    esi : 1;
        uint32_t    brs : 1;
        uint32_t    edl : 1;

        union {
            struct {
                uint32_t    id_ext : 29;
                uint32_t    prio : 3;
            };
            struct {
                uint32_t    : 18;
                uint32_t    id : 11;
                uint32_t    : 3;
            };
        };
        uint8_t     data3;
        uint8_t     data2;
        uint8_t     data1;
        uint8_t     data0;
        uint8_t     data7;
        uint8_t     data6;
        uint8_t     data5;
        uint8_t     data4;
    };
    uint32_t    u32[4];    
} Mailbox_t;

enum {
    // RX mailbox code values
    RX_INACTIVE = 0x0,
    RX_EMPTY    = 0x4,
    RX_FULL     = 0x2,
    RX_OVERRUN  = 0x6,
    RX_RANSWER  = 0xa,
    RX_BUSY     = 0x1,

    // TX mailbox code values
    TX_INACTIVE = 0x8,
    TX_ABORT    = 0x9,
    TX_DATA     = 0xc,
    TX_TANSWER  = 0xe
};

typedef struct {
    mp_obj_base_t       base;
    volatile CAN_regs_t *regs;
} cc16_can_obj_t;

static const cc16_can_obj_t cc16_can_obj[] = {
    {{&cc16_can_type}, CAN0 },
    {{&cc16_can_type}, CAN1 },
};

#define NUM_CAN (sizeof(cc16_can_obj) / sizeof(cc16_can_obj_t))

#define RX_FIFO_INTERRUPT       CAN_IFLAG1_BUF5I
#define RX_CONS_INTERRUPT       (1U << 8)

#define MBOX_ADDR(_can, _index) (&(_can)->RAMn0 + (4 * (_index)))
#define MBOX_FIFO(_can)         MBOX_ADDR(_can, 0)
#define MBOX_CONS_RECV(_can)    MBOX_ADDR(_can, 8)
#define MBOX_CONS_SEND(_can)    MBOX_ADDR(_can, 9)

#define MBOX_FIRST_TX           10
#define MBOX_LAST_TX            15

static void _can_configure(volatile CAN_regs_t *can) {

    // configure for 500kBps
    can->CTRL1 |= CAN_CTRL1_CLKSRC;             // select 80Mhz clock
    can->MCR |= CAN_MCR_FRZ;                    // enable freeze mode
    can->MCR &= ~CAN_MCR_MDIS;                  // clear disable bit
    while (can->MCR & CAN_MCR_LPMACK) {
        ;                                       // ... and wait for block to leave low-power mode
    }
    can->MCR ^= CAN_MCR_SOFTRST;                // request soft reset
    while (can->MCR & CAN_MCR_SOFTRST) {
        ;                                       // ... and wait for it to finish
    }
    while (!(can->MCR & CAN_MCR_FRZACK)) {
        ;                                       // ... entering frozen mode
    }
    can->MCR |= CAN_MCR_IRMQ |                  // new-style masking
        CAN_MCR_SRXDIS |                        // disable self-reception
        CAN_MCR_RFEN |                          // enable receive FIFO
        CAN_MCR_IDAM(0);                        // 32-bit acceptance filters
    can->CBT = CAN_CBT_BTF |                    // configure for 500kBPS @ 80MHz
        CAN_CBT_ERJW(1) |
        CAN_CBT_EPRESDIV(0x09) |
        CAN_CBT_EPROPSEG(0x07) |
        CAN_CBT_EPSEG1(0x04) |
        CAN_CBT_EPSEG2(0x01);
    can->CTRL2 |= CAN_CTRL2_MRP;                // prefer individual mailboxes over FIFO for rx

    // clear and enable interrupts
    can->IFLAG1 |= RX_FIFO_INTERRUPT | RX_CONS_INTERRUPT;
    can->IMASK1 |= RX_FIFO_INTERRUPT | RX_CONS_INTERRUPT;

    // set initial RX FIFO masks
    can->RAMn24 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 0 matches nothing
    can->RXIMR0 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn25 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 1 matches nothing
    can->RXIMR1 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn26 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 2 matches nothing
    can->RXIMR2 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn27 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 3 matches nothing
    can->RXIMR3 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn28 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 4 matches nothing
    can->RXIMR4 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn29 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 5 matches nothing
    can->RXIMR5 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn30 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 6 matches nothing
    can->RXIMR6 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit
    can->RAMn31 = (1U << 30) | (0x1fffffff << 1); // FIFO filter 7 matches nothing
    can->RXIMR7 = (1U << 30) | (0x1fffffff << 1); // ... mask to suit

    if (can == CAN0) {
        // set console RX mask for mailbox 8
        can->RXIMR8 = 0x1fffffff;                  // console RX buffer mask
        can->RAMn33 = 0x1ffffffd;                  // console RX buffer ID
    }

    can->MCR &= ~CAN_MCR_HALT;                      // disable halt mode
    while (can->MCR & CAN_MCR_NOTRDY) {
        ;                                           // ... and wait for it to take effect
    }

    if (can == CAN0) {
        can->RAMn32 = (RX_EMPTY << 24) | (1 << 21); // mark console RX buffer as idle
    } else {
        can->RAMn32 = RX_INACTIVE    << 24;         // mark unused buffer as idle
    }
    can->RAMn36 = TX_INACTIVE << 24;                // mark console TX buffer as idle
    can->RAMn40 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn44 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn48 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn52 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn56 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
    can->RAMn60 = TX_INACTIVE << 24;                // mark unused TX buffer as idle
   
}

void cc16_can_configure(void) {

    PCC->PCC_FlexCAN0 |= PCC_PCC_FlexCAN0_CGC;  // clock on
    PCC->PCC_FlexCAN1 |= PCC_PCC_FlexCAN1_CGC;  // clock on

    _can_configure(CAN0);
    _can_configure(CAN1);

    NVIC_EnableIRQ(CAN0_ORed_0_15_MB_IRQn);
    NVIC_EnableIRQ(CAN1_ORed_0_15_MB_IRQn);
}

// Send string of given length to stdout.
//
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    while (len) {
        size_t dlc = (len >= 8) ? 8 : len;

        while ((CAN0->RAMn36 & 0x0f000000) != 0x08000000) {
            ;                                               // wait for TX buffer

        }
        CAN0->RAMn37 = 0x1ffffffe;                          // CAN stdout ID
        CAN0->RAMn38 = ((len > 0) ? (str[0] << 24) : 0) |   // pack bytes big-endian
            ((len > 1) ? (str[1] << 16) : 0) |
            ((len > 2) ? (str[2] << 8) : 0) |
            ((len > 3) ? (str[3] << 0) : 0);
        CAN0->RAMn39 = ((len > 4) ? (str[4] << 24) : 0) |
            ((len > 5) ? (str[5] << 16) : 0) |
            ((len > 6) ? (str[6] << 8) : 0) |
            ((len > 7) ? (str[7] << 0) : 0);
        CAN0->RAMn36 = (0xc << 24) | (1U << 21) | (dlc << 16); // start transmission

        len -= dlc;
        str += dlc;
    }
}

// CAN console input buffer.
//
static volatile struct {
    char buf[128];
    uint8_t head, tail;
    int interrupt;
} cons_buf;

#define CONS_PTR_NEXT(_p)   ((_p + 1) % sizeof(cons_buf.buf))
#define CONS_BUF_FULL       (CONS_PTR_NEXT(cons_buf.tail) == cons_buf.head)
#define CONS_BUF_EMPTY      (cons_buf.head == cons_buf.tail)
#define CONS_BUF_POP(_v)                                    \
    if (CONS_BUF_EMPTY) {                                   \
        _v = -1;                                            \
    } else {                                                \
        _v = cons_buf.buf[cons_buf.head];                   \
        cons_buf.head = CONS_PTR_NEXT(cons_buf.head);       \
    }

// Receive a console byte
//
static inline void cons_buf_push(char c) {
    if (c == cons_buf.interrupt) {
        mp_sched_keyboard_interrupt();
    } else if (!CONS_BUF_FULL) {
        cons_buf.buf[cons_buf.tail] = c;
        cons_buf.tail = CONS_PTR_NEXT(cons_buf.tail);
    }
}

void mp_hal_set_interrupt_char(int c) {
    cons_buf.interrupt = c;
}

// CAN interrupt handlers
//

static void _can_interrupt(volatile CAN_regs_t *can) {

    if (can == CAN0) {
        while (can->IFLAG1 & RX_CONS_INTERRUPT) {
            volatile uint32_t *RAMn = MBOX_CONS_RECV(can);
            Mailbox_t mbox = { .u32 = { RAMn[0], RAMn[1], RAMn[2], RAMn[3]} };

            can->IFLAG1 = RX_CONS_INTERRUPT;

            if (mbox.dlc > 0) {
                cons_buf_push(mbox.data0);
                if (mbox.dlc > 1) {
                    cons_buf_push(mbox.data1);
                    if (mbox.dlc > 2) {
                        cons_buf_push(mbox.data2);
                        if (mbox.dlc > 3) {
                            cons_buf_push(mbox.data3);
                            if (mbox.dlc > 4) {
                                cons_buf_push(mbox.data4);
                                if (mbox.dlc > 5) {
                                    cons_buf_push(mbox.data5);
                                    if (mbox.dlc > 6) {
                                        cons_buf_push(mbox.data6);
                                        if (mbox.dlc > 7) {
                                            cons_buf_push(mbox.data7);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CAN0_ORed_0_15_MB_Handler(void) {
    _can_interrupt(CAN0);
}

void CAN1_ORed_0_15_MB_Handler(void) {
    _can_interrupt(CAN1);
}

// Get a single character, blocking until one is available
//
int mp_hal_stdin_rx_chr(void) {
    int c;

    do {
        CONS_BUF_POP(c);
    } while (c == -1);
    return c;
}

// Check for an available byte
//
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if (!CONS_BUF_EMPTY) {
        ret |= MP_STREAM_POLL_RD;
    }
    return ret;
}

static mp_obj_t cc16_can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    mp_uint_t bus_index = mp_obj_get_int(args[0]);
    if (bus_index > NUM_CAN) {
        mp_raise_ValueError(MP_ERROR_TEXT("CAN bus does not exist"));
    }
    return MP_OBJ_FROM_PTR(&cc16_can_obj[bus_index]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(cc16_can_make_new_obj, cc16_can_make_new);

// send(send, addr, *, timeout=5000)
static mp_obj_t cc16_can_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_data, ARG_id, ARG_timeout, ARG_extframe };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_data,     MP_ARG_REQUIRED | MP_ARG_OBJ,   {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,   {.u_int = 0} },
        { MP_QSTR_timeout,  MP_ARG_KW_ONLY  | MP_ARG_INT,   {.u_int = 0} },
        { MP_QSTR_extframe, MP_ARG_BOOL,                    {.u_bool = false} },
    };

    // parse args
    pyb_can_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len > CAN_MAX_DATA_FRAME) {
        mp_raise_ValueError(MP_ERROR_TEXT("CAN data field too long"));
    }

    Mailbox_t mbox;
    mbox.dlc = bufinfo.len;
    mbox.code = TX_DATA;
    if (args[ARG_extframe].u_bool) {
        mbox.ide = 1;
        mbox.id_ext = args[ARG_id].u_int & 0x1fffffff;
    } else {
        mbox_ide = 0;
        mbox.id = args[ARG_id].u_int & 0x7ff;
    }
    switch (bufinfo.len) {
    case 8:
        mbox.data7 = ((byte *)bufinfo.buf)[7];
    case 7:
        mbox.data6 = ((byte *)bufinfo.buf)[6];
    case 6:
        mbox.data5 = ((byte *)bufinfo.buf)[5];
    case 5:
        mbox.data4 = ((byte *)bufinfo.buf)[4];
    case 4:
        mbox.data3 = ((byte *)bufinfo.buf)[3];
    case 3:
        mbox.data2 = ((byte *)bufinfo.buf)[2];
    case 2:
        mbox.data1 = ((byte *)bufinfo.buf)[1];
    case 1:
        mbox.data0 = ((byte *)bufinfo.buf)[0];
        break;
    }

    // loop waiting for a free transmit mailbox
    // XXX timeout? bus errors?
    for (;;) {
        for (mp_uint_t index = MBOX_TX_FIRST; index <= MBOX_TX_LAST; index++) {
            volatile Mailbox_t *txmbx = (volatile Mailbox_t *)MBOX_ADDR(self->regs, index);
            if (txmbx->code == TX_INACTIVE) {
                txmbx.u32[1] = mbox.u32[1];
                txmbx.u32[2] = mbox.u32[2];
                txmbx.u32[3] = mbox.u32[3];
                txmbx.u32[0] = mbox.u32[0];
                break;
            }
        }
    }
    return mp_const_none;
}

static const mp_rom_map_elem_t cc16_can_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&cc16_can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&cc16_can_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter), MP_ROM_PTR(&cc16_can_setfilter_obj) },
    { MP_ROM_QSTR(MP_QSTR_clearfilter), MP_ROM_PTR(&cc16_can_clearfilter_obj) },
};
static MP_DEFINE_CONST_DICT(cc16_can_locals_dict, cc16_can_locals_dict_table);


const mp_obj_type_t cc16_can_type = {
    { &mp_type_type },
    .name = MP_QSTR_CAN,
    .print = cc16_can_print,
    .make_new = cc16_can_make_new,
    .locals_dict = (mp_obj_dict_t *)&cc16_can_locals_dict,
};

