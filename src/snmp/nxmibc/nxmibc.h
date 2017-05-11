/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005 Victor Kirhenshtein
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
** File: nxmibc.h
**
**/

#ifndef _nxmibc_h_
#define _nxmibc_h_

#ifdef __cplusplus

#include <nms_common.h>
#include <nms_util.h>
#include <nxsnmp.h>
#include <stdarg.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#endif


//
// Error codes
//

#define ERR_UNRESOLVED_IMPORT    1
#define ERR_UNRESOLVED_MODULE    2
#define ERR_PARSER_ERROR         3
#define ERR_CANNOT_OPEN_FILE     4
#define ERR_UNRESOLVED_SYMBOL    5
#define ERR_UNRESOLVED_SYNTAX    6


//
// Functions
//

#ifdef __cplusplus
extern "C" {
#endif

void Error(int nError, const char *pszModule, ...);

#ifdef __cplusplus
}
#endif

#endif
