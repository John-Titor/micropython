/*
 * Basic setjmp / longjmp compatible with the newlib jmp_buf prototype.
 */

.global setjmp
setjmp:
        moveal  %sp@(4), %a0
        movel   %sp@(0), %a0@(12)
        movel   %sp, %a0@(8)
        moveml  %d2-%d7/%a2-%a6, %a0@(20)
#if M68K_HAVE_FPU == 1
        fmovemx %fp2-%fp7, %a0@(64)
#endif
        clrl    %d0
        rts

.global longjmp
longjmp:
        moveal  %sp@(4), %a0
#if M68K_HAVE_FPU == 1
        fmovemx %a0@(64), %fp2-%fp7
#endif
        movel   %sp@(8), %d0
        moveml  %a0@(20), %d2-%d7/%a2-%a6
        moveal  %a0@(8), %sp
        movel   %a0@(12), %sp@
        rts
