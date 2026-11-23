/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: applink.c
**
**/

/*
 * 32-bit OpenSSL built with the "mingw" target has uplink enabled and terminates the
 * process on the first FILE* operation unless the executable exports OPENSSL_Applink.
 * Compiled as C because applink.c assigns function pointers to void*.
 *
 * TODO: remove once x86 OpenSSL is rebuilt with no-uplink (private/netxms#59)
 */
#include <openssl/applink.c>
