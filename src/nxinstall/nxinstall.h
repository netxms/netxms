/* 
** NetXMS - Network Management System
** NXSL-based installer tool collection
** Copyright (C) 2005-2011 Victor Kirhenshtein
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
** File: nxinstall.h
**
**/

#ifndef _nxinstall_h_
#define _nxinstall_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxsl.h>

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

/**
 * NXSL installer environment
 */
class NXSL_InstallerEnvironment : public NXSL_Environment
{
public:
	NXSL_InstallerEnvironment();

	virtual void trace(int level, const TCHAR *text);
};

int F_chdir(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_system(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);

/**
 * Global variables
 */
extern int g_traceLevel;

#endif
