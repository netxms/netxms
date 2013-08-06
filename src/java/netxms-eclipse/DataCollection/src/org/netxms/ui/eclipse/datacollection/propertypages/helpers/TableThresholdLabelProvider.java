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
package org.netxms.ui.eclipse.datacollection.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for list of table column definitions
 */
public class TableThresholdLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private WorkbenchLabelProvider eventLabelProvider = new WorkbenchLabelProvider();
	private Image thresholdIcon = Activator.getImageDescriptor("icons/threshold.png").createImage(); //$NON-NLS-1$
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case 0:
				return thresholdIcon;
			case 1:
			{
				final EventTemplate event = session.findEventTemplateByCode(((TableThreshold)element).getActivationEvent());
				return StatusDisplayInfo.getStatusImage((event != null) ? event.getSeverity() : Severity.UNKNOWN);
			}
			case 2:
			{
				final EventTemplate event = session.findEventTemplateByCode(((TableThreshold)element).getDeactivationEvent());
				return StatusDisplayInfo.getStatusImage((event != null) ? event.getSeverity() : Severity.UNKNOWN);
			}
		}
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
			case 0:
				return "";
			case 1:
			{
				final EventTemplate event = session.findEventTemplateByCode(((TableThreshold)element).getActivationEvent());
				return eventLabelProvider.getText(event);
			}
			case 2:
			{
				final EventTemplate event = session.findEventTemplateByCode(((TableThreshold)element).getDeactivationEvent());
				return eventLabelProvider.getText(event);
			}
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		thresholdIcon.dispose();
		eventLabelProvider.dispose();
		super.dispose();
	}
}
