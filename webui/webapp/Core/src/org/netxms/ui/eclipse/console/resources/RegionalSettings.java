/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.resources;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import org.eclipse.jface.preference.IPreferenceStore;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Helper class to deal with regional and language settings
 */
public class RegionalSettings
{
	public static final int DATETIME_FORMAT_SERVER = 0;
	public static final int DATETIME_FORMAT_JVM = 1;
	public static final int DATETIME_FORMAT_CUSTOM = 2;
	
	private int dateTimeFormat = DATETIME_FORMAT_SERVER;
	private String dateFormatString;
	private String timeFormatString;
	private String shortTimeFormatString;
	
	/**
	 * Private constructor
	 */
	private RegionalSettings()
	{
	}
	
	/**
	 * Get regional settings instance for current session
	 * 
	 * @return
	 */
	private static RegionalSettings getInstance()
	{
	   RegionalSettings instance = (RegionalSettings)ConsoleSharedData.getProperty("RegionalSettings");
	   if (instance == null)
	   {
	      instance = new RegionalSettings();
	      ConsoleSharedData.setProperty("RegionalSettings", instance);
	   }
	   return instance;
	}
	
	/**
	 * Update from preferences
	 */
	public static void updateFromPreferences()
	{
		IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		RegionalSettings instance = getInstance();
		instance.dateTimeFormat = ps.getInt("DATETIME_FORMAT"); //$NON-NLS-1$
		instance.dateFormatString = ps.getString("DATE_FORMAT_STRING"); //$NON-NLS-1$
		instance.timeFormatString = ps.getString("TIME_FORMAT_STRING"); //$NON-NLS-1$
		instance.shortTimeFormatString = ps.getString("SHORT_TIME_FORMAT_STRING"); //$NON-NLS-1$
      if (ps.getBoolean("USE_SERVER_TIMEZONE"))
         ConsoleSharedData.setServerTimeZone();
      else
         ConsoleSharedData.resetTimeZone();
	}
	
	/**
	 * Get formatter for date and time
	 * 
	 * @return
	 */
	public static DateFormat getDateTimeFormat()
	{
      RegionalSettings instance = getInstance();
	   DateFormat df;
		switch(instance.dateTimeFormat)
		{
			case DATETIME_FORMAT_SERVER:
				NXCSession session = ConsoleSharedData.getSession();
				df = new SimpleDateFormat(session.getDateFormat() + " " + session.getTimeFormat()); //$NON-NLS-1$
				break;
			case DATETIME_FORMAT_CUSTOM:
				try
				{
					df = new SimpleDateFormat(instance.dateFormatString + " " + instance.timeFormatString); //$NON-NLS-1$
				}
				catch(IllegalArgumentException e)
				{
					df = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM);
				}
				break;
			default:
				df = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM);
				break;
		}
		TimeZone tz = ConsoleSharedData.getTimeZone();
		if (tz != null)
		   df.setTimeZone(tz);
		return df;
	}

	/**
	 * Get formatter for date only
	 * 
	 * @return
	 */
	public static DateFormat getDateFormat()
	{
      RegionalSettings instance = getInstance();
      DateFormat df;
		switch(instance.dateTimeFormat)
		{
			case DATETIME_FORMAT_SERVER:
				NXCSession session = ConsoleSharedData.getSession();
				df = new SimpleDateFormat(session.getDateFormat());
				break;
			case DATETIME_FORMAT_CUSTOM:
				try
				{
				   df = new SimpleDateFormat(instance.dateFormatString);
				}
				catch(IllegalArgumentException e)
				{
				   df = DateFormat.getDateInstance(DateFormat.SHORT);
				}
            break;
			default:
			   df = DateFormat.getDateInstance(DateFormat.SHORT);
            break;
		}
      TimeZone tz = ConsoleSharedData.getTimeZone();
      if (tz != null)
         df.setTimeZone(tz);
      return df;
	}

   /**
    * Get formatter for time only
    * 
    * @return
    */
   public static DateFormat getTimeFormat()
   {
      RegionalSettings instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = ConsoleSharedData.getSession();
            df = new SimpleDateFormat(session.getTimeFormat());
            break;
         case DATETIME_FORMAT_CUSTOM:
            try
            {
               df = new SimpleDateFormat(instance.timeFormatString);
            }
            catch(IllegalArgumentException e)
            {
               df = DateFormat.getTimeInstance(DateFormat.MEDIUM);
            }
            break;
         default:
            df = DateFormat.getTimeInstance(DateFormat.MEDIUM);
            break;
      }
      TimeZone tz = ConsoleSharedData.getTimeZone();
      if (tz != null)
         df.setTimeZone(tz);
      return df;
   }

   /**
    * Get formatter for time only (short form)
    * 
    * @return
    */
   public static DateFormat getShortTimeFormat()
   {
      RegionalSettings instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = (NXCSession)ConsoleSharedData.getSession();
            df = new SimpleDateFormat(session.getShortTimeFormat());
            break;
         case DATETIME_FORMAT_CUSTOM:
            try
            {
               df = new SimpleDateFormat(instance.shortTimeFormatString);
            }
            catch(IllegalArgumentException e)
            {
               df = DateFormat.getTimeInstance(DateFormat.SHORT);
            }
            break;
         default:
            df = DateFormat.getTimeInstance(DateFormat.SHORT);
            break;
      }
      TimeZone tz = ConsoleSharedData.getTimeZone();
      if (tz != null)
         df.setTimeZone(tz);
      return df;
   }
   
   /**
    * Format time difference between current and give time as
    * [n days, ]hh:mm[:ss]
    * 
    * @param start period start time
    * @param showSeconds true to show seconds
    * @return formatted time difference
    */
   public static String formatTimeDifference(Date start, boolean showSeconds)
   {
      StringBuilder sb = new StringBuilder();
      int seconds = (int)((System.currentTimeMillis() - start.getTime()) / 1000);
      int days = seconds / 86400;
      if (days > 0)
      {
         sb.append(days);
         sb.append(" days, ");
         seconds -= days * 86400;
      }
      
      int hours = seconds / 3600;
      if (hours < 10)
         sb.append('0');
      sb.append(hours);
      seconds -= hours * 3600;
      
      sb.append(':');
      int minutes = seconds / 60;
      if (minutes < 10)
         sb.append('0');
      sb.append(minutes);
      
      if (showSeconds)
      {
         sb.append(':');
         seconds = seconds % 60;
         if (seconds < 10)
            sb.append('0');
         sb.append(seconds);
      }
      
      return sb.toString();
   }
}
