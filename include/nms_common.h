/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: nms_common.h
**
**/

#ifndef _nms_common_h_
#define _nms_common_h_

#ifndef _WIN32

typedef int BOOL
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef void * HANDLE;
typedef void * HMODULE;

#define TRUE   1
#define FALSE  0

#endif


//
// Interface types
//

#define IFTYPE_OTHER                1
#define IFTYPE_REGULAR1822          2
#define IFTYPE_HDH1822              3
#define IFTYPE_DDN_X25              4
#define IFTYPE_RFC877_X25           5
#define IFTYPE_ETHERNET_CSMACD      6
#define IFTYPE_ISO88023_CSMACD      7
#define IFTYPE_ISO88024_TOKENBUS    8
#define IFTYPE_ISO88025_TOKENRING   9
#define IFTYPE_ISO88026_MAN         10
#define IFTYPE_STARLAN              11
#define IFTYPE_PROTEON_10MBIT       12
#define IFTYPE_PROTEON_80MBIT       13
#define IFTYPE_HYPERCHANNEL         14
#define IFTYPE_FDDI                 15
#define IFTYPE_LAPB                 16
#define IFTYPE_SDLC                 17
#define IFTYPE_DS1                  18
#define IFTYPE_E1                   19
#define IFTYPE_BASIC_ISDN           20
#define IFTYPE_PRIMARY_ISDN         21
#define IFTYPE_PROP_PTP_SERIAL      22
#define IFTYPE_PPP                  23
#define IFTYPE_SOFTWARE_LOOPBACK    24
#define IFTYPE_EON                  25
#define IFTYPE_ETHERNET_3MBIT       26
#define IFTYPE_NSIP                 27
#define IFTYPE_SLIP                 28
#define IFTYPE_ULTRA                29
#define IFTYPE_DS3                  30
#define IFTYPE_SMDS                 31
#define IFTYPE_FRAME_RELAY          32


#endif   /* _nms_common_h_ */
