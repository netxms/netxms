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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Room;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.client.objects.configs.RoomPoint;

/**
 * Editable copy of room floor plan: outline, backdrop placement, passive elements and rack placements. Rack objects themselves
 * (name, status, footprint) are always read from the session object cache; only their placement is kept here.
 */
public class FloorPlanModel
{
   /**
    * Placement of a rack on the floor plan
    */
   public static class RackPlacement
   {
      public int x;
      public int y;
      public int rotation;
      public boolean placed;

      RackPlacement(Rack rack)
      {
         x = rack.getRoomX();
         y = rack.getRoomY();
         rotation = rack.getRoomRotation();
         placed = rack.isPlacedInRoom();
      }

      /**
       * Check if placement differs from the one stored in rack object
       */
      boolean differsFrom(Rack rack)
      {
         return (x != rack.getRoomX()) || (y != rack.getRoomY()) || (rotation != rack.getRoomRotation()) || (placed != rack.isPlacedInRoom());
      }
   }

   private Room room;
   private List<RoomPoint> outline = new ArrayList<RoomPoint>();
   private List<RoomPassiveElement> passiveElements = new ArrayList<RoomPassiveElement>();
   private int backgroundScale;
   private int backgroundX;
   private int backgroundY;
   private Map<Long, RackPlacement> placements = new HashMap<Long, RackPlacement>();
   private boolean roomModified = false;

   /**
    * Create model from room object.
    *
    * @param room room object
    */
   public FloorPlanModel(Room room)
   {
      reset(room);
   }

   /**
    * Discard all local changes and reload from room object.
    *
    * @param room room object
    */
   public void reset(Room room)
   {
      this.room = room;
      outline = new ArrayList<RoomPoint>(room.getOutline().size());
      for(RoomPoint p : room.getOutline())
         outline.add(new RoomPoint(p));
      passiveElements = new ArrayList<RoomPassiveElement>(room.getPassiveElements().size());
      for(RoomPassiveElement e : room.getPassiveElements())
         passiveElements.add(new RoomPassiveElement(e));
      backgroundScale = room.getBackgroundScale();
      backgroundX = room.getBackgroundX();
      backgroundY = room.getBackgroundY();
      placements.clear();
      for(Rack rack : room.getRacks())
         placements.put(rack.getObjectId(), new RackPlacement(rack));
      roomModified = false;
   }

   /**
    * Update rack list from room object without discarding local changes: racks added to the room since last reset get
    * their server-side placement, racks removed from the room are dropped.
    *
    * @param room current room object
    */
   public void updateRacks(Room room)
   {
      this.room = room;
      Map<Long, RackPlacement> updated = new HashMap<Long, RackPlacement>();
      for(Rack rack : room.getRacks())
      {
         RackPlacement p = placements.get(rack.getObjectId());
         updated.put(rack.getObjectId(), (p != null) ? p : new RackPlacement(rack));
      }
      placements = updated;
   }

   /**
    * @return room object this model was built from
    */
   public Room getRoom()
   {
      return room;
   }

   /**
    * @return racks located in the room
    */
   public List<Rack> getRacks()
   {
      return room.getRacks();
   }

   /**
    * @return racks not yet positioned on the floor plan
    */
   public List<Rack> getUnplacedRacks()
   {
      List<Rack> racks = new ArrayList<Rack>();
      for(Rack rack : room.getRacks())
      {
         RackPlacement p = placements.get(rack.getObjectId());
         if ((p != null) && !p.placed)
            racks.add(rack);
      }
      return racks;
   }

   /**
    * Get placement of given rack.
    *
    * @param rackId rack object ID
    * @return placement or null if rack is not in this room
    */
   public RackPlacement getPlacement(long rackId)
   {
      return placements.get(rackId);
   }

   /**
    * @return editable room outline
    */
   public List<RoomPoint> getOutline()
   {
      return outline;
   }

   /**
    * @return editable list of passive elements
    */
   public List<RoomPassiveElement> getPassiveElements()
   {
      return passiveElements;
   }

   /**
    * @return backdrop scale in micrometres per image pixel (0 if not calibrated)
    */
   public int getBackgroundScale()
   {
      return backgroundScale;
   }

   /**
    * @return X offset of backdrop in room coordinates
    */
   public int getBackgroundX()
   {
      return backgroundX;
   }

   /**
    * @return Y offset of backdrop in room coordinates
    */
   public int getBackgroundY()
   {
      return backgroundY;
   }

   /**
    * Set backdrop placement.
    *
    * @param scale scale in micrometres per image pixel
    * @param x X offset in room coordinates
    * @param y Y offset in room coordinates
    */
   public void setBackground(int scale, int x, int y)
   {
      backgroundScale = scale;
      backgroundX = x;
      backgroundY = y;
      roomModified = true;
   }

   /**
    * Mark room part of the model (outline, passive elements) as modified.
    */
   public void markRoomModified()
   {
      roomModified = true;
   }

   /**
    * @return true if room part of the model (outline, backdrop, passive elements) was modified
    */
   public boolean isRoomModified()
   {
      return roomModified;
   }

   /**
    * Get racks whose placement differs from the one stored on server.
    *
    * @return list of racks with modified placement
    */
   public List<Rack> getModifiedRacks()
   {
      List<Rack> racks = new ArrayList<Rack>();
      for(Rack rack : room.getRacks())
      {
         RackPlacement p = placements.get(rack.getObjectId());
         if ((p != null) && p.differsFrom(rack))
            racks.add(rack);
      }
      return racks;
   }

   /**
    * @return true if there are unsaved changes
    */
   public boolean isModified()
   {
      return roomModified || !getModifiedRacks().isEmpty();
   }
}
