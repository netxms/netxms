#
# Common make options for Novell NetWare
# Copyright (c) 2004 Victor Kirhenshtein
#

CFLAGS   = -nostdinc -fno-builtin -fpack-struct -c -D_NETWARE -I../../include -IC:/novell/ndk/libc/include
LDFLAGS  = --linker=c:/gcc-nlm/usr/bin/i586-netware-ld.exe --output-target=nlm32-i386 --input-target=elf32-i386
PLDFLAGS = -r -Lc:/gcc-nlm/usr/i586-netware/lib -Lc:/gcc-nlm/usr/lib/gcc-lib/i586-netware/3.2.3 -lstdc++ -lsupc++ -lgcc

CC       = i586-netware-g++
PRELINK  = i586-netware-ld
LINK     = i586-netware-nlmconv
