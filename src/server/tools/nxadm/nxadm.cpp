/* 
** NetXMS - Network Management System
** Local administration tool
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
** $module: nxadm.cpp
**
**/

#include "nxadm.h"


//
// Display help
//

static void Help(void)
{
   printf("NetXMS Administartor Tool  Version 1.0\n"
          "Copyright (c) 2004 NetXMS Development Team\n\n"
          "Usage: nxadm <command> [<command-specific arguments>]\n\n"
          "Possible commands are:\n"
          "   cfg   : Manage configuration variables\n"
          "   flags : Manage application flags\n"
          "   help  : Get more specific help\n"
          "\n");
}


//
// Handler for "cfg" command
//

static int H_Config(int argc, char *argv[])
{
   int iError = 2;   // Default is communication error

   if (argc == 0)
   {
      printf("Usage: nxadm cfg <command>\n\n"
             "Valid commands are:\n"
             "   list              : Enumerate all configuration variables\n"
             "   get <var>         : Get value of specific variable\n"
             "   set <var> <value> : Set value of specific variable\n");
      return 1;
   }

   if (!stricmp(argv[0], "get"))
   {
      if (argc == 1)
      {
         printf("ERROR: Variable name missing\n");
         iError = 1;
      }
      else
      {
         if (SendCommand(LA_CMD_GET_CONFIG))
            if (SendString(argv[1]))
            {
               int iCode;
               char szBuffer[256];

               iCode = RecvString(szBuffer, 256);
               switch(iCode)
               {
                  case 0:     // OK
                     printf("%s\n", szBuffer);
                     iError = 0;
                     break;
                  case 1:
                     printf("ERROR: Variable %s doesn't exist\n", argv[1]);
                     iError = 3;
                     break;
                  default:
                     break;
               }
            }
      }
   }
   else if (!stricmp(argv[0], "set"))
   {
      if (argc < 3)
      {
         printf("ERROR: Variable name or value missing\n");
         iError = 1;
      }
      else
      {
         if (SendCommand(LA_CMD_SET_CONFIG))
            if (SendString(argv[1]))
               if (SendString(argv[2]))
               {
                  if (RecvResponce() == LA_RESP_SUCCESS)
                  {
                     printf("Successful\n");
                     iError = 0;
                  }
                  else
                  {
                     printf("ERROR: Server was unable to set configuration variable\n");
                     iError = 3;
                  }
               }
      }
   }
   else
   {
      printf("ERROR: Invalid command syntax\n");
      iError = 1;
   }
   if (iError == 2)
      printf("ERROR: Client-server communication failed\n");
   return iError;
}


//
// Handler for "flags" command
//

static int H_Flags(int argc, char *argv[])
{
   int iError = 2;   // Default is communication error

   if (argc == 0)
   {
      printf("Usage: nxadm flags <command>\n\n"
             "Valid commands are:\n"
             "   get        : Get current application flags value\n"
             "   set<value> : Set application flags\n");
      return 1;
   }

   if (!stricmp(argv[0], "get"))
   {
      DWORD dwFlags;

      if (SendCommand(LA_CMD_GET_FLAGS))
         if (RecvDWord(&dwFlags))
         {
            printf("Current flags: 0x%08X\n", dwFlags);
            iError = 0;
         }
   }
   else if (!stricmp(argv[0], "set"))
   {
      if (argc == 1)
      {
         printf("ERROR: New flags value missing\n");
         iError = 1;
      }
      else
      {
         DWORD dwFlags;
         char *eptr;

         dwFlags = strtoul(argv[1], &eptr, 0);
         if (*eptr != 0)
         {
            printf("ERROR: Invalid numeric format\n");
            iError = 1;
         }
         else
         {
            if (SendCommand(LA_CMD_SET_FLAGS))
               if (SendDWord(dwFlags))
                  if (RecvResponce() == LA_RESP_SUCCESS)
                  {
                     printf("Successful\n");
                     iError = 0;
                  }
         }
      }
   }
   else
   {
      printf("ERROR: Invalid command syntax\n");
      iError = 1;
   }
   if (iError == 2)
      printf("ERROR: Client-server communication failed\n");
   return iError;
}


//
// Entry point
//

int main(int argc, char *argv[])
{
   int iError;
   static struct
   {
      char szName[16];
      int (* pHandler)(int, char *[]);
   } cmdList[] = 
   {
      { "cfg", H_Config },
      { "flags", H_Flags },
      { "", NULL }
   };

   if (argc > 1)
   {
#ifdef _WIN32
      WSADATA wsaData;

      WSAStartup(0x0002, &wsaData);
#endif

      if (Connect())
      {
         int i;

         for(i = 0; cmdList[i].pHandler != NULL; i++)
            if (!stricmp(cmdList[i].szName, argv[1]))
            {
               iError = cmdList[i].pHandler(argc - 2, &argv[2]);
               break;
            }
         if (cmdList[i].pHandler == NULL)
         {
            Help();
            iError = 1;
         }
         Disconnect();
      }
      else
      {
         iError = 2;
      }
   }
   else
   {
      Help();
      iError = 1;
   }
   return iError;
}
