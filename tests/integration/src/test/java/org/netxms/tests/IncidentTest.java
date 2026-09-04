/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Arrays;
import java.util.Date;
import java.util.EnumSet;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.IncidentState;
import org.netxms.client.events.Incident;
import org.netxms.client.events.IncidentSummary;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;

/**
 * Tests for incident list retrieval with server-side filtering
 */
public class IncidentTest extends AbstractSessionTest
{
   private static final EnumSet<IncidentState> ACTIVE_STATES = EnumSet.complementOf(EnumSet.of(IncidentState.CLOSED));

   /**
    * Check if incident with given ID is present in the list
    */
   private static boolean contains(List<IncidentSummary> list, long incidentId)
   {
      for(IncidentSummary s : list)
         if (s.getId() == incidentId)
            return true;
      return false;
   }

   @Test
   public void testStateFilter() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      long incidentId = session.createIncident(node.getObjectId(), "Integration test incident (state filter)", null, 0);

      // Open incident is visible in active list, in unfiltered list, and in per-object list; not in closed-only list
      assertTrue(contains(session.getIncidents(0, ACTIVE_STATES, null, null, 0), incidentId));
      assertTrue(contains(session.getIncidents(0), incidentId));
      assertTrue(contains(session.getIncidents(node.getObjectId(), ACTIVE_STATES, null, null, 0), incidentId));
      assertFalse(contains(session.getIncidents(0, EnumSet.of(IncidentState.CLOSED), null, null, 0), incidentId));

      session.closeIncident(incidentId);

      // Closed incident is dropped from active list and visible in closed-only and unfiltered lists
      assertFalse(contains(session.getIncidents(0, ACTIVE_STATES, null, null, 0), incidentId));
      assertTrue(contains(session.getIncidents(0, EnumSet.of(IncidentState.CLOSED), null, null, 0), incidentId));
      assertTrue(contains(session.getIncidents(0), incidentId));
      assertTrue(contains(session.getIncidents(node.getObjectId(), EnumSet.of(IncidentState.CLOSED), null, null, 0), incidentId));

      // Summary from database path carries the same data as details
      Incident details = session.getIncident(incidentId);
      assertEquals(IncidentState.CLOSED, details.getState());
      for(IncidentSummary s : session.getIncidents(0, EnumSet.of(IncidentState.CLOSED), null, null, 0))
      {
         if (s.getId() != incidentId)
            continue;
         assertEquals(details.getTitle(), s.getTitle());
         assertEquals(details.getSourceObjectId(), s.getSourceObjectId());
         assertEquals(IncidentState.CLOSED, s.getState());
      }

      session.disconnect();
   }

   @Test
   public void testTimeRangeAndLimit() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node node = TestHelper.findManagementServer(session);
      assertNotNull(node);

      Date before = new Date(System.currentTimeMillis() - 60000);
      long[] ids = new long[3];
      for(int i = 0; i < ids.length; i++)
      {
         ids[i] = session.createIncident(node.getObjectId(), "Integration test incident (limit) #" + i, null, 0);
         session.closeIncident(ids[i]);
      }

      // Time range on creation time
      List<IncidentSummary> inRange = session.getIncidents(node.getObjectId(), null, before, new Date(System.currentTimeMillis() + 60000), 0);
      for(long id : ids)
         assertTrue(contains(inRange, id));
      List<IncidentSummary> future = session.getIncidents(node.getObjectId(), null, new Date(System.currentTimeMillis() + 86400000L), null, 0);
      assertTrue(future.isEmpty());
      List<IncidentSummary> past = session.getIncidents(node.getObjectId(), null, null, before, 0);
      for(long id : ids)
         assertFalse(contains(past, id));

      // Limit returns newest incidents first
      List<IncidentSummary> limited = session.getIncidents(node.getObjectId(), EnumSet.of(IncidentState.CLOSED), before, null, 2);
      assertEquals(2, limited.size());
      long[] expected = Arrays.copyOfRange(ids, 1, 3);
      assertEquals(expected[1], limited.get(0).getId());
      assertEquals(expected[0], limited.get(1).getId());

      session.disconnect();
   }
}
