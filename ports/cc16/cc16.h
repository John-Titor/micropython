
#ifndef MICROPY_INCLUDED_CC16_H
#define MICROPY_INCLUDED_CC16_H

#include "s32k144.h"

extern void main(void);
extern void cc16_init(void);
extern void s32k_can_early_init(void);
extern int s32k_can_recv_console(void);

#endif // MICROPY_INCLUDED_CC16_H