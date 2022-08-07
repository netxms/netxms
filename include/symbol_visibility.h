/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: symbol_visibility.h
**
**/

#ifndef _symbol_visibility_h_
#define _symbol_visibility_h_

#ifndef _nms_common_h_
#ifndef _WIN32
#include <config.h>
#endif
#endif

#if defined(_WIN32) || DLL_DECLSPEC_SUPPORTED
#define __EXPORT __declspec(dllexport)
#define __IMPORT __declspec(dllimport)
#elif GLOBAL_SPECIFIER_SUPPORTED
#define __EXPORT __global
#define __IMPORT __global
#elif (__GNUC__ >= 4) || __ibmxl__ || VISIBILITY_ATTRIBUTE_SUPPORTED
#define __EXPORT __attribute__ ((visibility ("default")))
#define __IMPORT __attribute__ ((visibility ("default")))
#else
#define __EXPORT
#define __IMPORT
#endif

#ifdef __IBMCPP__
#define __EXPORT_VAR(s) s __EXPORT
#define __IMPORT_VAR(s) s __IMPORT
#else
#define __EXPORT_VAR(s) __EXPORT s
#define __IMPORT_VAR(s) __IMPORT s
#endif

#endif /* _symbol_visibility_h_ */
