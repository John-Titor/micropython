/*
 * Native Features interface.
 *
 * Provides access to emulator functions when run under an emulator that implements NatFeat.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <py/mpconfig.h>

#include "mpconfigport.h"

#if M68K_WITH_NATIVE_FEATURES

/* not actually extern */
extern uint32_t     _nfID(const char *);
extern uint32_t     _nfCall(uint32_t ID, ...);

__asm__ (
    "_nfID:                                                                 \n"
    "        .dc.w   0x7300                                                 \n"
    "        rts                                                            \n"
    "_nfCall:                                                               \n"
    "        .dc.w   0x7301                                                 \n"
    "        rts                                                            \n"
    );

void
m68k_nf_puts(const char *str) {
    static int nfid_stderr = 0;

    if (nfid_stderr == 0) {
        nfid_stderr = _nfID("NF_STDERR");
    }

    if (nfid_stderr > 0) {
        _nfCall(nfid_stderr, str);
    }
}

void
m68k_nf_shutdown(void) {
    int nfid_shutdown = _nfID("NF_SHUTDOWN");
    _nfCall(nfid_shutdown);
    for (;;) {
    }
}

#if M68K_WITH_NATFEAT_STDERR

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    // XXX inefficient
    while (len--) {
        char buf[2] = {*str++, 0};
        m68k_nf_puts(buf);
    }
}
#endif /* M68K_WITH_NATFEAT_STDERR */
#endif /* M68K_WITH_NATIVE_FEATURES */
