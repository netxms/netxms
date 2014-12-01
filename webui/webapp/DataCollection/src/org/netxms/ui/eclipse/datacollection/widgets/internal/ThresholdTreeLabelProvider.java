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

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.ThresholdLabelProvider;
import org.netxms.ui.eclipse.datacollection.propertypages.Thresholds;
import org.netxms.ui.eclipse.datacollection.widgets.ThresholdSummaryWidget;
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
		if ((element instanceof ThresholdViolationSummary) && (columnIndex == ThresholdSummaryWidget.COLUMN_NODE))
		{
         AbstractObject node = session.findObjectById(((ThresholdViolationSummary)element).getNodeId()); 
			return (node != null) ? wbLabelProvider.getImage(node) : null;
		}
		else if ((element instanceof ThresholdViolationSummary) && (columnIndex == ThresholdSummaryWidget.COLUMN_STATUS))
		{
			return StatusDisplayInfo.getStatusImage(((ThresholdViolationSummary)element).getCurrentSeverity());
		}
		else if ((element instanceof DciValue) && (columnIndex == ThresholdSummaryWidget.COLUMN_STATUS))
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
				case ThresholdSummaryWidget.COLUMN_NODE:
				   AbstractObject node = session.findObjectById(((ThresholdViolationSummary)element).getNodeId());					
					return (node != null) ? node.getObjectName() : null;
				case ThresholdSummaryWidget.COLUMN_STATUS:
					return StatusDisplayInfo.getStatusText(((ThresholdViolationSummary)element).getCurrentSeverity());
				default:
					return null;
			}
		}
		else if (element instanceof DciValue)
		{
			switch(columnIndex)
			{
				case ThresholdSummaryWidget.COLUMN_STATUS:
					return StatusDisplayInfo.getStatusText(((DciValue)element).getActiveThreshold().getCurrentSeverity());
				case ThresholdSummaryWidget.COLUMN_PARAMETER:
					return ((DciValue)element).getDescription();
				case ThresholdSummaryWidget.COLUMN_VALUE:
					return ((DciValue)element).getValue();
				case ThresholdSummaryWidget.COLUMN_CONDITION:
					return thresholdLabelProvider.getColumnText(((DciValue)element).getActiveThreshold(), Thresholds.COLUMN_OPERATION);
				case ThresholdSummaryWidget.COLUMN_TIMESTAMP:
					return RegionalSettings.getDateTimeFormat().format(((DciValue)element).getActiveThreshold().getLastEventTimestamp());
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
