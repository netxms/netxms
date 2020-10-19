/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.objecttools.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.ui.eclipse.objecttools.widgets.AbstractObjectToolExecutor;

/**
 * Label provider for object tool executors list
 */
public class ExecutorListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private WorkbenchLabelProvider wbLabelProvider = new WorkbenchLabelProvider();

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
      {
         AbstractObjectToolExecutor e = (AbstractObjectToolExecutor)element;
         return wbLabelProvider.getImage(e.getObject());
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      AbstractObjectToolExecutor e = (AbstractObjectToolExecutor)element;
      switch(columnIndex)
      {
         case 0:
            return e.getObject().getObjectName();
         case 1:
            return e.isRunning() ? "Running" : "Completed";
         default:
            return null;
      }
   }
}
