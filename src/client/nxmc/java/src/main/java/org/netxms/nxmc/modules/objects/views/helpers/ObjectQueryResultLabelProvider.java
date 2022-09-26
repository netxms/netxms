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
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.objects.queries.ObjectQueryResult;

/**
 * Label provider for displaying predefined query result
 */
public class ObjectQueryResultLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private Table table;

   /**
    * Create label provider for object query results.
    *
    * @param table underlying table control
    */
   public ObjectQueryResultLabelProvider(Table table)
   {
      this.table = table;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      if (element instanceof ObjectQueryResult)
      {
         String propertyName = getPropertyName(columnIndex);
         if (propertyName != null)
            return ((ObjectQueryResult)element).getPropertyValue(propertyName);
      }
      return null;
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
}
