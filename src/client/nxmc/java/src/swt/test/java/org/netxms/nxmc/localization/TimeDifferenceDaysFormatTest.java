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
package org.netxms.nxmc.localization;

import static org.junit.jupiter.api.Assertions.assertEquals;
import java.util.Locale;
import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

/**
 * Tests for DateFormatFactory.formatTimeDifference with values of one day or more (desktop build only: web build localizes the day
 * part through the RWT UI session)
 */
public class TimeDifferenceDaysFormatTest
{
   private static Locale savedLocale;

   @BeforeAll
   public static void setLocale()
   {
      savedLocale = Locale.getDefault();
      Locale.setDefault(Locale.US);
   }

   @AfterAll
   public static void restoreLocale()
   {
      Locale.setDefault(savedLocale);
   }

   @Test
   public void testPositive()
   {
      assertEquals("1 day, 01:01:01", DateFormatFactory.formatTimeDifference(90061, true));
      assertEquals("2 days, 00:00", DateFormatFactory.formatTimeDifference(172800, false));
   }

   @Test
   public void testNegative()
   {
      assertEquals("-1 day, 01:01:01", DateFormatFactory.formatTimeDifference(-90061, true));
      assertEquals("-2 days, 00:00", DateFormatFactory.formatTimeDifference(-172800, false));
   }
}
