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
