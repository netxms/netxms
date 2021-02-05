/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.EventTemplate;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.propertypages.Thresholds;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for threshold objects
 */
public class ThresholdLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private WorkbenchLabelProvider eventLabelProvider = new WorkbenchLabelProvider();
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private Image thresholdIcon = Activator.getImageDescriptor("icons/threshold.png").createImage(); //$NON-NLS-1$
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		switch(columnIndex)
		{
			case Thresholds.COLUMN_OPERATION:
				return thresholdIcon;
			case Thresholds.COLUMN_EVENT:
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getFireEvent());
				return StatusDisplayInfo.getStatusImage((event != null) ? event.getSeverity() : Severity.UNKNOWN);
         case Thresholds.COLUMN_DEACTIVATION_EVENT:
            final EventTemplate rearmEvent = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getRearmEvent());
            return StatusDisplayInfo.getStatusImage((rearmEvent != null) ? rearmEvent.getSeverity() : Severity.UNKNOWN);
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
			case Thresholds.COLUMN_OPERATION:
				return ((Threshold)element).getTextualRepresentation();
			case Thresholds.COLUMN_EVENT:
				final EventTemplate event = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getFireEvent());
				return eventLabelProvider.getText(event);
         case Thresholds.COLUMN_DEACTIVATION_EVENT:
            final EventTemplate rearmEvent = (EventTemplate)session.findEventTemplateByCode(((Threshold)element).getRearmEvent());
            return eventLabelProvider.getText(rearmEvent);
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
		eventLabelProvider.dispose();
		thresholdIcon.dispose();
		super.dispose();
	}
}
