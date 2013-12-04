;
; ldcw.s
;
; Provides __ldcw function for PA-RISC
;
; Based on http://h21007.www2.hp.com/portal/download/files/unprot/itanium/spinlocks.pdf
;

        .code
        .export __ldcw,entry,priv_lev=3,rtnval=gr
   
__ldcw
        .proc 
        .callinfo no_calls
        .enter
   
        addi    15,%arg0,%arg2
        depi    0,31,4,%arg2
        bv      (%r2)
        ldcws,co (%arg2),%ret0

; This loop is never executed and needed for reduce
; prefetching after mispredicted bv on pre PA 8700 processors.
ldcw_loop
        b       ldcw_loop
        nop
        .leave
        .procend
