/* 
** NetXMS - Network Management System
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
** $module: nxsl.h
**
**/

#ifndef _nxsl_h_
#define _nxsl_h_

#ifdef _WIN32
#ifdef LIBNXSL_EXPORTS
#define LIBNXSL_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSL_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSL_EXPORTABLE
#endif


//
// Class representing compiled NXSL program
//

class LIBNXSL_EXPORTABLE NXSL_Program
{
public:
   NXSL_Program(void);
   ~NXSL_Program();

   int Run(void);
};


//
// Functions
//

NXSL_Program LIBNXSL_EXPORTABLE *NXSLCompileScript(TCHAR *pszSource,
                                                   TCHAR *pszError, int nBufSize);


#endif
