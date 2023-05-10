/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.assetmanagement.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Asset;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.tools.ComparatorHelper;

/**
 * Asset property comparator
 */
public class AssetListComparator extends ViewerComparator
{
   private NXCSession session = Registry.getSession();
   private AssetPropertyReader propertyReader;

   /**
    * Asset instance comparator
    * 
    * @param propertyReader asset property reader
    */
   public AssetListComparator(AssetPropertyReader propertyReader)
   {
      this.propertyReader = propertyReader;
   }

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      int result;
      String attribute = (String)((SortableTableViewer)viewer).getTable().getSortColumn().getData("Attribute");
      if (attribute == null)
      {
         // Compare by object name
         result = ((AbstractObject)e1).getObjectName().compareToIgnoreCase(((AbstractObject)e2).getObjectName());
      }
      else
      {
         Asset a1 = getAsset(e1);
         Asset a2 = getAsset(e2);
         
         String v1 = a1.getProperties().get(attribute);
         String v2 = a2.getProperties().get(attribute);
         
         switch(session.getAssetManagementSchema().get(attribute).getDataType())
         {
            case INTEGER:
               Long i1 = null;
               Long i2 = null;
               try
               {
                  i1 = Long.parseLong(v1);
               }
               catch(NumberFormatException e)
               {
               }
               try
               {
                  i2 = Long.parseLong(v2);
               }
               catch(NumberFormatException e)
               {
               }

               if (i1 == null)
                  result = (i2 == null) ? 0 : -1;
               else if (i2 == null)
                  result = 1;
               else
                  result = Long.compare(i1, i2);
               break;
            case NUMBER:
               Double n1 = null;
               Double n2 = null;
               try
               {
                  n1 = Double.parseDouble(v1);
               }
               catch(NumberFormatException e)
               {
               }
               try
               {
                  n2 = Double.parseDouble(v2);
               }
               catch(NumberFormatException e)
               {
               }

               if (n1 == null)
                  result = (n2 == null) ? 0 : -1;
               else if (n2 == null)
                  result = 1;
               else
                  result = Double.compare(n1, n2);
               break;
            case IP_ADDRESS:
               if (v1 == null)
                  result = (v2 == null) ? 0 : -1;
               else if (v2 == null)
                  result = 1;
               else
                  result = ComparatorHelper.compareStringsNatural(v1, v2);
               break;
            default:
               result = propertyReader.valueToText(attribute, v1).compareToIgnoreCase(propertyReader.valueToText(attribute, v2));
               break;
         }
      }

      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }   

   /**
    * Get asset from element
    *
    * @param element list element
    * @return asset object
    */
   private Asset getAsset(Object element)
   {
      if (element instanceof Asset)
         return (Asset)element;
      return session.findObjectById(((AbstractObject)element).getAssetId(), Asset.class);
   }
}
