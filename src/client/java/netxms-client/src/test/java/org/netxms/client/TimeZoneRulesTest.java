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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.time.Instant;
import java.time.ZoneId;
import java.time.zone.ZoneRules;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.jupiter.api.Test;

/**
 * Tests for TimeZoneRules class
 */
public class TimeZoneRulesTest
{
   private static final String NAME = "(<[A-Za-z0-9+\\-]{3,}>|[A-Za-z]{3,})";
   private static final String OFFSET = "-?\\d{1,2}(:\\d{2}(:\\d{2})?)?";
   private static final String DATE = "(M\\d{1,2}\\.[1-5]\\.[0-6]|J\\d{1,3}|\\d{1,3})(/-?\\d{1,3}(:\\d{2}(:\\d{2})?)?)?";
   private static final Pattern STRUCTURE = Pattern.compile("^" + NAME + OFFSET + "(" + NAME + "(" + OFFSET + ")?," + DATE + "," + DATE + ")?$");
   private static final Pattern STD_OFFSET = Pattern.compile("^(?:<[^>]+>|[A-Za-z]+)(-?)(\\d{1,2})(?::(\\d{2}))?(?::(\\d{2}))?");

   @Test
   public void testKnownZones()
   {
      assertEquals("EET-2EEST,M3.5.0/3,M10.5.0/4", TimeZoneRules.toPosixRule(ZoneId.of("Europe/Riga")));
      assertEquals("EST5EDT,M3.2.0,M11.1.0", TimeZoneRules.toPosixRule(ZoneId.of("America/New_York")));
      assertEquals("AEST-10AEDT,M10.1.0,M4.1.0/3", TimeZoneRules.toPosixRule(ZoneId.of("Australia/Sydney")));
      assertEquals("IST-5:30", TimeZoneRules.toPosixRule(ZoneId.of("Asia/Kolkata")));
      assertEquals("UTC0", TimeZoneRules.toPosixRule(ZoneId.of("UTC")));
      assertEquals("GMT0IST,M3.5.0/1,M10.5.0", TimeZoneRules.toPosixRule(ZoneId.of("Europe/Dublin")));

      String santiago = TimeZoneRules.toPosixRule(ZoneId.of("America/Santiago"));
      assertNotNull(santiago);
      assertTrue(santiago.matches("^" + NAME + "4" + NAME + ",M9\\.1\\.6/24,M4\\.1\\.6/24$"), "Unexpected rule for America/Santiago: " + santiago);
   }

   @Test
   public void testAllZones()
   {
      Instant now = Instant.now();
      List<String> unsupported = new ArrayList<>();
      int total = 0;
      for(String id : new TreeSet<>(ZoneId.getAvailableZoneIds()))
      {
         total++;
         ZoneId zoneId = ZoneId.of(id);
         String rule = TimeZoneRules.toPosixRule(zoneId);
         if (rule == null)
         {
            unsupported.add(id);
            continue;
         }

         assertTrue(rule.length() < 128, "Rule too long for " + id + ": " + rule);
         assertTrue(STRUCTURE.matcher(rule).matches(), "Malformed rule for " + id + ": " + rule);

         ZoneRules rules = zoneId.getRules();
         int expected = rules.getTransitionRules().isEmpty() ? -rules.getOffset(now).getTotalSeconds() : -rules.getStandardOffset(now).getTotalSeconds();
         assertEquals(expected, parseStandardOffset(rule), "Standard offset mismatch for " + id + ": " + rule);
      }

      System.out.println("Zones without POSIX rule: " + unsupported.size() + " of " + total);
      for(String id : unsupported)
         System.out.println("   " + id + " " + ZoneId.of(id).getRules().getTransitionRules());
      assertTrue(unsupported.size() < total * 15 / 100, "Too many zones without POSIX rule: " + unsupported.size() + " of " + total);
   }

   /**
    * Parse standard offset (in seconds, POSIX sign convention) from POSIX TZ rule string.
    *
    * @param rule POSIX TZ rule string
    * @return standard offset in seconds
    */
   private static int parseStandardOffset(String rule)
   {
      Matcher m = STD_OFFSET.matcher(rule);
      assertTrue(m.find(), "Cannot parse standard offset from " + rule);
      int v = Integer.parseInt(m.group(2)) * 3600;
      if (m.group(3) != null)
         v += Integer.parseInt(m.group(3)) * 60;
      if (m.group(4) != null)
         v += Integer.parseInt(m.group(4));
      return m.group(1).isEmpty() ? v : -v;
   }
}
