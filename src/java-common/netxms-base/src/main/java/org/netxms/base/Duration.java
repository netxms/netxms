/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
 * Conversion between number of seconds and human readable duration representation. Duration is written as a sequence of
 * &lt;number&gt;&lt;unit&gt; groups (for example "2h 30m"). Recognized units are s (seconds), m (minutes), h (hours), d (days),
 * and w (weeks); unit letters are case-insensitive. Only the first character of the unit is significant, so spelled out units
 * ("2 hours 30 minutes") are accepted as well. Number without unit is interpreted as seconds. Whitespace between number and
 * unit and between groups is ignored. Same grammar is implemented by ParseDuration() in libnetxms.
 */
public final class Duration
{
   private static final long[] UNIT_MULTIPLIERS = { 604800L, 86400L, 3600L, 60L, 1L };
   private static final String[] UNIT_NAMES = { "w", "d", "h", "m", "s" };

   /**
    * Get multiplier for given unit letter.
    *
    * @param unit unit letter
    * @return multiplier in seconds or 0 if unit letter is not recognized
    */
   private static long getMultiplier(char unit)
   {
      switch(Character.toLowerCase(unit))
      {
         case 's':
            return 1L;
         case 'm':
            return 60L;
         case 'h':
            return 3600L;
         case 'd':
            return 86400L;
         case 'w':
            return 604800L;
         default:
            return 0L;
      }
   }

   /**
    * Parse duration.
    *
    * @param text duration in human readable form
    * @return duration in seconds
    * @throws DurationFormatException if given text is not a valid duration
    */
   public static long parse(String text) throws DurationFormatException
   {
      if (text == null)
         throw new DurationFormatException();

      long seconds = 0;
      int pos = 0;
      int length = text.length();
      boolean groupFound = false;
      while(pos < length)
      {
         while((pos < length) && Character.isWhitespace(text.charAt(pos)))
            pos++;
         if (pos == length)
            break;

         int numberStart = pos;
         while((pos < length) && Character.isDigit(text.charAt(pos)))
            pos++;
         if (pos == numberStart)
            throw new DurationFormatException();

         long value;
         try
         {
            value = Long.parseLong(text.substring(numberStart, pos));
         }
         catch(NumberFormatException e)
         {
            throw new DurationFormatException();
         }

         while((pos < length) && Character.isWhitespace(text.charAt(pos)))
            pos++;

         long multiplier = 1L;
         if (pos < length)
         {
            multiplier = getMultiplier(text.charAt(pos));
            if (multiplier == 0)
               throw new DurationFormatException();
            while((pos < length) && Character.isLetter(text.charAt(pos)))
               pos++;   // skip the rest of spelled out unit name
         }

         seconds += value * multiplier;
         if (seconds < 0)
            throw new DurationFormatException();   // overflow
         groupFound = true;
      }

      if (!groupFound)
         throw new DurationFormatException();
      return seconds;
   }

   /**
    * Parse duration, returning given default value if text cannot be parsed.
    *
    * @param text duration in human readable form
    * @param defaultValue value to be returned if text is not a valid duration
    * @return duration in seconds or default value
    */
   public static long parse(String text, long defaultValue)
   {
      try
      {
         return parse(text);
      }
      catch(DurationFormatException e)
      {
         return defaultValue;
      }
   }

   /**
    * Format duration in human readable form (for example "2h 30m").
    *
    * @param seconds duration in seconds
    * @return duration in human readable form
    */
   public static String format(long seconds)
   {
      if (seconds <= 0)
         return "0s";

      StringBuilder sb = new StringBuilder();
      long rest = seconds;
      for(int i = 0; i < UNIT_MULTIPLIERS.length; i++)
      {
         long value = rest / UNIT_MULTIPLIERS[i];
         if (value > 0)
         {
            if (sb.length() > 0)
               sb.append(' ');
            sb.append(value);
            sb.append(UNIT_NAMES[i]);
            rest -= value * UNIT_MULTIPLIERS[i];
         }
      }
      return sb.toString();
   }

   /**
    * Prevent instantiation of utility class.
    */
   private Duration()
   {
   }
}
