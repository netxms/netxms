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
import org.junit.jupiter.api.Test;

/**
 * Tests for DateFormatFactory.formatTimeDifference with values below one day (no localized day part, works in desktop and web builds)
 */
public class TimeDifferenceFormatTest
{
   @Test
   public void testPositive()
   {
      assertEquals("00:00:43", DateFormatFactory.formatTimeDifference(43, true));
      assertEquals("01:01:01", DateFormatFactory.formatTimeDifference(3661, true));
      assertEquals("01:01", DateFormatFactory.formatTimeDifference(3661, false));
   }

   @Test
   public void testNegative()
   {
      assertEquals("-00:00:43", DateFormatFactory.formatTimeDifference(-43, true));
      assertEquals("-01:01:01", DateFormatFactory.formatTimeDifference(-3661, true));
      assertEquals("-01:01", DateFormatFactory.formatTimeDifference(-3661, false));
   }

   @Test
   public void testZero()
   {
      assertEquals("00:00:00", DateFormatFactory.formatTimeDifference(0, true));
   }
}
