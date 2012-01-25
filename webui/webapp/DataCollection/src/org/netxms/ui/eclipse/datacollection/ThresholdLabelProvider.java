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
package org.netxms.ui.eclipse.datacollection;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
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
	private static final long serialVersionUID = 1L;
	private static final String[] functions = { "last(", "average(", "deviation(", "diff()", "error(", "sum(" };
	private static final String[] operations = { "<", "<=", "==", ">=", ">", "!=", "like", "!like" };
	
	private WorkbenchLabelProvider eventLabelProvider = new WorkbenchLabelProvider();
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private Image thresholdIcon = Activator.getImageDescriptor("icons/threshold.png").createImage();
	
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
				final EventTemplate event = session.findEventTemplateByCode(((Threshold)element).getFireEvent());
				return StatusDisplayInfo.getStatusImage(event.getSeverity());
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
				int f = ((Threshold)element).getFunction();
				StringBuilder text = new StringBuilder(functions[f]);
				if (f != Threshold.F_DIFF)
				{
					text.append(((Threshold)element).getArg1());
					text.append(") ");
				}
				else
				{
					text.append(' ');
				}
				text.append(operations[((Threshold)element).getOperation()]);
				text.append(' ');
				text.append(((Threshold)element).getValue());
				return text.toString();
			case Thresholds.COLUMN_EVENT:
				final EventTemplate event = session.findEventTemplateByCode(((Threshold)element).getFireEvent());
				return eventLabelProvider.getText(event);
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
