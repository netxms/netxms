/**
 * NetXMS Java Bridge
 * Copyright (C) 2013 TEMPEST a.s.
 * Copyright (C) 2014-2017 Raden Solutions
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
package org.netxms.bridge;

/**
 * Platform utility class
 */
public final class Platform
{
   /**
    * Get NetXMS directory (internal bridge to native code)
    * 
    * @param type directory type
    * @return path to directory or empty string on error
    */
   protected static native String getNetXMSDirectoryInternal(int type);

   /**
    * Write log message using NetXMS logging facility
    * 
    * @param level log level
    * @param message message
    */
   protected static native void writeLog(int level, String message);

   /**
    * Write debug log message using NetXMS loggin facility
    * 
    * @param level debug level (0-9)
    * @param message message
    */
   public static native void writeDebugLog(int level, String message);

   /**
    * Wrapper for native writeLog call
    * 
    * @param level log level
    * @param message message text
    */
   public static void writeLog(LogLevel level, String message)
   {
      writeLog(level.getValue(), message);
   }

   /**
    * Write exception's stack trace to debug log
    * 
    * @param level log level
    * @param prefix message prefix
    * @param e exception to log
    */
   public static void writeDebugLog(int level, String prefix, Throwable e)
   {
      for(StackTraceElement s : e.getStackTrace())
      {
         writeDebugLog(level, prefix + s.toString());
      }
      if (e.getCause() != null)
      {
         writeDebugLog(level, prefix.trim() + " Caused by: " + e.getCause().getClass().getCanonicalName() + ": " + e.getCause().getMessage());
         writeDebugLog(level, prefix, e.getCause());
      }
   }
   
   /**
    * Get NetXMS directory
    * 
    * @param type directory type
    * @return path to directory or empty string on error
    */
   public static String getNetXMSDirectory(DirectoryType type)
   {
      return getNetXMSDirectoryInternal(type.getValue());
   }
}
