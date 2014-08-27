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

import java.util.Date;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.ThresholdViolationSummary;
import org.netxms.ui.eclipse.datacollection.widgets.ThresholdSummaryWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Comparator for threshold overview tree
 */
public class ThresholdTreeComparator extends ViewerComparator
{
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		int result = 0;
		
		if ((e1 instanceof ThresholdViolationSummary) && (e2 instanceof ThresholdViolationSummary))
		{
			switch((Integer)((SortableTreeViewer)viewer).getTree().getSortColumn().getData("ID")) //$NON-NLS-1$
			{
				case ThresholdSummaryWidget.COLUMN_NODE:
					String name1 = session.getObjectName(((ThresholdViolationSummary)e1).getNodeId());
					String name2 = session.getObjectName(((ThresholdViolationSummary)e2).getNodeId());
					result = name1.compareToIgnoreCase(name2);
					break;
				case ThresholdSummaryWidget.COLUMN_STATUS:
					result = ((ThresholdViolationSummary)e1).getCurrentSeverity().compareTo(((ThresholdViolationSummary)e2).getCurrentSeverity());
					break;
				default:
					break;
			}
		}
		else if ((e1 instanceof DciValue) && (e2 instanceof DciValue))
		{
			switch((Integer)((SortableTreeViewer)viewer).getTree().getSortColumn().getData("ID")) //$NON-NLS-1$
			{
				case ThresholdSummaryWidget.COLUMN_STATUS:
					result = ((DciValue)e1).getActiveThreshold().getCurrentSeverity().compareTo(((DciValue)e2).getActiveThreshold().getCurrentSeverity());
					break;
				case ThresholdSummaryWidget.COLUMN_PARAMETER:
					result = ((DciValue)e1).getDescription().compareToIgnoreCase(((DciValue)e2).getDescription());
					break;
				case ThresholdSummaryWidget.COLUMN_VALUE:
					result = ((DciValue)e1).getValue().compareToIgnoreCase(((DciValue)e2).getValue());
					break;
				case ThresholdSummaryWidget.COLUMN_TIMESTAMP:
					Date t1 = ((DciValue)e1).getActiveThreshold().getLastEventTimestamp();
					Date t2 = ((DciValue)e2).getActiveThreshold().getLastEventTimestamp();
					result = Long.signum(t1.getTime() - t2.getTime());
					break;
				default:
					break;
			}
		}
		return (((SortableTreeViewer)viewer).getTree().getSortDirection() == SWT.UP) ? result : -result;
	}
}
