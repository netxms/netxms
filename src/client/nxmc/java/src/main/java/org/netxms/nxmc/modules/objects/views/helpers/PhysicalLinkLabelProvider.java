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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalLink;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;

/**
 * Physical link label provider
 */
public class PhysicalLinkLabelProvider extends LabelProvider implements ITableLabelProvider
{  
   private NXCSession session;
   private DecoratingObjectLabelProvider objectLabelProvider;

   /**
    * Physical link label provider constructor
    */
   public PhysicalLinkLabelProvider()
   {
      session = Registry.getSession();
      objectLabelProvider = new DecoratingObjectLabelProvider();
   }

   /**
    * Get text for object
    * 
    * @param link reference to link
    * @param isLeft if left or right link object should be used
    * @return text to display about object
    */
   public String getObjectText(PhysicalLink link, boolean isLeft)
   {
      String name = "Unknown";
      AbstractObject object = session.findObjectById(isLeft ? link.getLeftObjectId() : link.getRightObjectId());
      if (object instanceof Interface)
      {
         name = ((Interface)object).getParentNode().getObjectName();
      }
      else if (object instanceof Rack)
      {
         name = object.getObjectName();
      }
      return name;
   }

   /**
    * Get port text - text with interface or with patch panel information
    * 
    * @param link link object
    * @param isLeft if left or right link object should be used
    * @return text with port description
    */
   public String getPortText(PhysicalLink link, boolean isLeft)
   {      
      if (isLeft)
         return getPortTextInternal(link.getLeftObjectId(), link.getLeftPatchPanelId(), link.getLeftPortNumber(), link.getLeftFront());
      else
         return getPortTextInternal(link.getRightObjectId(), link.getRightPatchPanelId(), link.getRightPortNumber(), link.getRightFront());
   }

   /**
    * Internal function that generates port text - text with interface or with patch panel information
    * 
    * @param objectId link object id
    * @param patchPanelId link patch panel id
    * @param portNumber link port number
    * @param side link side
    * @return text with port description
    */
   private String getPortTextInternal(long objectId, long patchPanelId, int portNumber, int side)
   {
      StringBuilder sb = new StringBuilder();
      AbstractObject obj = session.findObjectById(objectId);
      if(obj instanceof Interface)
      {
         sb.append(obj.getObjectName());
      }
      else if (obj instanceof Rack)
      {
         PassiveRackElement el = ((Rack)obj).getPassiveElement(patchPanelId);
         sb.append(el.toString());
         sb.append("/");
         sb.append(portNumber);
         sb.append("/");
         sb.append(side == 0 ? "Front" : "Back");
      }
      return sb.toString();
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      AbstractObject object = null;
      switch(columnIndex)
      {
         case PhysicalLinkManager.COL_LEFT_PORT:
            object = session.findObjectById(((PhysicalLink)element).getLeftObjectId());
            return (object != null && object instanceof Interface) ? objectLabelProvider.getImage(object) : null;
         case PhysicalLinkManager.COL_RIGHT_PORT:
            object = session.findObjectById(((PhysicalLink)element).getRightObjectId());
            return (object != null && object instanceof Interface) ? objectLabelProvider.getImage(object) : null;
         case PhysicalLinkManager.COL_LEFT_OBJECT:
            object = session.findObjectById(((PhysicalLink)element).getLeftObjectId());
            break;
         case PhysicalLinkManager.COL_RIGHT_OBJECT:
            object = session.findObjectById(((PhysicalLink)element).getRightObjectId());
            break;

      }
      if (object != null)
      {
         if (object instanceof Interface)
         {
            object = ((Interface)object).getParentNode();
         }
         return (object != null) ? objectLabelProvider.getImage(object) : null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      PhysicalLink link = (PhysicalLink)element;
      switch(columnIndex)
      {
         case PhysicalLinkManager.COL_PHYSICAL_LINK_ID:
            return Long.toString(link.getId());
         case PhysicalLinkManager.COL_DESCRIPTOIN:
            return link.getDescription();
         case PhysicalLinkManager.COL_LEFT_OBJECT:
         case PhysicalLinkManager.COL_RIGHT_OBJECT:
            return getObjectText(link, columnIndex == PhysicalLinkManager.COL_LEFT_OBJECT);
         case PhysicalLinkManager.COL_LEFT_PORT:
         case PhysicalLinkManager.COL_RIGHT_PORT:
            return getPortText(link, columnIndex == PhysicalLinkManager.COL_LEFT_PORT);
      }
      return null;
   }
}
