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
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Objects;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;

/**
 * Tests for object time zone property (set, inheritance through container hierarchy, override, removal, validation)
 */
public class ObjectTimeZoneTest extends AbstractSessionTest
{
   private static final String CONTAINER_A = "TZ-container-A";
   private static final String CONTAINER_B = "TZ-container-B";
   private static final String CONTAINER_C = "TZ-container-C";
   private final String[] containers = { CONTAINER_A, CONTAINER_B, CONTAINER_C };

   /**
    * Wait until object's effective time zone reaches expected value (object cache is updated asynchronously).
    */
   private static AbstractObject waitForTimeZone(NXCSession session, long objectId, String expected) throws Exception
   {
      AbstractObject object = null;
      for(int i = 0; i < 50; i++)
      {
         object = session.findObjectById(objectId);
         if (Objects.equals(object.getTimeZone(), expected))
            break;
         Thread.sleep(100);
      }
      return object;
   }

   /**
    * Modify time zone of given object.
    */
   private static void setTimeZone(NXCSession session, long objectId, String timeZone) throws Exception
   {
      NXCObjectModificationData md = new NXCObjectModificationData(objectId);
      md.setTimeZone(timeZone);
      session.modifyObject(md);
   }

   /**
    * 1. Creates containers A -> B -> C.
    * 2. Sets time zone on A: B and C inherit it, source object is A.
    * 3. Overrides time zone on B: C inherits from B, A unchanged.
    * 4. Removes time zone from A: B and C keep B's zone.
    * 5. Removes time zone from B: all zones cleared.
    * 6. Invalid zone name is rejected with RCC.INVALID_TIME_ZONE.
    */
   @Test
   public void testTimeZoneInheritance() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      TestHelper.findAndDeleteContainer(session, containers);

      TestHelper.createContainer(session, CONTAINER_A);
      TestHelper.createContainer(session, CONTAINER_B);
      TestHelper.createContainer(session, CONTAINER_C);

      AbstractObject containerA = session.findObjectByName(CONTAINER_A);
      AbstractObject containerB = session.findObjectByName(CONTAINER_B);
      AbstractObject containerC = session.findObjectByName(CONTAINER_C);
      assertNull(containerA.getTimeZone());
      assertNull(containerB.getTimeZone());
      assertNull(containerC.getTimeZone());

      session.bindObject(containerA.getObjectId(), containerB.getObjectId());
      session.bindObject(containerB.getObjectId(), containerC.getObjectId());
      Thread.sleep(300);

      // Set on A, inherited by B and C
      setTimeZone(session, containerA.getObjectId(), "Europe/Riga");
      containerA = waitForTimeZone(session, containerA.getObjectId(), "Europe/Riga");
      containerB = waitForTimeZone(session, containerB.getObjectId(), "Europe/Riga");
      containerC = waitForTimeZone(session, containerC.getObjectId(), "Europe/Riga");
      assertEquals("Europe/Riga", containerA.getTimeZone());
      assertEquals("EET-2EEST,M3.5.0/3,M10.5.0/4", containerA.getTimeZoneRule());
      assertEquals(0, containerA.getTimeZoneSourceObjectId());
      assertFalse(containerA.isTimeZoneInherited());
      assertEquals("Europe/Riga", containerB.getTimeZone());
      assertEquals("EET-2EEST,M3.5.0/3,M10.5.0/4", containerB.getTimeZoneRule());
      assertEquals(containerA.getObjectId(), containerB.getTimeZoneSourceObjectId());
      assertTrue(containerB.isTimeZoneInherited());
      assertEquals("Europe/Riga", containerC.getTimeZone());
      assertEquals(containerA.getObjectId(), containerC.getTimeZoneSourceObjectId());

      // Time zone must not be visible as custom attribute
      assertFalse(containerA.getCustomAttributes().containsKey("$$timeZone"));
      assertFalse(containerB.getCustomAttributes().containsKey("$$timeZone"));
      assertFalse(containerC.getCustomAttributes().containsKey("$$timeZone"));

      // Override on B
      setTimeZone(session, containerB.getObjectId(), "America/New_York");
      containerB = waitForTimeZone(session, containerB.getObjectId(), "America/New_York");
      containerC = waitForTimeZone(session, containerC.getObjectId(), "America/New_York");
      containerA = session.findObjectById(containerA.getObjectId());
      assertEquals("Europe/Riga", containerA.getTimeZone());
      assertEquals("America/New_York", containerB.getTimeZone());
      assertEquals("EST5EDT,M3.2.0,M11.1.0", containerB.getTimeZoneRule());
      assertEquals(0, containerB.getTimeZoneSourceObjectId());
      assertEquals("America/New_York", containerC.getTimeZone());
      assertEquals(containerB.getObjectId(), containerC.getTimeZoneSourceObjectId());

      // Clear on A: B and C keep B's zone
      setTimeZone(session, containerA.getObjectId(), null);
      containerA = waitForTimeZone(session, containerA.getObjectId(), null);
      assertNull(containerA.getTimeZone());
      assertNull(containerA.getTimeZoneRule());
      containerB = session.findObjectById(containerB.getObjectId());
      containerC = session.findObjectById(containerC.getObjectId());
      assertEquals("America/New_York", containerB.getTimeZone());
      assertEquals(0, containerB.getTimeZoneSourceObjectId());
      assertEquals("America/New_York", containerC.getTimeZone());
      assertEquals(containerB.getObjectId(), containerC.getTimeZoneSourceObjectId());

      // Clear on B: everything cleared
      setTimeZone(session, containerB.getObjectId(), "");
      containerB = waitForTimeZone(session, containerB.getObjectId(), null);
      containerC = waitForTimeZone(session, containerC.getObjectId(), null);
      assertNull(containerB.getTimeZone());
      assertNull(containerC.getTimeZone());

      // Invalid zone name (unknown to JVM, so only the name is sent and server cannot resolve it)
      final long objectId = containerA.getObjectId();
      NXCException e = assertThrows(NXCException.class, () -> setTimeZone(session, objectId, "Mars/Olympus"));
      assertEquals(RCC.INVALID_TIME_ZONE, e.getErrorCode());
      containerA = session.findObjectById(objectId);
      assertNull(containerA.getTimeZone());

      TestHelper.findAndDeleteContainer(session, containers);
   }
}
