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

import java.time.DayOfWeek;
import java.time.Instant;
import java.time.LocalDate;
import java.time.YearMonth;
import java.time.ZoneId;
import java.time.ZoneOffset;
import java.time.zone.ZoneOffsetTransitionRule;
import java.time.zone.ZoneOffsetTransitionRule.TimeDefinition;
import java.time.zone.ZoneRules;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;
import java.util.regex.Pattern;

/**
 * Helper for converting Java time zone rules into POSIX TZ rule strings (the format zic writes into TZif footers, for example
 * <code>EET-2EEST,M3.5.0/3,M10.5.0/4</code>).
 */
public final class TimeZoneRules
{
   private static final Pattern ALPHABETIC_NAME = Pattern.compile("[A-Za-z]{3,}");
   private static final int MAX_NAME_LENGTH = 31;
   private static final int MAX_RULE_LENGTH = 128;
   private static final int DEFAULT_TRANSITION_TIME = 7200;

   /**
    * Private constructor to prevent instantiation
    */
   private TimeZoneRules()
   {
   }

   /**
    * Convert current rules of given time zone into POSIX TZ rule string. Only the currently effective rules are considered (the rules
    * describing transitions after the last explicitly listed one), so historical changes of a zone are not reflected. Fixed-offset
    * zones are described by their current offset (which may be a permanent daylight saving offset). Rules bound to a fixed day of
    * month are emitted as Julian day rules (<code>Jn</code>), which is exact for dates after February counted from month start, but
    * only approximate for rules counted back from the end of February (leap years shift such dates by one day).
    *
    * @param zoneId time zone
    * @return POSIX TZ rule string, or null if the zone cannot be expressed exactly
    */
   public static String toPosixRule(ZoneId zoneId)
   {
      ZoneRules rules = zoneId.getRules();
      List<ZoneOffsetTransitionRule> trs = rules.getTransitionRules();
      Instant now = Instant.now();

      if (trs.isEmpty())
      {
         ZoneOffset off = rules.getOffset(now);
         String s = name(zoneId, off, false) + offset(off);
         return (s.length() < MAX_RULE_LENGTH) ? s : null;
      }

      if (trs.size() != 2)
         return null;

      ZoneOffsetTransitionRule start = trs.get(0);
      ZoneOffsetTransitionRule end = trs.get(1);
      if (start.getOffsetAfter().equals(start.getStandardOffset()))
      {
         ZoneOffsetTransitionRule t = start;
         start = end;
         end = t;
      }
      if (start.getOffsetAfter().equals(start.getStandardOffset()))
         return null;

      ZoneOffset std = start.getStandardOffset();
      ZoneOffset dst = start.getOffsetAfter();

      String startRule = transition(start);
      String endRule = transition(end);
      if ((startRule == null) || (endRule == null))
         return null;

      StringBuilder sb = new StringBuilder();
      sb.append(name(zoneId, std, false));
      sb.append(offset(std));
      sb.append(name(zoneId, dst, true));
      if (dst.getTotalSeconds() - std.getTotalSeconds() != 3600)
         sb.append(offset(dst));
      sb.append(',');
      sb.append(startRule);
      sb.append(',');
      sb.append(endRule);
      return (sb.length() < MAX_RULE_LENGTH) ? sb.toString() : null;
   }

   /**
    * Format zone offset in POSIX convention (positive values are west of UTC).
    *
    * @param offset zone offset
    * @return formatted offset
    */
   private static String offset(ZoneOffset offset)
   {
      int v = -offset.getTotalSeconds();
      StringBuilder sb = new StringBuilder();
      if (v < 0)
      {
         sb.append('-');
         v = -v;
      }
      appendTime(sb, v);
      return sb.toString();
   }

   /**
    * Append non-negative number of seconds as <code>H</code>, <code>H:MM</code>, or <code>H:MM:SS</code>.
    *
    * @param sb string builder
    * @param seconds number of seconds (non-negative)
    */
   private static void appendTime(StringBuilder sb, int seconds)
   {
      int h = seconds / 3600;
      int m = (seconds % 3600) / 60;
      int s = seconds % 60;
      sb.append(h);
      if ((m != 0) || (s != 0))
      {
         sb.append(':');
         if (m < 10)
            sb.append('0');
         sb.append(m);
         if (s != 0)
         {
            sb.append(':');
            if (s < 10)
               sb.append('0');
            sb.append(s);
         }
      }
   }

   /**
    * Get zone name for given offset in POSIX format: either alphabetic abbreviation or quoted numeric designation like
    * <code>&lt;+0330&gt;</code>.
    *
    * @param zoneId time zone
    * @param offset zone offset
    * @param daylight true for daylight saving time name
    * @return zone name
    */
   private static String name(ZoneId zoneId, ZoneOffset offset, boolean daylight)
   {
      String n = TimeZone.getTimeZone(zoneId).getDisplayName(daylight, TimeZone.SHORT, Locale.ENGLISH);
      if ((n != null) && (n.length() <= MAX_NAME_LENGTH) && ALPHABETIC_NAME.matcher(n).matches())
         return n;

      int v = offset.getTotalSeconds();
      StringBuilder sb = new StringBuilder("<");
      sb.append((v < 0) ? '-' : '+');
      v = Math.abs(v);
      int h = v / 3600;
      int m = (v % 3600) / 60;
      if (h < 10)
         sb.append('0');
      sb.append(h);
      if (m != 0)
      {
         if (m < 10)
            sb.append('0');
         sb.append(m);
      }
      sb.append('>');
      return sb.toString();
   }

   /**
    * Format transition rule in POSIX format (<code>Mm.w.d[/time]</code> or <code>Jn[/time]</code>).
    *
    * @param r transition rule
    * @return formatted rule or null if rule cannot be expressed exactly
    */
   private static String transition(ZoneOffsetTransitionRule r)
   {
      int secs = r.getLocalTime().toSecondOfDay();
      if (r.isMidnightEndOfDay())
         secs = 86400;

      // POSIX transition time is given in local wall time in effect before the transition
      TimeDefinition td = r.getTimeDefinition();
      if (td == TimeDefinition.STANDARD)
         secs += r.getOffsetBefore().getTotalSeconds() - r.getStandardOffset().getTotalSeconds();
      else if (td == TimeDefinition.UTC)
         secs += r.getOffsetBefore().getTotalSeconds();

      int dom = r.getDayOfMonthIndicator();
      DayOfWeek dow = r.getDayOfWeek();
      int month = r.getMonth().getValue();

      StringBuilder sb = new StringBuilder();
      if (dow == null)
      {
         LocalDate date;
         if (dom > 0)
         {
            if ((month == 2) && (dom == 29))
               return null;
            date = LocalDate.of(2001, month, dom);
         }
         else
         {
            date = LocalDate.of(2001, month, 1).plusMonths(1).plusDays(dom);
         }
         sb.append('J').append(date.getDayOfYear());
      }
      else if ((dom == -1) || ((month != 2) && (dom >= YearMonth.of(2001, month).lengthOfMonth() - 6)))
      {
         // Last given weekday of month (Java encodes "lastSun" as "Sun>=25" for 31-day months, so both forms are accepted;
         // February is excluded because its length varies). Indicators past the last-week boundary (e.g. "Fri>=26", which is
         // how Java encodes "lastThu 24:00") are shifted back to that boundary with the difference added to transition time.
         if (dom > 0)
         {
            int k = dom - (YearMonth.of(2001, month).lengthOfMonth() - 6);
            dow = dow.minus(k);
            secs += k * 86400;
         }
         sb.append('M').append(month).append(".5.").append(dow.getValue() % 7);
      }
      else
      {
         if (dom < -1)
         {
            // "dow on or before D" is equivalent to "dow on or after D-6"
            int d = YearMonth.of(2001, month).lengthOfMonth() + 1 + dom;
            dom = d - 6;
            if (dom < 1)
               return null;
         }

         // Shift indicator to 1 mod 7 so that rule can be expressed as "n-th weekday of month"
         int k = (dom - 1) % 7;
         dom -= k;
         dow = dow.minus(k);
         secs += k * 86400;

         int week = (dom - 1) / 7 + 1;
         if (week > 4)
            return null;
         sb.append('M').append(month).append('.').append(week).append('.').append(dow.getValue() % 7);
      }

      if (secs != DEFAULT_TRANSITION_TIME)
      {
         sb.append('/');
         if (secs < 0)
         {
            sb.append('-');
            secs = -secs;
         }
         appendTime(sb, secs);
      }
      return sb.toString();
   }
}
