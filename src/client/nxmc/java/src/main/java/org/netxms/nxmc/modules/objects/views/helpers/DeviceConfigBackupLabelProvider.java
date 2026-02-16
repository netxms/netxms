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

import java.util.Arrays;
import java.util.List;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.DeviceConfigBackup;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.objects.views.DeviceConfigBackupView;

/**
 * Label provider for device configuration backup list
 */
public class DeviceConfigBackupLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private List<DeviceConfigBackup> backupList;

   /**
    * Create label provider.
    *
    * @param backupList reference to sorted backup list (newest first)
    */
   public DeviceConfigBackupLabelProvider(List<DeviceConfigBackup> backupList)
   {
      this.backupList = backupList;
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
      DeviceConfigBackup backup = (DeviceConfigBackup)element;
      switch(columnIndex)
      {
         case DeviceConfigBackupView.COLUMN_TIMESTAMP:
            return DateFormatFactory.getDateTimeFormat().format(backup.getTimestamp());
         case DeviceConfigBackupView.COLUMN_RUNNING_CONFIG:
            return (backup.getRunningConfigSize() > 0) ? getSizeString(backup.getRunningConfigSize()) : "\u2014";
         case DeviceConfigBackupView.COLUMN_STARTUP_CONFIG:
            return (backup.getStartupConfigSize() > 0) ? getSizeString(backup.getStartupConfigSize()) : "\u2014";
         case DeviceConfigBackupView.COLUMN_STATUS:
            return getStatusText(backup);
      }
      return null;
   }

   /**
    * Get status text for given backup entry by comparing hashes with previous (older) entry.
    *
    * @param backup backup entry
    * @return status text
    */
   private String getStatusText(DeviceConfigBackup backup)
   {
      int index = backupList.indexOf(backup);
      if (index < 0)
         return "";

      if (index == backupList.size() - 1)
         return "Initial";

      DeviceConfigBackup previous = backupList.get(index + 1);
      boolean runningChanged = !Arrays.equals(backup.getRunningConfigHash(), previous.getRunningConfigHash());
      boolean startupChanged = !Arrays.equals(backup.getStartupConfigHash(), previous.getStartupConfigHash());

      if (runningChanged && startupChanged)
         return "R S";
      if (runningChanged)
         return "R";
      if (startupChanged)
         return "S";
      return "";
   }

   /**
    * Format file size as human-readable string.
    *
    * @param size size in bytes
    * @return formatted size string
    */
   private static String getSizeString(long size)
   {
      if (size >= 10995116277760L)
         return String.format("%.1f TB", size / 1099511627776.0);
      if (size >= 10737418240L)
         return String.format("%.1f GB", size / 1073741824.0);
      if (size >= 10485760)
         return String.format("%.1f MB", size / 1048576.0);
      if (size >= 10240)
         return String.format("%.1f KB", size / 1024.0);
      return Long.toString(size);
   }
}
