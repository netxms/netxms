/* 
** NetXMS - Network Management System
** Command line event sender
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxevent.cpp
**
**/

#include "nxevent.h"


//
// Static data
//

static WORD m_wServerPort = 4701;


//
// Entry point
//

int main(int argc, char *argv[])
{
   int ch;
   BOOL bStart = TRUE;

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:bi:hlnp:qs:vw:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxevent [<options>] <server> <event_id> [<param_1> [... <param_N>]]\n"
                   "Valid options are:\n"
                   "   -h           : Display help and exit.\n"
                   "   -p <port>    : Specify server's port number. Default is %d.\n"
                   "   -w <seconds> : Specify command timeout (default is 3 seconds)\n"
                   "\n", m_wServerPort);
            bStart = FALSE;
            break;
         case '?':
            bStart = FALSE;
            break;
         default:
            break;
      }
   }

   // Do requested action if everything is OK
   if (bStart)
   {
   }

   return 0;
}
