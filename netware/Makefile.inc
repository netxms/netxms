#
# Common make options for Novell NetWare
# Copyright (c) 2004 Victor Kirhenshtein
#

CFLAGS   = -nostdinc -fno-exceptions -fno-builtin -fpack-struct -fno-rtti \
           -c -D_NETWARE -D_POSIX_SOURCE -ID:/nwsdk/libc/include \
           -ID:/nwsdk/fs64/include \
	   -I../../include -I../../../include \
           -I../../../../include -I../libexpat/libexpat
LDFLAGS  = --linker=i586-netware-ld.exe --output-target=nlm32-i386 --input-target=elf32-i386
PLDFLAGS = -r

CC       = i586-netware-g++
PRELINK  = i586-netware-ld
LINK     = i586-netware-nlmconv
PERL	 = perl
