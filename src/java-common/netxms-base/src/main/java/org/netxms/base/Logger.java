/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.base;

/**
 * Logging class for client library
 */
public class Logger
{
	private static LoggingFacility facility = null;
	
	/**
	 * Set logging facility. If facility is null, logging will be disabled.
	 * 
	 * @param facility logging facility or null
	 */
	public static void setLoggingFacility(LoggingFacility facility)
	{
		Logger.facility = facility;
	}
	
	/**
	 * Write message to log using current logging facility.
	 * 
	 * @param level severity level (defined in interface LoggingFacility)
	 * @param tag message tag
	 * @param message message text
	 * @param t associated throwable, or null
	 */
	public static void writeLog(int level, String tag, String message, Throwable t)
	{
		if (facility != null)
			facility.writeLog(level, tag, message, t);
	}
	
	/**
	 * Write DEBUG message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 * @param t associated throwable, or null
	 */
	public static void debug(String tag, String message, Throwable t)
	{
		writeLog(LoggingFacility.DEBUG, tag, message, t);
	}
	
	/**
	 * Write DEBUG message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 */
	public static void debug(String tag, String message)
	{
		writeLog(LoggingFacility.DEBUG, tag, message, null);
	}
	
	/**
	 * Write INFO message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 * @param t associated throwable, or null
	 */
	public static void info(String tag, String message, Throwable t)
	{
		writeLog(LoggingFacility.INFO, tag, message, t);
	}
	
	/**
	 * Write INFO message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 */
	public static void info(String tag, String message)
	{
		writeLog(LoggingFacility.INFO, tag, message, null);
	}
	
	/**
	 * Write WARNING message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 * @param t associated throwable, or null
	 */
	public static void warning(String tag, String message, Throwable t)
	{
		writeLog(LoggingFacility.WARNING, tag, message, t);
	}
	
	/**
	 * Write WARNING message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 */
	public static void warning(String tag, String message)
	{
		writeLog(LoggingFacility.WARNING, tag, message, null);
	}
	
	/**
	 * Write ERROR message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 * @param t associated throwable, or null
	 */
	public static void error(String tag, String message, Throwable t)
	{
		writeLog(LoggingFacility.ERROR, tag, message, t);
	}
	
	/**
	 * Write ERROR message to log
	 * 
	 * @param tag message tag
	 * @param message message text
	 */
	public static void error(String tag, String message)
	{
		writeLog(LoggingFacility.ERROR, tag, message, null);
	}
}
