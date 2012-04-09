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
package org.netxms.ui.eclipse.datacollection.widgets.internal;

import java.text.DateFormat;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.ThresholdLabelProvider;
import org.netxms.ui.eclipse.datacollection.propertypages.Thresholds;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for threshold violation tree
 */
public class ThresholdTreeLabelProvider extends LabelProvider implements ITableLabelProvider
{
	private WorkbenchLabelProvider wbLabelProvider = new WorkbenchLabelProvider();
	private ThresholdLabelProvider thresholdLabelProvider = new ThresholdLabelProvider();
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
	 */
	@Override
	public Image getColumnImage(Object element, int columnIndex)
	{
		if ((element instanceof ThresholdViolationSummary) && (columnIndex == 0))
		{
			Node node = (Node)session.findObjectById(((ThresholdViolationSummary)element).getNodeId(), Node.class);
			return (node != null) ? wbLabelProvider.getImage(node) : null;
		}
		else if ((element instanceof ThresholdViolationSummary) && (columnIndex == 1))
		{
			return StatusDisplayInfo.getStatusImage(((ThresholdViolationSummary)element).getCurrentSeverity());
		}
		else if ((element instanceof DciValue) && (columnIndex == 1))
		{
			return StatusDisplayInfo.getStatusImage(((DciValue)element).getActiveThreshold().getCurrentSeverity());
		}
		return null;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
	 */
	@Override
	public String getColumnText(Object element, int columnIndex)
	{
		if (element instanceof ThresholdViolationSummary)
		{
			switch(columnIndex)
			{
				case 0:
					Node node = (Node)session.findObjectById(((ThresholdViolationSummary)element).getNodeId(), Node.class);
					return (node != null) ? node.getObjectName() : null;
				case 1:
					return StatusDisplayInfo.getStatusText(((ThresholdViolationSummary)element).getCurrentSeverity());
				default:
					return null;
			}
		}
		else if (element instanceof DciValue)
		{
			switch(columnIndex)
			{
				case 1:
					return StatusDisplayInfo.getStatusText(((DciValue)element).getActiveThreshold().getCurrentSeverity());
				case 2:
					return ((DciValue)element).getDescription();
				case 3:
					return ((DciValue)element).getValue();
				case 4:
					return thresholdLabelProvider.getColumnText(((DciValue)element).getActiveThreshold(), Thresholds.COLUMN_OPERATION);
				case 5:
					return DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.SHORT).format(((DciValue)element).getActiveThreshold().getLastEventTimestamp());
				default:
					return null;
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
		wbLabelProvider.dispose();
		super.dispose();
	}
}
