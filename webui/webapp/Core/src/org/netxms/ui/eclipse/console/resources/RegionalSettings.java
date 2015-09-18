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
	
	private static int dateTimeFormat = DATETIME_FORMAT_SERVER;
	private static String dateFormatString;
	private static String timeFormatString;
	private static String shortTimeFormatString;
	
	/**
	 * Update from preferences
	 */
	public static void updateFromPreferences()
	{
		IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		dateTimeFormat = ps.getInt("DATETIME_FORMAT"); //$NON-NLS-1$
		dateFormatString = ps.getString("DATE_FORMAT_STRING"); //$NON-NLS-1$
		timeFormatString = ps.getString("TIME_FORMAT_STRING"); //$NON-NLS-1$
      shortTimeFormatString = ps.getString("SHORT_TIME_FORMAT_STRING"); //$NON-NLS-1$
	}
	
	/**
	 * Get formatter for date and time
	 * 
	 * @return
	 */
	public static DateFormat getDateTimeFormat()
	{
		switch(dateTimeFormat)
		{
			case DATETIME_FORMAT_SERVER:
				NXCSession session = ConsoleSharedData.getSession();
				return new SimpleDateFormat(session.getDateFormat() + " " + session.getTimeFormat()); //$NON-NLS-1$
			case DATETIME_FORMAT_CUSTOM:
				try
				{
					return new SimpleDateFormat(dateFormatString + " " + timeFormatString); //$NON-NLS-1$
				}
				catch(IllegalArgumentException e)
				{
					return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM);
				}
			default:
				return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM);
		}
	}

	/**
	 * Get formatter for date only
	 * 
	 * @return
	 */
	public static DateFormat getDateFormat()
	{
		switch(dateTimeFormat)
		{
			case DATETIME_FORMAT_SERVER:
				NXCSession session = ConsoleSharedData.getSession();
				return new SimpleDateFormat(session.getDateFormat());
			case DATETIME_FORMAT_CUSTOM:
				try
				{
					return new SimpleDateFormat(dateFormatString);
				}
				catch(IllegalArgumentException e)
				{
					return DateFormat.getDateInstance(DateFormat.SHORT);
				}
			default:
				return DateFormat.getDateInstance(DateFormat.SHORT);
		}
	}

   /**
    * Get formatter for time only
    * 
    * @return
    */
   public static DateFormat getTimeFormat()
   {
      switch(dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = ConsoleSharedData.getSession();
            return new SimpleDateFormat(session.getTimeFormat());
         case DATETIME_FORMAT_CUSTOM:
            try
            {
               return new SimpleDateFormat(timeFormatString);
            }
            catch(IllegalArgumentException e)
            {
               return DateFormat.getTimeInstance(DateFormat.MEDIUM);
            }
         default:
            return DateFormat.getTimeInstance(DateFormat.MEDIUM);
      }
   }

   /**
    * Get formatter for time only (short form)
    * 
    * @return
    */
   public static DateFormat getShortTimeFormat()
   {
      switch(dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = (NXCSession)ConsoleSharedData.getSession();
            return new SimpleDateFormat(session.getShortTimeFormat());
         case DATETIME_FORMAT_CUSTOM:
            try
            {
               return new SimpleDateFormat(shortTimeFormatString);
            }
            catch(IllegalArgumentException e)
            {
               return DateFormat.getTimeInstance(DateFormat.SHORT);
            }
         default:
            return DateFormat.getTimeInstance(DateFormat.SHORT);
      }
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
