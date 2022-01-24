/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.modules.agentmanagement.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.agentmanagement.views.PackageManager;

/**
 * Agent package comparator
 */
public class PackageComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      PackageInfo p1 = (PackageInfo)e1;
      PackageInfo p2 = (PackageInfo)e2;
      int result;
      switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
      {
         case PackageManager.COLUMN_COMMAND:
            result = p1.getCommand().compareToIgnoreCase(p2.getCommand());
            break;
         case PackageManager.COLUMN_DESCRIPTION:
            result = p1.getDescription().compareToIgnoreCase(p2.getDescription());
            break;
         case PackageManager.COLUMN_FILE:
            result = p1.getFileName().compareToIgnoreCase(p2.getFileName());
            break;
         case PackageManager.COLUMN_ID:
            result = Long.signum(p1.getId() - p2.getId());
            break;
         case PackageManager.COLUMN_NAME:
            result = p1.getName().compareToIgnoreCase(p2.getName());
            break;
         case PackageManager.COLUMN_PLATFORM:
            result = p1.getPlatform().compareToIgnoreCase(p2.getPlatform());
            break;
         case PackageManager.COLUMN_TYPE:
            result = p1.getType().compareToIgnoreCase(p2.getType());
            break;
         case PackageManager.COLUMN_VERSION:
            result = p1.getVersion().compareToIgnoreCase(p2.getVersion());
            break;
         default:
            result = 0;
      }
      return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
   }
}
