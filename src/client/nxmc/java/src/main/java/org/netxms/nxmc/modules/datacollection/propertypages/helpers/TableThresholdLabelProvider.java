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
package org.netxms.nxmc.modules.datacollection.propertypages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.TableThreshold;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.StatusDisplayInfo;

/**
 * Label provider for list of table column definitions
 */
public class TableThresholdLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private NXCSession session = Registry.getSession();
	private Image thresholdIcon = ResourceManager.getImageDescriptor("icons/threshold.png").createImage(); //$NON-NLS-1$
	
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
			case 2:
			{
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((TableThreshold)element).getActivationEvent());
				return StatusDisplayInfo.getStatusImage((event != null) ? event.getSeverity() : Severity.UNKNOWN);
			}
			case 3:
			{
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((TableThreshold)element).getDeactivationEvent());
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
				return ((TableThreshold)element).getConditionAsText();
			case 1:
			   return Integer.toString(((TableThreshold)element).getSampleCount());
			case 2:
			{
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((TableThreshold)element).getActivationEvent());
				return event.getName();
			}
			case 3:
			{
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((TableThreshold)element).getDeactivationEvent());
				return event.getName();
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
		super.dispose();
	}
}
