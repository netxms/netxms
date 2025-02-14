/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.ConnectionLocator;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PointList;
import org.netxms.client.maps.LinkDataLocation;
import org.netxms.client.maps.NetworkMapLink;

public class MultiLabelConnectionLocator extends ConnectionLocator
{   
   private NetworkMapLink link;
   private LinkDataLocation labelLocation;

   /**
    * Constructor for MultiLabelConnectionLocator instance
    * 
    * @param connection
    * @param linkPosition
    */
   public MultiLabelConnectionLocator(Connection connection, NetworkMapLink link, LinkDataLocation labelLocation)
   {
      super(connection);
      this.link = link;
      this.labelLocation = labelLocation;
   }

   /* (non-Javadoc)
    * @see org.eclipse.draw2d.ConnectionLocator#getLocation(org.eclipse.draw2d.geometry.PointList)
    */
   @SuppressWarnings("deprecation")
   @Override
   protected Point getLocation(PointList points)
   {      
      double firstObjectX = points.getFirstPoint().preciseX();
      double firstObjectY = points.getFirstPoint().preciseY();
      double secondObjectX = points.getLastPoint().preciseX();
      double secondObjectY = points.getLastPoint().preciseY();
      
      double adj = Math.abs((secondObjectX - firstObjectX));
      double opp = Math.abs((secondObjectY - firstObjectY));
      double theta = Math.atan(opp/adj);
      double length = Math.sqrt(Math.pow(adj, 2) + Math.pow(opp, 2));
      
      double x;
      double y;
      double locationLenght;
      
      /*
       * Link length is divided to 11 segments
       *    1 segment from each side is left for interface label    
       *    labels and DCIs location is spread in their parts of the link (3 segment for each part) 
       *    (x)|object1(3x)|center(3x)|object2(3x)|(x)   
       */
      double sectionLenght = length / 11;
      double elementPart = sectionLenght * 3;
      
      if (link.getDuplicateCount() != 0)
      {
         if (link.isDirectionInverted())
         {
            locationLenght = (elementPart / (link.getDuplicateCount() + 2)) * ((link.getDuplicateCount() + 2) - (link.getPosition() + 1));
         }
         else
         {
            locationLenght = (elementPart / (link.getDuplicateCount() + 2)) * (link.getPosition() + 1);            
         }
      }
      else
      {
         locationLenght = elementPart / 100 * link.getConfig().getLabelPosition();
      }

      if (labelLocation == LinkDataLocation.OBJECT1)
      {
         locationLenght += sectionLenght * 1;
      }      
      if (labelLocation == LinkDataLocation.CENTER)
      {
         locationLenght += sectionLenght * (1 + 3);         
      }      
      if (labelLocation == LinkDataLocation.OBJECT2)
      {
         locationLenght += sectionLenght * (1 + 3 + 3);
      }

      //X grows from left to right and y from up to down    
      if ((firstObjectX >= secondObjectX) && (firstObjectY >= secondObjectY)) //Second object in 270 to 360 degrees IV
      {
         x = firstObjectX - locationLenght * Math.cos(theta);
         y = firstObjectY - locationLenght * Math.sin(theta);
      }
      else if ((firstObjectX >= secondObjectX) && (firstObjectY < secondObjectY))  //Second object in >0 to 90 degrees I
      {
         x = firstObjectX - locationLenght * Math.cos(-theta);
         y = firstObjectY - locationLenght * Math.sin(-theta);
      }
      else if ((firstObjectX < secondObjectX) && (firstObjectY > secondObjectY))  //Second object in >180 to <270 degrees II
      {
         x = firstObjectX + locationLenght * Math.cos(-theta);
         y = firstObjectY + locationLenght * Math.sin(-theta);
      }
      else //Second object in >90 to 180 degrees II
      {
         x = firstObjectX + locationLenght * Math.cos(theta);
         y = firstObjectY + locationLenght * Math.sin(theta);
      }
      
      return new Point(x, y);
   }
}
