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
package org.netxms.nxmc.modules.tools.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.base.InetAddressEx;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AccessPoint;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Zone;
import org.netxms.client.objects.queries.ObjectQueryResult;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.modules.objects.widgets.helpers.DecoratingObjectLabelProvider;
import org.netxms.nxmc.modules.tools.views.ObjectFinder;

/**
 * Label provider for object search results
 */
public class ObjectSearchResultLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private DecoratingObjectLabelProvider objectLabelProvider;
   private Table table;

   /**
    * Create label provider for object query results.
    *
    * @param table underlying table control
    */
   public ObjectSearchResultLabelProvider(final TableViewer viewer)
   {
      this.table = viewer.getTable();
      objectLabelProvider = new DecoratingObjectLabelProvider((object) -> viewer.update(object, null));
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex != 0)
         return null;
      AbstractObject object = (element instanceof ObjectQueryResult) ? ((ObjectQueryResult)element).getObject() : (AbstractObject)element;
      return objectLabelProvider.getImage(object);
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AbstractObject object = (element instanceof ObjectQueryResult) ? ((ObjectQueryResult)element).getObject() : (AbstractObject)element;
      switch(columnIndex)
      {
         case ObjectFinder.COL_CLASS:
            return object.getObjectClassName();
         case ObjectFinder.COL_ID:
            return Long.toString(object.getObjectId());
         case ObjectFinder.COL_IP_ADDRESS:
            if (object instanceof AbstractNode)
            {
               InetAddressEx addr = ((AbstractNode)object).getPrimaryIP();
               return addr.isValidUnicastAddress() ? addr.getHostAddress() : null;
            }
            if (object instanceof AccessPoint)
            {
               InetAddressEx addr = ((AccessPoint)object).getIpAddress();
               return addr.isValidUnicastAddress() ? addr.getHostAddress() : null;
            }
            if (object instanceof Interface)
            {
               return ((Interface)object).getIpAddressListAsString();
            }
            return null;
         case ObjectFinder.COL_NAME:
            return objectLabelProvider.getText(object);
         case ObjectFinder.COL_PARENT:
            return getParentNames(object);
         case ObjectFinder.COL_ZONE:
            if (Registry.getSession().isZoningEnabled())
            {
               if (object instanceof AbstractNode)
               {
                  int zoneId = ((AbstractNode)object).getZoneId();
                  Zone zone = Registry.getSession().findZone(zoneId);
                  return (zone != null) ? zone.getObjectName() + " (" + Integer.toString(zoneId) + ")" : Integer.toString(zoneId);
               }
               else if (object instanceof Interface)
               {
                  int zoneId = ((Interface)object).getZoneId();
                  Zone zone = Registry.getSession().findZone(zoneId);
                  return (zone != null) ? zone.getObjectName() + " (" + Integer.toString(zoneId) + ")" : Integer.toString(zoneId);
               }
            }
            /* no break */
         default:
            if (element instanceof ObjectQueryResult)
            {
               String propertyName = getPropertyName(columnIndex);
               if (propertyName != null)
                  return ((ObjectQueryResult)element).getPropertyValue(propertyName);
            }
            return null;
      }
   }

   /**
    * Get property name for given column index
    *
    * @param columnIndex column index
    * @return property name or null
    */
   private String getPropertyName(int columnIndex)
   {
      TableColumn c = table.getColumn(columnIndex);
      return (c != null) ? (String)c.getData("propertyName") : null;
   }

   /**
    * Get list of parent object names
    * 
    * @param object
    * @return
    */
   private static String getParentNames(AbstractObject object)
   {
      StringBuilder sb = new StringBuilder();
      for(AbstractObject o : object.getParentsAsArray())
      {
         if ((o instanceof AbstractNode)|| (o instanceof Collector) || (o instanceof Container) || (o instanceof Rack) || (o instanceof Cluster))
         {
            if (sb.length() > 0)
               sb.append(", ");
            sb.append(o.getObjectName());
         }
      }
      return sb.toString();
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      objectLabelProvider.dispose();
      super.dispose();
   }
}
