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
** $module: main.cpp
**
**/

#include "nms_core.h"


//
// Global variables
//

DWORD g_flags = 0;
char g_configFile[256] = DEFAULT_CONFIG_FILE;


//
// Server initialization
//

BOOL Initialize(void)
{
   return TRUE;
}


//
// Server shutdown
//

void Shutdown(void)
{
}


//
// Common main()
//

void Main(void)
{
}


//
// Startup code
//

int main(int argc, char *argv[])
{
   if (ParseCommandLine(argc, argv) == -1)
      return 1;

   if (LoadConfig() == -1)
      return 1;

   if (!IsStandalone())
   {
      InitService();
   }
   else
   {
      if (!Initialize())
      {
         printf("NMS Core initialization failed\n");
         return 1;
      }
      Main();
   }
   return 0;
}
