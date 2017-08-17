/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.netxms.ui.eclipse.eventmanager.views.EventConfigurator;

/**
 * Label provider for event template objects
 */
public class EventTemplateLabelProvider extends WorkbenchLabelProvider implements ITableLabelProvider
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		return (columnIndex == 0) ? getImage(element) : null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case EventConfigurator.COLUMN_CODE:
				return Long.toString(((EventTemplate)element).getCode());
			case EventConfigurator.COLUMN_NAME:
				return getText(element);
			case EventConfigurator.COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(((EventTemplate)element).getSeverity());
			case EventConfigurator.COLUMN_FLAGS:
				return ((((EventTemplate)element).getFlags() & EventTemplate.FLAG_WRITE_TO_LOG) != 0) ? "L" : "-"; //$NON-NLS-1$ //$NON-NLS-2$
			case EventConfigurator.COLUMN_MESSAGE:
				return ((EventTemplate)element).getMessage();
			case EventConfigurator.COLUMN_DESCRIPTION:
				return ((EventTemplate)element).getDescription();
		}
		return null;
	}
}
