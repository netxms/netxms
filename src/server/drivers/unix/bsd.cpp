/*
** NetXMS - Network Management System
** Drivers for BSD systems accessible via SSH
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: bsd.cpp
**
**/

#include "unix.h"

/**
 * Get driver name
 */
const wchar_t *FreeBSDDriver::getName()
{
   return L"FREEBSD";
}

/**
 * Get kernel name reported by "uname -s"
 */
const char *FreeBSDDriver::getKernelName() const
{
   return "FreeBSD";
}

/**
 * Get driver name
 */
const wchar_t *NetBSDDriver::getName()
{
   return L"NETBSD";
}

/**
 * Get kernel name reported by "uname -s"
 */
const char *NetBSDDriver::getKernelName() const
{
   return "NetBSD";
}

/**
 * Get driver name
 */
const wchar_t *OpenBSDDriver::getName()
{
   return L"OPENBSD";
}

/**
 * Get kernel name reported by "uname -s"
 */
const char *OpenBSDDriver::getKernelName() const
{
   return "OpenBSD";
}
