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
package org.netxms.client.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RoomGridLabels;
import org.netxms.client.constants.RoomType;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.client.objects.configs.RoomPoint;

/**
 * Room object - physical room with floor plan where racks are placed at their real location. All coordinates and dimensions are
 * in millimetres in room-local coordinates unless stated otherwise.
 */
public class Room extends DataCollectionContainer
{
   private RoomType roomType;
   private List<RoomPoint> outline;
   private double area;
   private int height;
   private int gridOriginX;
   private int gridOriginY;
   private int gridTileSize;
   private RoomGridLabels gridLabels;
   private UUID backgroundImage;
   private int backgroundScale;
   private int backgroundX;
   private int backgroundY;
   private List<RoomPassiveElement> passiveElements;

   /**
    * Create from NXCP message.
    *
    * @param msg NXCP message
    * @param session owning client session
    */
   public Room(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      roomType = RoomType.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_ROOM_TYPE));
      outline = outlineFromArray(msg.getFieldAsInt32Array(NXCPCodes.VID_OUTLINE));
      area = msg.getFieldAsDouble(NXCPCodes.VID_AREA);
      height = msg.getFieldAsInt32(NXCPCodes.VID_HEIGHT);
      gridOriginX = msg.getFieldAsInt32(NXCPCodes.VID_GRID_ORIGIN_X);
      gridOriginY = msg.getFieldAsInt32(NXCPCodes.VID_GRID_ORIGIN_Y);
      gridTileSize = msg.getFieldAsInt32(NXCPCodes.VID_GRID_TILE_SIZE);
      gridLabels = RoomGridLabels.getByValue(msg.getFieldAsInt32(NXCPCodes.VID_GRID_LABELS));
      backgroundImage = msg.getFieldAsUUID(NXCPCodes.VID_BACKGROUND);
      if (NXCommon.EMPTY_GUID.equals(backgroundImage))
         backgroundImage = null;
      backgroundScale = msg.getFieldAsInt32(NXCPCodes.VID_BACKGROUND_SCALE);
      backgroundX = msg.getFieldAsInt32(NXCPCodes.VID_BACKGROUND_X);
      backgroundY = msg.getFieldAsInt32(NXCPCodes.VID_BACKGROUND_Y);

      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_ELEMENTS);
      passiveElements = new ArrayList<RoomPassiveElement>(count);
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         passiveElements.add(new RoomPassiveElement(msg, fieldId));
         fieldId += 10;
      }
   }

   /**
    * Convert array of interleaved x,y values (as used in NXCP messages) to list of points.
    *
    * @param values interleaved x,y values (can be null)
    * @return list of points
    */
   public static List<RoomPoint> outlineFromArray(int[] values)
   {
      if (values == null)
         return new ArrayList<RoomPoint>(0);
      List<RoomPoint> points = new ArrayList<RoomPoint>(values.length / 2);
      for(int i = 0; i + 1 < values.length; i += 2)
         points.add(new RoomPoint(values[i], values[i + 1]));
      return points;
   }

   /**
    * Convert list of points to array of interleaved x,y values (as used in NXCP messages).
    *
    * @param points list of points
    * @return interleaved x,y values
    */
   public static int[] outlineToArray(List<RoomPoint> points)
   {
      int[] values = new int[points.size() * 2];
      int i = 0;
      for(RoomPoint p : points)
      {
         values[i++] = p.x;
         values[i++] = p.y;
      }
      return values;
   }

   /**
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Room";
   }

   /**
    * Get racks located in this room.
    *
    * @return list of racks located in this room
    */
   public List<Rack> getRacks()
   {
      List<Rack> racks = new ArrayList<Rack>();
      for(AbstractObject o : getChildrenAsArray())
      {
         if (o instanceof Rack)
            racks.add((Rack)o);
      }
      return racks;
   }

   /**
    * Get room type.
    *
    * @return room type
    */
   public RoomType getRoomType()
   {
      return roomType;
   }

   /**
    * Get room outline as ordered list of polygon vertices.
    *
    * @return room outline
    */
   public List<RoomPoint> getOutline()
   {
      return outline;
   }

   /**
    * Get floor area in square metres (calculated by server from room outline).
    *
    * @return floor area in square metres
    */
   public double getArea()
   {
      return area;
   }

   /**
    * Get room height.
    *
    * @return room height in millimetres or 0 if undeclared
    */
   public int getHeight()
   {
      return height;
   }

   /**
    * @return X coordinate of floor tile grid origin
    */
   public int getGridOriginX()
   {
      return gridOriginX;
   }

   /**
    * @return Y coordinate of floor tile grid origin
    */
   public int getGridOriginY()
   {
      return gridOriginY;
   }

   /**
    * Get floor tile size.
    *
    * @return floor tile size in millimetres or 0 if there is no tile grid
    */
   public int getGridTileSize()
   {
      return gridTileSize;
   }

   /**
    * @return floor tile label scheme
    */
   public RoomGridLabels getGridLabels()
   {
      return gridLabels;
   }

   /**
    * Get image library UUID of floor plan backdrop.
    *
    * @return image library UUID of floor plan backdrop or null if not set
    */
   public UUID getBackgroundImage()
   {
      return backgroundImage;
   }

   /**
    * Get backdrop calibration.
    *
    * @return backdrop scale in micrometres per image pixel
    */
   public int getBackgroundScale()
   {
      return backgroundScale;
   }

   /**
    * @return X offset of the backdrop in room coordinates
    */
   public int getBackgroundX()
   {
      return backgroundX;
   }

   /**
    * @return Y offset of the backdrop in room coordinates
    */
   public int getBackgroundY()
   {
      return backgroundY;
   }

   /**
    * Get passive elements placed on floor plan.
    *
    * @return list of passive elements
    */
   public List<RoomPassiveElement> getPassiveElements()
   {
      return passiveElements;
   }
}
