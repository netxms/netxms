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
** $module: config.cpp
**
**/

#include "nms_core.h"


//
// Help text
//

static char help_text[]="NMS Version " VERSION_STRING " Server\n"
                        "Copyright (c) 2003 SecurityProjects.org\n\n"
                        "Usage: nms_core [<options>] <command>\n\n"
                        "Valid options are:\n"
                        "   --config <file>  : Set non-default configuration file\n"
                        "                    : Default is " DEFAULT_CONFIG_FILE "\n"
                        "\n"
                        "Valid commands are:\n"
                        "   check-config     : Check configuration file syntax\n"
#ifdef _WIN32
                        "   install          : Install Win32 service\n"
                        "   install-events   : Install Win32 event source\n"
#endif
                        "   help             : Display help and exit\n"
#ifdef _WIN32
                        "   remove           : Remove Win32 service\n"
                        "   remove-events    : Remove Win32 event source\n"
                        "   standalone       : Run in standalone mode (not as service)\n"
#endif
                        "   version          : Display version and exit\n"
                        "\n";


//
// Load and parse configuration file
// Returns 0 on success and -1 on failure
//

int LoadConfig(void)
{
   return 0;
}


//
// Parse command line
// Returns 0 on success and -1 on failure
//

int ParseCommandLine(int argc, char *argv[])
{
   int i;

   for(i = 1; i < argc; i++)
   {
      if (!strcmp(argv[i], "help"))    // Display help and exit
      {
         printf(help_text);
         return -1;
      }
      else if (!strcmp(argv[i], "version"))    // Display version and exit
      {
         printf("NMS Version " VERSION_STRING " Build of " __DATE__ "\n");
         return -1;
      }
      else if (!strcmp(argv[i], "--config"))  // Config file
      {
         i++;
         strcpy(g_configFile, argv[i]);     // Next word should contain name of the config file
      }
      else if (!strcmp(argv[i], "check-config"))
      {
         g_flags |= CF_STANDALONE;
         printf("Checking configuration file (%s):\n\n", g_configFile);
         LoadConfig();
         return -1;
      }
#ifdef _WIN32
      else if (!strcmp(argv[i], "standalone"))  // Run in standalone mode
      {
         g_flags |= CF_STANDALONE;
         return TRUE;
      }
      else if ((!strcmp(argv[i], "install"))||
               (!strcmp(argv[i], "install-events")))
      {
         char path[MAX_PATH], *ptr;

         ptr = strrchr(argv[0], '\\');
         if (ptr != NULL)
            ptr++;
         else
            ptr = argv[0];

         _fullpath(path, ptr, 255);

         if (stricmp(&path[strlen(path)-4], ".exe"))
            strcat(path, ".exe");

         if (!strcmp(argv[i], "install"))
            InstallService(path);
         else
            InstallEventSource(path);
         return FALSE;
      }
      else if (!strcmp(argv[i], "remove"))
      {
         RemoveService();
         return FALSE;
      }
      else if (!strcmp(argv[i], "remove-events"))
      {
         RemoveEventSource();
         return FALSE;
      }
      else if (!strcmp(argv[i], "start"))
      {
         StartCoreService();
         return FALSE;
      }
      else if (!strcmp(argv[i], "stop"))
      {
         StopCoreService();
         return FALSE;
      }
#endif   /* _WIN32 */
      else
      {
         printf("ERROR: Invalid command line argument\n\n");
         return -1;
      }
   }

   return 0;
}
