/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.reporting.tools;

import org.netxms.reporting.Server;

/**
 * Thread-local information for currently running report
 */
public final class ThreadLocalReportInfo
{
   private static final ThreadLocal<String> location = new ThreadLocal<String>();
   private static final ThreadLocal<Server> server = new ThreadLocal<Server>();

   public static String getReportLocation()
   {
      return location.get();
   }

   public static void setReportLocation(final String location)
   {
      ThreadLocalReportInfo.location.set(location);
   }

   public static Server getServer()
   {
      return server.get();
   }

   public static void setServer(Server server)
   {
      ThreadLocalReportInfo.server.set(server);
   }
}
