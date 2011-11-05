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
 * "FILE" class
 */
class NXSL_FileClass : public NXSL_Class
{
public:
   NXSL_FileClass();

   virtual NXSL_Value *getAttr(NXSL_Object *object, const TCHAR *attr);
};

/**
 * NXSL installer environment
 */
class NXSL_InstallerEnvironment : public NXSL_Environment
{
public:
	NXSL_InstallerEnvironment();

	virtual void trace(int level, const TCHAR *text);
};

/**
 * NXSL external functions
 */
int F_access(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_chdir(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_CopyFile(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_DeleteFile(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_fopen(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_fclose(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_feof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_fgets(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_fputs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_mkdir(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_RenameFile(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_rmdir(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_system(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);

/**
 * Global variables
 */
extern int g_traceLevel;
extern NXSL_FileClass g_nxslFileClass;

#endif
