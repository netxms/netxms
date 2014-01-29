; 
; libnetxms - Common NetXMS utility library
; Copyright (C) 2003-2012 Victor Kirhenshtein
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU Lesser General Public License as published
; by the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU Lesser General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
; File: ldcw.s
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
