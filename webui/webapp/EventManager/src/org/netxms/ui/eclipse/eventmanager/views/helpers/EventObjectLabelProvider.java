/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.eventmanager.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.eventmanager.widgets.EventTemplateList;

/**
 * Label provider for event template objects
 */
public class EventObjectLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	/**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
	   if ((columnIndex != 0))
	      return null;
		return getImage(element);
	}

	/**
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case EventTemplateList.COLUMN_CODE:
				return Long.toString(((EventTemplate)element).getCode());
			case EventTemplateList.COLUMN_NAME:
				return ((EventTemplate)element).getName();
			case EventTemplateList.COLUMN_SEVERITY:
            return StatusDisplayInfo.getStatusText(((EventTemplate)element).getSeverity());
			case EventTemplateList.COLUMN_FLAGS:
            return ((((EventTemplate)element).getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0) ? "L" : "-"; //$NON-NLS-1$ //$NON-NLS-2$
			case EventTemplateList.COLUMN_MESSAGE:
            return ((EventTemplate)element).getMessage();
		}
		return null;
	}
}
