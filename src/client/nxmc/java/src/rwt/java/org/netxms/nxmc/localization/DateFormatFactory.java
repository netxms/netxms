/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.localization;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;
import org.eclipse.rap.rwt.RWT;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.TimeFormatter;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.xnap.commons.i18n.I18n;

/**
 * Factory class to provide locale-correct date and time formatters
 */
public class DateFormatFactory
{
   public static final int DATETIME_FORMAT_SERVER = 0;
   public static final int DATETIME_FORMAT_JVM = 1;
   public static final int DATETIME_FORMAT_CUSTOM = 2;

   /**
    * Internal implementation of TimeFormatter interface
    */
   private static final TimeFormatter TIME_FORMATTER = new TimeFormatter() {
      @Override
      public String formatUptime(long seconds)
      {
         return formatTimeDifference(seconds, true);
      }

      @Override
      public String formatDateAndTime(long timestamp)
      {
         return getDateTimeFormat().format(new Date(timestamp * 1000));
      }
   };

   private I18n i18n = LocalizationHelper.getI18n(DateFormatFactory.class);
   private int dateTimeFormat = DATETIME_FORMAT_SERVER;
   private String dateFormatString;
   private String timeFormatString;
   private String shortTimeFormatString;

   /**
    * Create instance for current session
    */
   public static void createInstance()
   {
      PreferenceStore ps = PreferenceStore.getInstance();
      DateFormatFactory instance = new DateFormatFactory();
      instance.dateTimeFormat = ps.getAsInteger("DateFormatFactory.Format.DateTime", DATETIME_FORMAT_SERVER);
      instance.dateFormatString = ps.getAsString("DateFormatFactory.Format.Date");
      instance.timeFormatString = ps.getAsString("DateFormatFactory.Format.Time");
      instance.shortTimeFormatString = ps.getAsString("DateFormatFactory.Format.ShortTime");
      RWT.getUISession().setAttribute("netxms.dateFormatFactory", instance);
      if (ps.getAsBoolean("DateFormatFactory.UseServerTimeZone", false))
         Registry.setServerTimeZone();
      else
         Registry.resetTimeZone();
   }

   /**
    * Update from preferences
    */
   public static void updateFromPreferences()
   {
      PreferenceStore ps = PreferenceStore.getInstance();
      DateFormatFactory instance = getInstance();
      instance.dateTimeFormat = ps.getAsInteger("DateFormatFactory.Format.DateTime", DATETIME_FORMAT_SERVER);
      instance.dateFormatString = ps.getAsString("DateFormatFactory.Format.Date");
      instance.timeFormatString = ps.getAsString("DateFormatFactory.Format.Time");
      instance.shortTimeFormatString = ps.getAsString("DateFormatFactory.Format.ShortTime");
      if (ps.getAsBoolean("DateFormatFactory.UseServerTimeZone", false))
         Registry.setServerTimeZone();
      else
         Registry.resetTimeZone();
   }

   /**
    * Get time formatter
    *
    * @return time formatter instance
    */
   public static TimeFormatter getTimeFormatter()
   {
      return TIME_FORMATTER;
   }

   /**
    * Get instance for current session
    *
    * @return instance
    */
   private static DateFormatFactory getInstance()
   {
      return (DateFormatFactory)RWT.getUISession().getAttribute("netxms.dateFormatFactory");
   }

   /**
    * Get formatter for date and time
    * 
    * @return
    */
   public static DateFormat getDateTimeFormat()
   {
      DateFormatFactory instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = Registry.getSession();
            df = new SimpleDateFormat(session.getDateFormat() + " " + session.getTimeFormat());
            break;
         case DATETIME_FORMAT_CUSTOM:
            try
            {
               df = new SimpleDateFormat(instance.dateFormatString + " " + instance.timeFormatString);
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
      TimeZone tz = Registry.getTimeZone();
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
      DateFormatFactory instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = Registry.getSession();
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
      TimeZone tz = Registry.getTimeZone();
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
      DateFormatFactory instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = Registry.getSession();
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
      TimeZone tz = Registry.getTimeZone();
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
      DateFormatFactory instance = getInstance();
      DateFormat df;
      switch(instance.dateTimeFormat)
      {
         case DATETIME_FORMAT_SERVER:
            NXCSession session = Registry.getSession();
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
      TimeZone tz = Registry.getTimeZone();
      if (tz != null)
         df.setTimeZone(tz);
      return df;
   }

   /**
    * Format provided time difference as [n days, ]hh:mm[:ss]
    * 
    * @param seconds number of seconds
    * @param showSeconds true to show seconds
    * @return formatted time
    */
   public static String formatTimeDifference(long seconds, boolean showSeconds)
   {
      StringBuilder sb = new StringBuilder();
      long days = seconds / 86400;
      if (days > 0)
      {
         DateFormatFactory instance = getInstance();
         sb.append(instance.i18n.trn("{0} day", "{0} days", days, days));
         sb.append(", ");
         seconds -= days * 86400;
      }

      long hours = seconds / 3600;
      if (hours < 10)
         sb.append('0');
      sb.append(hours);
      seconds -= hours * 3600;

      sb.append(':');
      long minutes = seconds / 60;
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

   /**
    * Format time difference between current and give time as [n days, ]hh:mm[:ss]
    * 
    * @param start period start time
    * @param showSeconds true to show seconds
    * @return formatted time difference
    */
   public static String formatTimeDifference(Date start, boolean showSeconds)
   {
      long seconds = ((System.currentTimeMillis() - start.getTime()) / 1000);
      return formatTimeDifference(seconds, showSeconds);
   }
}
