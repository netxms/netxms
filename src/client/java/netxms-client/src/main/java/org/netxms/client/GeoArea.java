/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.GeoLocation;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Geographical area
 */
public class GeoArea
{
   protected int id;
   protected String name;
   protected String comments;
   protected List<GeoLocation> border;

   /**
    * Create new geographical area object.
    *
    * @param id area ID
    * @param name area name
    * @param comments comments
    * @param border border points
    */
   public GeoArea(int id, String name, String comments, List<GeoLocation> border)
   {
      this.id = id;
      this.name = name;
      this.comments = comments;
      this.border = new ArrayList<GeoLocation>(border);
   }

   /**
    * Create area object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public GeoArea(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      name = msg.getFieldAsString(baseId + 1);
      comments = msg.getFieldAsString(baseId + 2);

      int count = msg.getFieldAsInt32(baseId + 3);
      border = new ArrayList<GeoLocation>(count);
      if (count > 0)
      {
         long fieldId = baseId + 10;
         for(int i = 0; i < count; i++)
         {
            double lat = msg.getFieldAsDouble(fieldId++);
            double lon = msg.getFieldAsDouble(fieldId++);
            border.add(new GeoLocation(lat, lon));
         }
      }
   }

   /**
    * Copy constructor
    *
    * @param src source object
    */
   public GeoArea(GeoArea src)
   {
      id = src.id;
      name = src.name;
      comments = src.comments;
      border = new ArrayList<GeoLocation>(src.border);
   }

   /**
    * Fill NXCP message with area data
    *
    * @param msg NXCP message
    */
   void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_AREA_ID, id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setField(NXCPCodes.VID_COMMENTS, comments);
      msg.setFieldInt16(NXCPCodes.VID_NUM_ELEMENTS, border.size());
      long fieldId = NXCPCodes.VID_ELEMENT_LIST_BASE;
      for(GeoLocation l : border)
      {
         msg.setField(fieldId++, l.getLatitude());
         msg.setField(fieldId++, l.getLongitude());
      }
   }

   /**
    * Get bounding box for this area. First element of resulting array will contain north-west corner and second element -
    * south-east corner.
    * 
    * @return bounding box for this area (north-west and south-east corners)
    */
   public GeoLocation[] getBoundingBox()
   {
      if (border.isEmpty())
         return new GeoLocation[] { new GeoLocation(0.0, 0.0), new GeoLocation(0.0, 0.0) };

      double north, south, east, west;
      north = south = border.get(0).getLatitude();
      east = west = border.get(0).getLongitude();
      for(int i = 1; i < border.size(); i++)
      {
         GeoLocation l = border.get(i);

         if (l.getLatitude() < south)
            south = l.getLatitude();
         else if (l.getLatitude() > north)
            north = l.getLatitude();

         if (l.getLongitude() < west)
            west = l.getLongitude();
         else if (l.getLongitude() > east)
            east = l.getLongitude();
      }
      return new GeoLocation[] { new GeoLocation(north, west), new GeoLocation(south, east) };
   }

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return (name != null) ? name : ("[" + Integer.toString(id) + "]");
   }

   /**
    * @return the comments
    */
   public String getComments()
   {
      return (comments != null) ? comments : "";
   }

   /**
    * @return the border
    */
   public List<GeoLocation> getBorder()
   {
      return border;
   }
}
