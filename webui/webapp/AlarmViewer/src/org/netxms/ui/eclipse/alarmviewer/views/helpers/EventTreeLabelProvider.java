/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.views.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.views.AlarmDetails;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.console.tools.RegionalSettings;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for event tree
 */
public class EventTreeLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private static final long serialVersionUID = 1L;

	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if (columnIndex == AlarmDetails.EV_COLUMN_SEVERITY)
			return StatusDisplayInfo.getStatusImage(((EventInfo)element).getSeverity());
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case AlarmDetails.EV_COLUMN_SEVERITY:
				return StatusDisplayInfo.getStatusText(((EventInfo)element).getSeverity());
			case AlarmDetails.EV_COLUMN_SOURCE:
				AbstractObject o = session.findObjectById(((EventInfo)element).getSourceObjectId());
				return (o != null) ? o.getObjectName() : ("[" + ((EventInfo)element).getSourceObjectId() + "]"); //$NON-NLS-1$ //$NON-NLS-2$
			case AlarmDetails.EV_COLUMN_NAME:
				return ((EventInfo)element).getName();
			case AlarmDetails.EV_COLUMN_MESSAGE:
				return ((EventInfo)element).getMessage();
			case AlarmDetails.EV_COLUMN_TIMESTAMP:
				return RegionalSettings.getDateTimeFormat().format(((EventInfo)element).getTimeStamp());
		}
		return null;
	}
}
