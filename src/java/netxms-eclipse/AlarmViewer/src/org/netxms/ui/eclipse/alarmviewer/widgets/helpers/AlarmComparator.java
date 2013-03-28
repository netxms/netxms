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
package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Comparator for alarm list
 */
public class AlarmComparator extends ViewerComparator
{
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TableColumn sortColumn = ((TableViewer)viewer).getTable().getSortColumn();
		if (sortColumn == null)
			return 0;
		
		int rc;
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
			case AlarmList.COLUMN_SEVERITY:
				rc = Integer.signum(((Alarm)e1).getCurrentSeverity() - ((Alarm)e2).getCurrentSeverity());
				break;
			case AlarmList.COLUMN_STATE:
				rc = Integer.signum(((Alarm)e1).getState() - ((Alarm)e2).getState());
				break;
			case AlarmList.COLUMN_SOURCE:
				AbstractObject obj1 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)e1).getSourceObjectId());
				AbstractObject obj2 = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)e2).getSourceObjectId());
				String name1 = (obj1 != null) ? obj1.getObjectName() : Messages.AlarmComparator_Unknown;
				String name2 = (obj2 != null) ? obj2.getObjectName() : Messages.AlarmComparator_Unknown;
				rc = name1.compareToIgnoreCase(name2);
				break;
			case AlarmList.COLUMN_MESSAGE:
				rc = ((Alarm)e1).getMessage().compareToIgnoreCase(((Alarm)e2).getMessage());
				break;
			case AlarmList.COLUMN_COUNT:
				rc = Integer.signum(((Alarm)e1).getRepeatCount() - ((Alarm)e2).getRepeatCount());
				break;
			case AlarmList.COLUMN_CREATED:
				rc = ((Alarm)e1).getCreationTime().compareTo(((Alarm)e2).getCreationTime());
				break;
			case AlarmList.COLUMN_LASTCHANGE:
				rc = ((Alarm)e1).getLastChangeTime().compareTo(((Alarm)e2).getLastChangeTime());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
