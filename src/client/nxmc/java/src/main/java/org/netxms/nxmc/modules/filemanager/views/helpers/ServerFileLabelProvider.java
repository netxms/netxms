/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.filemanager.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.server.ServerFile;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.filemanager.views.ServerFileManager;

/**
 * Label provider for ServerFile objects
 */
public class ServerFileLabelProvider extends BaseFileLabelProvider implements ITableLabelProvider
{
   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == 0)
			return getImage(element);
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case ServerFileManager.COLUMN_NAME:
				return getText(element);
			case ServerFileManager.COLUMN_TYPE:
				return ((ServerFile)element).getExtension();
			case ServerFileManager.COLUMN_SIZE:
            return getSizeString(((ServerFile)element).getSize());
			case ServerFileManager.COLUMN_MODIFYED:
            return ((ServerFile)element).getModificationTime().getTime() == 0 ? "" : DateFormatFactory.getDateTimeFormat().format(((ServerFile)element).getModificationTime());
		}
		return null;
	}
}
