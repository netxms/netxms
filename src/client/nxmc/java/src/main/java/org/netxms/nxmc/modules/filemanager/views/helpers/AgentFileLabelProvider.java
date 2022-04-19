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

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.server.AgentFile;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.modules.filemanager.views.AgentFileManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for AgentFile objects
 */
public class AgentFileLabelProvider extends BaseFileLabelProvider implements ITableLabelProvider, IColorProvider
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
			case AgentFileManager.COLUMN_NAME:
				return getText(element);
			case AgentFileManager.COLUMN_TYPE:
				return ((AgentFile)element).getExtension();
			case AgentFileManager.COLUMN_SIZE:
			   if (((AgentFile)element).isDirectory())
			   {
			      if (((AgentFile)element).getFileInfo() != null)
			         return getSizeString(((AgentFile)element).getFileInfo().getSize()) + " (" + ((AgentFile)element).getFileInfo().getItemCount() + " files)";
			      else
			         return "";
			   }			      
            return (((AgentFile)element).isPlaceholder()) ? "" : getSizeString(((AgentFile)element).getSize());
			case AgentFileManager.COLUMN_MODIFYED:
            return (((AgentFile)element).isPlaceholder() || ((AgentFile)element).getModificationTime().getTime() == 0) ? "" :
                  DateFormatFactory.getDateTimeFormat().format(((AgentFile)element).getModificationTime());
			case AgentFileManager.COLUMN_OWNER:
			   return ((AgentFile)element).getOwner();
			case AgentFileManager.COLUMN_GROUP:
            return ((AgentFile)element).getGroup();
			case AgentFileManager.COLUMN_ACCESS_RIGHTS:
            return ((AgentFile)element).getAccessRights();
		}
		return null;
	}

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((AgentFile)element).isPlaceholder())
         return StatusDisplayInfo.getStatusColor(ObjectStatus.DISABLED);
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
