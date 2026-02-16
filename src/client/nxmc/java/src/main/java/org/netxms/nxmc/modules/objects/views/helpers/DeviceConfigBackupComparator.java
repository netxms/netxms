/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.DeviceConfigBackup;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.objects.views.DeviceConfigBackupView;

/**
 * Comparator for device configuration backup list
 */
public class DeviceConfigBackupComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public int compare(Viewer viewer, Object e1, Object e2)
   {
      DeviceConfigBackup b1 = (DeviceConfigBackup)e1;
      DeviceConfigBackup b2 = (DeviceConfigBackup)e2;

      int result;
      int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID");
      switch(column)
      {
         case DeviceConfigBackupView.COLUMN_TIMESTAMP:
            result = b1.getTimestamp().compareTo(b2.getTimestamp());
            break;
         case DeviceConfigBackupView.COLUMN_RUNNING_CONFIG:
            result = Long.compare(b1.getRunningConfigSize(), b2.getRunningConfigSize());
            break;
         case DeviceConfigBackupView.COLUMN_STARTUP_CONFIG:
            result = Long.compare(b1.getStartupConfigSize(), b2.getStartupConfigSize());
            break;
         default:
            result = 0;
            break;
      }

      int direction = ((SortableTableViewer)viewer).getTable().getSortDirection();
      return (direction == SWT.UP) ? result : -result;
   }
}
