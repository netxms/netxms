#
# Common make options for Novell NetWare
# Copyright (c) 2004 Victor Kirhenshtein
#

CFLAGS   = -nostdinc -fno-exceptions -fno-builtin -fpack-struct -c -D_NETWARE -I../../include -I../../../include -IC:/novell/ndk/libc/include
LDFLAGS  = --linker=c:/gcc-nlm/usr/bin/i586-netware-ld.exe --output-target=nlm32-i386 --input-target=elf32-i386
PLDFLAGS = -r

CC       = i586-netware-g++
PRELINK  = i586-netware-ld
LINK     = i586-netware-nlmconv
PERL	 = perl
