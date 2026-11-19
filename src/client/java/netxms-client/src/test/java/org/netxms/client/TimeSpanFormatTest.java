/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import org.junit.jupiter.api.Test;
import org.netxms.client.datacollection.TimeFormatter;

/**
 * Tests for TimeFormatter.formatTimeSpan
 */
public class TimeSpanFormatTest
{
   private static String format(long seconds, boolean showSeconds)
   {
      return TimeFormatter.formatTimeSpan(seconds, showSeconds, (days) -> days + ((days == 1) ? " day" : " days"));
   }

   @Test
   public void testBelowOneDay()
   {
      assertEquals("00:00:00", format(0, true));
      assertEquals("00:00:43", format(43, true));
      assertEquals("01:01:01", format(3661, true));
      assertEquals("01:01", format(3661, false));
   }

   @Test
   public void testDays()
   {
      assertEquals("1 day, 01:01:01", format(90061, true));
      assertEquals("2 days, 00:00", format(172800, false));
   }

   @Test
   public void testNegative()
   {
      assertEquals("-00:00:43", format(-43, true));
      assertEquals("-01:01:01", format(-3661, true));
      assertEquals("-01:01", format(-3661, false));
      assertEquals("-1 day, 01:01:01", format(-90061, true));
      assertEquals("-2 days, 00:00", format(-172800, false));
   }
}
