/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.junit.jupiter.api.Test;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.RoomElementType;
import org.netxms.client.constants.RoomGridLabels;
import org.netxms.client.constants.RoomType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Room;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.client.objects.configs.RoomPoint;

/**
 * Tests for Room object class and placement of racks on room floor plan
 */
public class RoomTest extends AbstractSessionTest
{
   /**
    * Create object and wait until it appears in client side object cache
    */
   private static <T extends AbstractObject> T createObject(NXCSession session, NXCObjectCreationData cd, Class<T> objectClass) throws Exception
   {
      long id = session.createObject(cd);
      for(int i = 0; i < 50; i++)
      {
         T object = session.findObjectById(id, objectClass);
         if (object != null)
            return object;
         Thread.sleep(100);
      }
      throw new AssertionError("Object " + cd.getName() + " did not appear in object cache");
   }

   /**
    * Modify object and return updated copy from client side object cache
    */
   private static <T extends AbstractObject> T modifyObject(NXCSession session, NXCObjectModificationData md, Class<T> objectClass) throws Exception
   {
      session.modifyObject(md);
      Thread.sleep(500); // object update notification is asynchronous
      return session.findObjectById(md.getObjectId(), objectClass);
   }

   @Test
   public void testRoomAndRackPlacement() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      String suffix = Long.toString(System.currentTimeMillis());
      Room room = null, room2 = null;
      Rack parkedRack = null;
      try
      {
         // Room created with width and depth gets rectangular outline
         NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_ROOM, "IT-Room-" + suffix, GenericObject.SERVICEROOT);
         cd.setRoomType(RoomType.COMPUTER_ROOM);
         cd.setWidth(12000);
         cd.setDepth(8000);
         cd.setHeight(40000); // does not fit into 16 bit integer
         room = createObject(session, cd, Room.class);
         assertEquals(RoomType.COMPUTER_ROOM, room.getRoomType());
         assertEquals(4, room.getOutline().size());
         assertEquals(12000, room.getOutline().get(2).x);
         assertEquals(8000, room.getOutline().get(2).y);
         assertEquals(96.0, room.getArea(), 0.000001);
         assertEquals(40000, room.getHeight());
         assertEquals(0, room.getGridTileSize());
         assertNull(room.getBackgroundImage());
         assertTrue(room.getPassiveElements().isEmpty());

         // Modify room: L-shaped outline, grid, backdrop, passive elements
         List<RoomPoint> outline = new ArrayList<RoomPoint>();
         outline.add(new RoomPoint(0, 0));
         outline.add(new RoomPoint(0, 10000));
         outline.add(new RoomPoint(6000, 10000));
         outline.add(new RoomPoint(6000, 4000));
         outline.add(new RoomPoint(10000, 4000));
         outline.add(new RoomPoint(10000, -500));
         UUID image = UUID.randomUUID();
         List<RoomPassiveElement> elements = new ArrayList<RoomPassiveElement>();
         RoomPassiveElement column = new RoomPassiveElement();
         column.name = "C1";
         column.type = RoomElementType.COLUMN;
         column.x = 3000;
         column.y = -200;
         column.rotation = 45;
         column.width = 400;
         column.depth = 450;
         elements.add(column);

         NXCObjectModificationData md = new NXCObjectModificationData(room.getObjectId());
         md.setRoomType(RoomType.TELECOM);
         md.setRoomOutline(outline);
         md.setRoomHeight(3200);
         md.setGridTileSize(600);
         md.setGridOriginX(-100);
         md.setGridOriginY(150);
         md.setGridLabels(RoomGridLabels.LETTERS_NUMBERS);
         md.setRoomBackgroundImage(image);
         md.setRoomBackgroundScale(2500);
         md.setRoomBackgroundX(-300);
         md.setRoomBackgroundY(400);
         md.setRoomPassiveElements(elements);
         room = modifyObject(session, md, Room.class);
         assertEquals(RoomType.TELECOM, room.getRoomType());
         assertEquals(6, room.getOutline().size());
         assertEquals(-500, room.getOutline().get(5).y);
         assertEquals(78.5, room.getArea(), 0.000001);
         assertEquals(3200, room.getHeight());
         assertEquals(600, room.getGridTileSize());
         assertEquals(-100, room.getGridOriginX());
         assertEquals(150, room.getGridOriginY());
         assertEquals(RoomGridLabels.LETTERS_NUMBERS, room.getGridLabels());
         assertEquals(image, room.getBackgroundImage());
         assertEquals(2500, room.getBackgroundScale());
         assertEquals(-300, room.getBackgroundX());
         assertEquals(400, room.getBackgroundY());
         assertEquals(1, room.getPassiveElements().size());
         RoomPassiveElement e = room.getPassiveElements().get(0);
         assertTrue(e.id != 0);
         assertEquals("C1", e.name);
         assertEquals(RoomElementType.COLUMN, e.type);
         assertEquals(3000, e.x);
         assertEquals(-200, e.y);
         assertEquals(45, e.rotation);
         assertEquals(400, e.width);
         assertEquals(450, e.depth);

         // Background image can be cleared, invalid outline is rejected
         md = new NXCObjectModificationData(room.getObjectId());
         md.setRoomBackgroundImage(NXCommon.EMPTY_GUID);
         room = modifyObject(session, md, Room.class);
         assertNull(room.getBackgroundImage());

         final NXCObjectModificationData invalid = new NXCObjectModificationData(room.getObjectId());
         invalid.setRoomOutline(outline.subList(0, 2));
         NXCException ex = assertThrows(NXCException.class, () -> session.modifyObject(invalid));
         assertEquals(RCC.INVALID_ARGUMENT, ex.getErrorCode());

         // Rack footprint: defaults and explicit values
         cd = new NXCObjectCreationData(AbstractObject.OBJECT_RACK, "IT-Rack-default-" + suffix, room.getObjectId());
         Rack rack = createObject(session, cd, Rack.class);
         assertEquals(42, rack.getHeight());
         assertEquals(600, rack.getWidth());
         assertEquals(1000, rack.getDepth());
         assertFalse(rack.isPlacedInRoom());

         cd = new NXCObjectCreationData(AbstractObject.OBJECT_RACK, "IT-Rack-wide-" + suffix, GenericObject.SERVICEROOT);
         cd.setHeight(47);
         cd.setWidth(800);
         cd.setDepth(1200);
         parkedRack = createObject(session, cd, Rack.class);
         assertEquals(47, parkedRack.getHeight());
         assertEquals(800, parkedRack.getWidth());
         assertEquals(1200, parkedRack.getDepth());

         // Placement on floor plan; "front side only" flag must not be affected
         session.bindObject(room.getObjectId(), parkedRack.getObjectId());
         md = new NXCObjectModificationData(parkedRack.getObjectId());
         md.setObjectFlags(Rack.FRONT_SIDE_ONLY | Rack.PLACED_IN_ROOM, Rack.FRONT_SIDE_ONLY | Rack.PLACED_IN_ROOM);
         md.setRoomX(1200);
         md.setRoomY(-600);
         md.setRoomRotation(-90);
         md.setWidth(750);
         parkedRack = modifyObject(session, md, Rack.class);
         assertTrue(parkedRack.isPlacedInRoom());
         assertTrue(parkedRack.isFrontSideOnly());
         assertEquals(1200, parkedRack.getRoomX());
         assertEquals(-600, parkedRack.getRoomY());
         assertEquals(270, parkedRack.getRoomRotation());
         assertEquals(750, parkedRack.getWidth());
         assertEquals(1200, parkedRack.getDepth());
         room = session.findObjectById(room.getObjectId(), Room.class); // cached copy is replaced on update
         assertEquals(2, room.getRacks().size());

         final NXCObjectModificationData invalidRack = new NXCObjectModificationData(parkedRack.getObjectId());
         invalidRack.setDepth(0);
         ex = assertThrows(NXCException.class, () -> session.modifyObject(invalidRack));
         assertEquals(RCC.INVALID_ARGUMENT, ex.getErrorCode());

         // Rack may have only one room parent, rooms do not nest
         cd = new NXCObjectCreationData(AbstractObject.OBJECT_ROOM, "IT-Room2-" + suffix, GenericObject.SERVICEROOT);
         room2 = createObject(session, cd, Room.class);
         assertEquals(RoomType.OTHER, room2.getRoomType());
         assertEquals(36.0, room2.getArea(), 0.000001); // default 6 x 6 m outline
         final long room2Id = room2.getObjectId();
         final long rackId = parkedRack.getObjectId();
         ex = assertThrows(NXCException.class, () -> session.bindObject(room2Id, rackId));
         assertEquals(RCC.OBJECT_HIERARCHY_VIOLATION, ex.getErrorCode());
         final long roomId = room.getObjectId();
         assertThrows(NXCException.class, () -> session.bindObject(roomId, room2Id));

         // Placement is only valid for the room it was made in
         session.unbindObject(room.getObjectId(), parkedRack.getObjectId());
         Thread.sleep(500);
         parkedRack = session.findObjectById(parkedRack.getObjectId(), Rack.class);
         assertNotNull(parkedRack);
         assertFalse(parkedRack.isPlacedInRoom());
         assertTrue(parkedRack.isFrontSideOnly());
      }
      finally
      {
         if (parkedRack != null)
            session.deleteObject(parkedRack.getObjectId());
         if (room2 != null)
            session.deleteObject(room2.getObjectId());
         if (room != null)
            session.deleteObject(room.getObjectId()); // also deletes rack located only in this room
         session.disconnect();
      }
   }
}
