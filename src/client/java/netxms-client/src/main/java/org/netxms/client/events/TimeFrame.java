/**
 * NetXMS - open source network management system
 * Copyright (C) 2013-2023 Raden Solutions
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
package org.netxms.client.events;

import java.text.DateFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Locale;
import org.netxms.base.NXCPMessage;
import org.netxms.client.ClientLocalizationHelper;

/**
 * Time frame configuration
 * 
 * Server format: time has format: startHours startMinutes endHours endMinutes dddddddd in int BCD format
 * 
 * date has format: 12 bits of months form December to January, 7 bits of week days form Sunday to Monday, 31 dates of month + last
 * bit is last day of month
 */
public class TimeFrame
{
   public static final String[] DAYS_OF_THE_WEEK = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
   public static final String[] MONTHS = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

   private long daysConfiguration;

   private int startMinute;
   private int startHour;
   private int endMinute;
   private int endHour;
   private String daysOfMonth;
   private boolean[] daysOfWeek;
   private boolean[] months;

   /**
    * Default constructor
    */
   public TimeFrame()
   {
      startMinute = 0;
      startHour = 0;
      endMinute = 23;
      endHour = 59;
      daysOfMonth = new String("1-31");
      daysOfWeek = new boolean[7];
      months = new boolean[12];
      daysConfiguration = 0;
   }

   /**
    * Constructor
    * 
    * @param time time form server
    * @param date date from server
    */
   public TimeFrame(int time, long date)
   {
      daysConfiguration = date;

      startHour = (time / 1000000);
      startMinute = (time / 10000) % 100;
      endHour = (time % 10000) / 100;
      endMinute = time % 100;

      StringBuilder days = new StringBuilder();
      int state = 0;
      boolean isLastDay = (date & 1) == 1;
      date >>= 1;
      for(int i = 0; i < 31; i++)
      {
         switch(state)
         {
            case 0:
               if ((date & 1) == 1)
               {
                  if (days.length() != 0)
                     days.append(", ");
                  days.append(i + 1);
                  state = 1;
               }
               break;
            case 1:
               if ((date & 1) == 1 && (date & 2) == 2)
               {
                  state = 2;
                  days.append("-");
               }
               else if ((date & 1) == 1)
               {
                  days.append(", ");
                  days.append(i + 1);
               }
               else
               {
                  state = 0;
               }
               break;
            case 2:
               if ((date & 1) == 1 && ((date & 2) == 0 || i == 30))
               {
                  state = 0;
                  days.append(i + 1);
               }
               break;
         }

         date >>= 1;
      }
      if (isLastDay)
      {
         if (days.length() != 0)
            days.append(", ");
         days.append("L");
      }
      daysOfMonth = days.toString();

      daysOfWeek = new boolean[7];
      for(int i = 0; i < 7; i++)
      {
         daysOfWeek[i] = ((date & 1) == 1);
         date >>= 1;
      }

      months = new boolean[12];
      for(int i = 0; i < 12; i++)
      {
         months[i] = ((date & 1) == 1);
         date >>= 1;
      }
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public TimeFrame(TimeFrame src)
   {
      startMinute = src.startMinute;
      startHour = src.startHour;
      endMinute = src.endMinute;
      endHour = src.endHour;
      daysOfMonth = src.daysOfMonth;
      daysOfWeek = Arrays.copyOf(src.daysOfWeek, src.daysOfWeek.length);
      months = Arrays.copyOf(src.months, src.months.length);
      daysConfiguration = src.daysConfiguration;
   }

   /**
    * Format time frame to human readable
    * 
    * @param dfTime time format
    * @param locale loacale to use
    * 
    * @return formatted and translated time frame string
    */
   public String getFormattedDateString(final DateFormat dfTime, Locale locale)
   {
      if (isNever())
         return ClientLocalizationHelper.getText("TimeFrame_Never", locale);

      StringBuilder sb = new StringBuilder();
      if (isAnyTime())
      {
         sb.append(ClientLocalizationHelper.getText("TimeFrame_AnyTime", locale));
      }
      else
      {
         Calendar from = Calendar.getInstance();
         from.set(Calendar.MINUTE, getStartMinute());
         from.set(Calendar.HOUR_OF_DAY, getStartHour());
         from.set(Calendar.SECOND, 0);
         Calendar to = Calendar.getInstance();
         to.set(Calendar.MINUTE, getEndMinute());
         to.set(Calendar.HOUR_OF_DAY, getEndHour());
         to.set(Calendar.SECOND, 0);
         sb.append(String.format(ClientLocalizationHelper.getText("TimeFrame_TimeFormat", locale), dfTime.format(from.getTime()), dfTime.format(to.getTime())));
      }

      if (isAnyDay())
      {
         sb.append(ClientLocalizationHelper.getText("TimeFrame_AnyDay", locale));
      }
      else
      {
         sb.append(String.format(ClientLocalizationHelper.getText("TimeFrame_DayFormat", locale), getDaysOfMonth(), buildIntervals(daysOfWeek, DAYS_OF_THE_WEEK, locale),
               buildIntervals(months, MONTHS, locale)));
      }

      return sb.toString();
   }

   /**
    * Build interval text
    * 
    * @param bits array with selected items
    * @param options array with each item name
    * @param locale locale to use
    * 
    * @return formatted interval string
    */
   private String buildIntervals(boolean[] bits, String[] options, Locale locale)
   {
      StringBuilder text = new StringBuilder();
      int state = 0;
      for(int i = 0; i < bits.length; i++)
      {
         switch(state)
         {
            case 0:
               if (bits[i])
               {
                  if (text.length() != 0)
                     text.append(", ");
                  text.append(ClientLocalizationHelper.getText(options[i], locale));
                  state = 1;
               }
               break;
            case 1:
               if (bits[i] && (i + 1 != bits.length) && bits[i + 1])
               {
                  state = 2;
                  text.append("-");
               }
               else if (bits[i])
               {
                  text.append(", ");
                  text.append(ClientLocalizationHelper.getText(options[i], locale));
               }
               else
               {
                  state = 0;
               }
               break;
            case 2:
               if (bits[i] && ((i + 1 == bits.length) || !bits[i + 1]))
               {
                  state = 0;
                  text.append(ClientLocalizationHelper.getText(options[i], locale));
               }
               break;
         }
      }
      return text.toString();

   }

   /**
    * Fill NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      int time = startHour * 1000000 + startMinute * 10000 + endHour * 100 + endMinute; // BCD format
      msg.setFieldInt32(baseId++, time);
      msg.setFieldInt64(baseId++, daysConfiguration);
   }

   /**
    * @return the startMinute
    */
   public int getStartMinute()
   {
      return startMinute;
   }

   /**
    * @return the startHour
    */
   public int getStartHour()
   {
      return startHour;
   }

   /**
    * @return the endMinute
    */
   public int getEndMinute()
   {
      return endMinute;
   }

   /**
    * @return the endHour
    */
   public int getEndHour()
   {
      return endHour;
   }

   /**
    * @return the daysOfMonth
    */
   public String getDaysOfMonth()
   {
      return daysOfMonth;
   }

   /**
    * @return the daysOfWeek
    */
   public boolean[] getDaysOfWeek()
   {
      return daysOfWeek;
   }

   /**
    * @return the months
    */
   public boolean[] getMonths()
   {
      return months;
   }

   /**
    * @return the daysConfiguration
    */
   public long getDaysConfiguration()
   {
      return daysConfiguration;
   }

   /**
    * Update time frame with new data
    * 
    * @param startHour start hour
    * @param startMinute start minute
    * @param endHour end hour
    * @param endMinute end minute
    * @param daysOfWeek selected days of week
    * @param daysOfMonth selected days of month
    * @param months selected months
    * 
    * @throws TimeFrameFormatException exception in case of parsing error
    */
   public void update(int startHour, int startMinute, int endHour, int endMinute, boolean[] daysOfWeek, String daysOfMonth, boolean[] months) throws TimeFrameFormatException
   {
      int startTime = startHour * 100 + startMinute; // BCD format
      int endTime = endHour * 100 + endMinute; // BCD format
      if (startHour < 0 || startHour > 23 || endHour < 0 || endHour > 23 || startMinute < 0 || startMinute > 59 || endMinute < 0 || endMinute > 59)
      {
         throw new TimeFrameFormatException(TimeFrameFormatException.TIME_VALIDATION_FAILURE);
      }
      if (startTime > endTime)
      {
         throw new TimeFrameFormatException(TimeFrameFormatException.TIME_INCORRECT_ORDER);
      }

      long date = 0;
      for(int i = 11; i >= 0; i--)
      {
         if (months[i])
            date++;
         date <<= 1;
      }

      for(int i = 6; i >= 0; i--)
      {
         if (daysOfWeek[i])
            date++;
         date <<= 1;
      }

      date <<= 31;
      if (daysOfMonth.isEmpty())
      {
         date |= 0XFFFFFFFF;
      }
      else
      {
         String[] entries = daysOfMonth.split(",");
         for(String entry : entries)
         {
            if (entry.contains("-"))
            {
               String ranges[] = entry.split("-");
               try
               {
                  int start = Integer.parseInt(ranges[0].trim());
                  int end = Integer.parseInt(ranges[1].trim());
                  if (start >= 1 && end <= 31)
                  {
                     while(start <= end)
                     {
                        date |= 1L << start;
                        start++;
                     }
                  }
                  else
                  {
                     throw new TimeFrameFormatException(TimeFrameFormatException.DAY_OUT_OF_RANGE);
                  }
               }
               catch(Exception e)
               {
                  throw new TimeFrameFormatException(TimeFrameFormatException.DAY_NOT_A_NUMBER);
               }
            }
            else if (entry.equalsIgnoreCase("L"))
            {
               date |= 1;
            }
            else
            {

               try
               {
                  int num = Integer.parseInt(entry.trim());
                  if (num >= 1 && num <= 31)
                  {
                     date |= 1L << num;
                  }
                  else
                  {
                     throw new TimeFrameFormatException(TimeFrameFormatException.DAY_OUT_OF_RANGE);
                  }
               }
               catch(Exception e)
               {
                  throw new TimeFrameFormatException(TimeFrameFormatException.DAY_NOT_A_NUMBER);
               }
            }
         }
      }
      this.startHour = startHour;
      this.startMinute = startMinute;
      this.endHour = endHour;
      this.endMinute = endMinute;
      this.daysOfMonth = daysOfMonth;
      this.daysOfWeek = daysOfWeek;
      this.months = months;
      daysConfiguration = date;
   }

   /**
    * Check if any date filter is selected
    * 
    * @return true if any date pass thought filter
    */
   public boolean isAnyDay()
   {
      return daysConfiguration == 0x7FFFFFFFFFFFFL || daysConfiguration == 0x7FFFFFFFFFFFEL;
   }

   /**
    * Check if any time filter is selected
    * 
    * @return true if any time pass thought filter
    */
   public boolean isAnyTime()
   {
      return startHour == 0 && startMinute == 0 && endHour == 23 && endMinute == 59;
   }

   /**
    * Check if this schedule will never be executed
    * 
    * @return true if this filter is equal to never execute
    */
   public boolean isNever()
   {
      return (daysConfiguration & 0xFFFFFFFFL) == 0 || (daysConfiguration & 0x7F00000000L) == 0 || (daysConfiguration & 0x7FF8000000000L) == 0;
   }
}
