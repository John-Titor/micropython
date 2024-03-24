//
// Support for the 'machine' module.
//

// boards may not define this...
#ifndef M68K_MIN_IPL
# define M68K_MIN_IPL 0
#endif

static void mp_machine_idle(void) {
    uint16_t sr = m68k_get_sr();        // preserve SR across stop
    __asm__ volatile (
        "stop #0x2" __XSTR(M68K_MIN_IPL) "00 \n"                  // XXX not correct for M68K_MIN_IPL != 0, needs preprocessor magic
        :
        :
        : "memory"
        );
    m68k_set_sr(sr);                    // restore SR
}