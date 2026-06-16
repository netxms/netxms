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
import org.junit.jupiter.api.Test;
import org.netxms.client.constants.CalendarPeriod;
import org.netxms.client.constants.ColumnFilterType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogFilter;
import org.netxms.client.xml.XMLTools;

/**
 * Tests for log filter serialization (used for persistence of saved log views)
 */
public class LogFilterTest
{
   @Test
   public void testFilterXmlRoundTrip() throws Exception
   {
      LogFilter filter = new LogFilter();
      filter.setColumnFilter("relative_ts", new ColumnFilter(24, TimeUnit.HOUR));
      filter.setColumnFilter("period_ts", new ColumnFilter(CalendarPeriod.THIS_WEEK));
      filter.setColumnFilter("range_ts", new ColumnFilter(1000L, 2000L));
      ColumnFilter less = new ColumnFilter(ColumnFilterType.LESS, 500L);
      less.setNegated(true);
      filter.setColumnFilter("less_ts", less);

      ColumnFilter set = new ColumnFilter();
      set.addSubFilter(new ColumnFilter(ColumnFilterType.GREATER, 100L));
      set.addSubFilter(new ColumnFilter(ColumnFilterType.LESS, 900L));
      filter.setColumnFilter("set_ts", set);

      String xml = XMLTools.serialize(filter);
      LogFilter restored = XMLTools.createFromXml(LogFilter.class, xml);

      ColumnFilter relative = restored.getColumnFilter("relative_ts");
      assertNotNull(relative);
      assertEquals(ColumnFilterType.RELATIVE, relative.getType());
      assertEquals(24, relative.getRelativeValue());
      assertEquals(TimeUnit.HOUR, relative.getRelativeUnit());

      ColumnFilter period = restored.getColumnFilter("period_ts");
      assertNotNull(period);
      assertEquals(ColumnFilterType.CURRENT_PERIOD, period.getType());
      assertEquals(CalendarPeriod.THIS_WEEK, period.getPeriod());

      ColumnFilter range = restored.getColumnFilter("range_ts");
      assertNotNull(range);
      assertEquals(ColumnFilterType.RANGE, range.getType());
      assertEquals(1000L, range.getRangeFrom());
      assertEquals(2000L, range.getRangeTo());

      ColumnFilter restoredLess = restored.getColumnFilter("less_ts");
      assertNotNull(restoredLess);
      assertEquals(ColumnFilterType.LESS, restoredLess.getType());
      assertEquals(500L, restoredLess.getNumericValue());
      assertEquals(true, restoredLess.isNegated());

      ColumnFilter restoredSet = restored.getColumnFilter("set_ts");
      assertNotNull(restoredSet);
      assertEquals(ColumnFilterType.SET, restoredSet.getType());
      assertNotNull(restoredSet.getSubFilters());
      assertEquals(2, restoredSet.getSubFilters().size());
   }
}
