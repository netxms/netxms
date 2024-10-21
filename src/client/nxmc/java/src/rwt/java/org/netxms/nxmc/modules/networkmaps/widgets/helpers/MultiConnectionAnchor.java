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

import org.eclipse.draw2d.ChopboxAnchor;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.netxms.client.maps.NetworkMapLink;

public class MultiConnectionAnchor extends ChopboxAnchor
{   
   private NetworkMapLink link;

   /**
    * @param owner
    * @param linkedFigure
    * @param link
    */
   public MultiConnectionAnchor(IFigure owner, NetworkMapLink link)
   {
      super(owner);
      this.link = link;
   }

   /* (non-Javadoc)
    * @see org.eclipse.draw2d.ChopboxAnchor#getLocation(org.eclipse.draw2d.geometry.Point)
    */
   @SuppressWarnings("deprecation")
   @Override
   public Point getLocation(Point reference)
   {
      Rectangle r = Rectangle.getSINGLETON();
      r.setBounds(getBox());
      getOwner().translateToAbsolute(r);

      double centerX = r.getCenter().preciseX();
      double centerY = r.getCenter().preciseY();

      if (r.isEmpty()
            || (reference.x == (int) centerX && reference.y == (int) centerY))
         return new Point((int) centerX, (int) centerY); // This avoids
                                             // divide-by-zero
      double dx = reference.preciseX() - centerX;
      double dy = reference.preciseY() - centerY;

      // r.width, r.height, dx, and dy are guaranteed to be non-zero.
      double scale = 0.5f / Math.max(Math.abs(dx) / r.width, Math.abs(dy)
            / r.height);

      dx *= scale;
      dy *= scale;

      double adj = Math.abs((reference.preciseX() - centerX));
      double opp = Math.abs((reference.preciseY() - centerY));
      double theta = Math.atan(opp/adj);
      
      double xOffset = centerX;
      double yOffset = centerY;
      
      if (link.getDuplicateCount() > 0)
      {
         float maxWidth = Math.min(r.height, r.width) * 1.3f; //Maximum width a bit wider than minimal side
         float maxWidthBetweenLinks = Math.min(r.height, r.width) * 0.35f;   
         float pixelsPerLink =  Math.min(maxWidthBetweenLinks, Math.round(maxWidth/link.getDuplicateCount()));
         float shift = ((pixelsPerLink * link.getDuplicateCount()) / 2 * -1) + link.getPosition() * pixelsPerLink;
         if ((centerX < reference.preciseX() && centerY < reference.preciseY()) ||
               (centerX > reference.preciseX() && centerY > reference.preciseY()))
         {
            xOffset = (centerX + shift * Math.sin(-theta));
            yOffset = (centerY + shift * Math.cos(-theta));
         }
         else
         {
            xOffset = (centerX + shift * Math.sin(theta));
            yOffset = (centerY + shift * Math.cos(theta));
         }         
      }
      
      centerX = dx + xOffset;
      centerY = dy + yOffset;

      return new Point(Math.round(centerX), Math.round(centerY));
   }
}
