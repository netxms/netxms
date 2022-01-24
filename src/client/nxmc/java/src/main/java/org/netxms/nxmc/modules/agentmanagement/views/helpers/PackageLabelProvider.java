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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.packages.PackageInfo;
import org.netxms.nxmc.modules.agentmanagement.views.PackageManager;

/**
 * Label provider for package manager
 */
public class PackageLabelProvider extends LabelProvider implements ITableLabelProvider
{
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
	   if (element instanceof String)
	      return (String)element;
	   
		PackageInfo p = (PackageInfo)element;
		switch(columnIndex)
		{
         case PackageManager.COLUMN_COMMAND:
            return p.getCommand();
         case PackageManager.COLUMN_DESCRIPTION:
            return p.getDescription();
         case PackageManager.COLUMN_FILE:
            return p.getFileName();
			case PackageManager.COLUMN_ID:
				return Long.toString(p.getId());
			case PackageManager.COLUMN_NAME:
				return p.getName();
         case PackageManager.COLUMN_PLATFORM:
            return p.getPlatform();
         case PackageManager.COLUMN_TYPE:
            return p.getType();
			case PackageManager.COLUMN_VERSION:
				return p.getVersion();
		}
		return null;
	}
}
