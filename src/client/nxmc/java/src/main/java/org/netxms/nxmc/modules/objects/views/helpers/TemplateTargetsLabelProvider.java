/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

import java.util.function.Consumer;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.interfaces.ZoneMember;
import org.netxms.nxmc.modules.objects.views.TemplateTargets;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;

/**
 * Template targets label provider
 */
public class TemplateTargetsLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private BaseObjectLabelProvider objectLabelProvider;

   /**
    * Constructor
    * 
    * @param imageUpdateCallback object image update callback
    */
   public TemplateTargetsLabelProvider(Consumer<AbstractObject> imageUpdateCallback)
   {
      objectLabelProvider = new BaseObjectLabelProvider(imageUpdateCallback);
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return (columnIndex == 0) ? objectLabelProvider.getImage(element) : null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AbstractObject object = (AbstractObject)element;
      switch(columnIndex)
      {
         case TemplateTargets.COLUMN_ID:
            return Long.toString(object.getObjectId());
         case TemplateTargets.COLUMN_NAME:
            return objectLabelProvider.getText(element);
         case TemplateTargets.COLUMN_ZONE:
            return getZone(element);
         case TemplateTargets.COLUMN_PRIMARY_HOST_NAME:
            return getPrimaryHostName(element);
         case TemplateTargets.COLUMN_DESCRIPTION:
            return getDescription(element);
      }
      return null;
   }

   /**
    * Get node's primary name
    * 
    * @param element
    * @return node's primary host name
    */
   public static String getPrimaryHostName(Object element)
   {
      return (element instanceof Node) ? ((Node)element).getPrimaryName() : "";
   }

   /**
    * Get node's description
    * 
    * @param element
    * @return node's description
    */
   public static String getDescription(Object element)
   {
      return (element instanceof Node) ? ((Node)element).getSystemDescription() : "";
   }

   /**
    * Get object name 
    * 
    * @param element element
    * @return object name
    */
   public String getName(Object element)
   {
      return objectLabelProvider.getText(element);
   }

   /**
    * Get object zone name
    * 
    * @param element element
    * @return object zone name
    */
   public static String getZone(Object element)
   {
      return (element instanceof ZoneMember) ? ((ZoneMember)element).getZoneName() : "";
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
