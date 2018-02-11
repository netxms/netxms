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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Connection;
import org.eclipse.draw2d.ConnectionLocator;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.PointList;
import org.netxms.client.maps.NetworkMapLink;

public class MultiLabelConnectionLocator extends ConnectionLocator
{   
   private NetworkMapLink link;

   /**
    * Constructor for MultiLabelConnectionLocator instance
    * 
    * @param connection
    * @param linkPosition
    */
   public MultiLabelConnectionLocator(Connection connection, NetworkMapLink link)
   {
      super(connection);
      this.link = link;
   }

   /* (non-Javadoc)
    * @see org.eclipse.draw2d.ConnectionLocator#getLocation(org.eclipse.draw2d.geometry.PointList)
    */
   @SuppressWarnings("deprecation")
   @Override
   protected Point getLocation(PointList points)
   {      
      double adj = Math.abs((points.getLastPoint().preciseX() - points.getFirstPoint().preciseX()));
      double opp = Math.abs((points.getLastPoint().preciseY() - points.getFirstPoint().preciseY()));
      double theta = Math.atan(opp/adj);
      double length = Math.sqrt(Math.pow(adj, 2) + Math.pow(opp, 2));
      
      double x;
      double y;
      double dividedLength = (length / (link.getDuplicateCount() + 2)) * (link.getPosition() + 1);
      if ((points.getFirstPoint().preciseX() > points.getLastPoint().preciseX()) &&
          (points.getFirstPoint().preciseY() > points.getLastPoint().preciseY()) ||
         ((points.getFirstPoint().preciseX() > points.getLastPoint().preciseX()) &&
          (points.getFirstPoint().preciseY() == points.getLastPoint().preciseY())))
      {
         x = points.getLastPoint().preciseX() + dividedLength * Math.cos(theta);
         y = points.getLastPoint().preciseY() + dividedLength * Math.sin(theta);
      }
      else if (((points.getFirstPoint().preciseX() > points.getLastPoint().preciseX()) &&
               (points.getFirstPoint().preciseY() < points.getLastPoint().preciseY())) ||
              ((points.getFirstPoint().preciseX() == points.getLastPoint().preciseX()) &&
               (points.getFirstPoint().preciseY() < points.getLastPoint().preciseY())))
      {
         x = points.getFirstPoint().preciseX() - (length - dividedLength) * Math.cos(-theta);
         y = points.getFirstPoint().preciseY() - (length - dividedLength) * Math.sin(-theta);
      }
      else if ((points.getFirstPoint().preciseX() < points.getLastPoint().preciseX()) &&
               (points.getFirstPoint().preciseY() > points.getLastPoint().preciseY()))
      {
         x = points.getLastPoint().preciseX() - (length - dividedLength) * Math.cos(-theta);
         y = points.getLastPoint().preciseY() - (length - dividedLength) * Math.sin(-theta);
      }
      else if ((points.getFirstPoint().preciseX() == points.getLastPoint().preciseX()) &&
               (points.getFirstPoint().preciseY() > points.getLastPoint().preciseY()))
      {
         x = points.getLastPoint().preciseX() + (length - dividedLength) * Math.cos(theta);
         y = points.getLastPoint().preciseY() + (length - dividedLength) * Math.sin(theta);
      }
      else
      {
         x = points.getFirstPoint().preciseX() + dividedLength * Math.cos(theta);
         y = points.getFirstPoint().preciseY() + dividedLength * Math.sin(theta);
      }
      
      return new Point(x, y);
   }
}
