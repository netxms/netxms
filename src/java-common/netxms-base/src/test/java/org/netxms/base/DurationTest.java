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

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import org.junit.jupiter.api.Test;

/**
 * Duration parser and formatter test
 */
public class DurationTest
{
   @Test
   public void testParse() throws Exception
   {
      // Bare values are interpreted as seconds
      assertEquals(0L, Duration.parse("0"));
      assertEquals(120L, Duration.parse("120"));
      assertEquals(4294967296L, Duration.parse("4294967296"));

      // Units (case-insensitive)
      assertEquals(30L, Duration.parse("30s"));
      assertEquals(300L, Duration.parse("5m"));
      assertEquals(7200L, Duration.parse("2h"));
      assertEquals(86400L, Duration.parse("1d"));
      assertEquals(604800L, Duration.parse("1w"));
      assertEquals(7200L, Duration.parse("2H"));
      assertEquals(259200L, Duration.parse("3D"));

      // Whitespace is ignored, only the first character of the unit matters
      assertEquals(10L, Duration.parse("  10  "));
      assertEquals(7200L, Duration.parse(" 2 h "));
      assertEquals(120L, Duration.parse("2 minutes"));

      // Multiple groups
      assertEquals(9000L, Duration.parse("2h 30m"));
      assertEquals(9000L, Duration.parse("2h30m"));
      assertEquals(788645L, Duration.parse("1w 2d 3h 4m 5s"));
      assertEquals(5400L, Duration.parse("1 hour 30 minutes"));
      assertEquals(90L, Duration.parse("1m 30"));
   }

   @Test
   public void testParseInvalid()
   {
      assertThrows(DurationFormatException.class, () -> Duration.parse(null));
      assertThrows(DurationFormatException.class, () -> Duration.parse(""));
      assertThrows(DurationFormatException.class, () -> Duration.parse("   "));
      assertThrows(DurationFormatException.class, () -> Duration.parse("abc"));
      assertThrows(DurationFormatException.class, () -> Duration.parse("10x"));
      assertThrows(DurationFormatException.class, () -> Duration.parse("2h x"));
      assertThrows(DurationFormatException.class, () -> Duration.parse("2h 30x"));
      assertThrows(DurationFormatException.class, () -> Duration.parse("-5"));

      assertEquals(7L, Duration.parse("10x", 7));
      assertEquals(7L, Duration.parse(null, 7));
      assertEquals(300L, Duration.parse("5m", 7));
   }

   @Test
   public void testFormat()
   {
      assertEquals("0s", Duration.format(0));
      assertEquals("0s", Duration.format(-1));
      assertEquals("45s", Duration.format(45));
      assertEquals("1m 30s", Duration.format(90));
      assertEquals("2h", Duration.format(7200));
      assertEquals("2h 30m", Duration.format(9000));
      assertEquals("1w 2d 3h 4m 5s", Duration.format(788645));
   }

   @Test
   public void testRoundTrip() throws Exception
   {
      long[] values = { 0, 1, 59, 60, 90, 3600, 9000, 86400, 604800, 788645 };
      for(long v : values)
         assertEquals(v, Duration.parse(Duration.format(v)));
   }
}
