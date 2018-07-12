/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: systemd.cpp
**
**/

#include <nms_util.h>

#if WITH_SYSTEMD
extern "C"
{
#include <systemd/sd-bus.h>
}

/**
 * Find the name of the systemd service given pid (if run as service)
 */
static char *FindServiceName(UINT32 pid)
{
   char path[64];
   snprintf(path, 64, "/proc/%d/cgroup", pid);
   FILE *cgroup = fopen(path, "r");
   char *serviceName = NULL;
   if (cgroup != NULL)
   {
     char fileBuffer[255];
     while(fgets(fileBuffer, 255, cgroup) != NULL)
     {
        serviceName = strstr(fileBuffer, ".service");
        if (serviceName != NULL)
        {
           serviceName[8] = 0;
           while(*serviceName != '/')
              serviceName--;
           serviceName++;
           serviceName = strdup(serviceName);
           break;
        }
     }
     fclose(cgroup);
   }

   return serviceName;
}

/**
 * Restart service run by systemd
 */
bool RestartService(UINT32 pid)
{
   sd_bus_error error = SD_BUS_ERROR_NULL;
   sd_bus_message *m = NULL;
   sd_bus *bus = NULL;
   const char *path;
   int r;
   char *serviceName = FindServiceName(pid);
   bool result = false;

   /* Connect to the system bus */
   r = sd_bus_open_system(&bus);
   if (r < 0)
   {
      _ftprintf(stderr, _T("Failed to connect to system bus: %hs\n"), strerror(-r));
      sd_bus_message_unref(m);
      sd_bus_unref(bus);
      goto finish;
   }

   /* Issue the method call and store the respons message in m */
   r = sd_bus_call_method(bus,
                          "org.freedesktop.systemd1",           /* service to contact */
                          "/org/freedesktop/systemd1",          /* object path */
                          "org.freedesktop.systemd1.Manager",   /* interface name */
                          "RestartUnit",                        /* method name */
                          &error,                               /* object to return error in */
                          &m,                                   /* return message on success */
                          "ss",                                 /* input signature */
                          serviceName,                          /* first argument */
                          "replace");                           /* second argument */
   if (r < 0)
   {
      _ftprintf(stderr, _T("Failed to issue method call: %hs\n"), error.message);
      goto finish;
   }

   /* Parse the response message */
   r = sd_bus_message_read(m, "o", &path);
   if (r < 0)
   {
      _ftprintf(stderr, _T("Failed to parse response message: %hs\n"), strerror(-r));
      goto finish;
   }

   _tprintf(_T("Queued service job as %hs\n"), path);
   result = true;

finish:
   sd_bus_error_free(&error);
   sd_bus_message_unref(m);
   sd_bus_unref(bus);
   free(serviceName);

   return result;
}

#endif
