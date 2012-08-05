/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.snmp.views.helpers;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.views.SnmpTrapEditor;

/**
 * Comparator for SNMP trap configuration editor
 *
 */
public class SnmpTrapComparator extends ViewerComparator
{
	private NXCSession session;
	
	/**
	 * The constructor
	 */
	public SnmpTrapComparator()
	{
		session = (NXCSession)ConsoleSharedData.getSession();
	}
	
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
		SnmpTrap trap1 = (SnmpTrap)e1;
		SnmpTrap trap2 = (SnmpTrap)e2;
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
			case SnmpTrapEditor.COLUMN_ID:
				rc = Long.signum(trap1.getId() - trap2.getId());
				break;
			case SnmpTrapEditor.COLUMN_TRAP_OID:
				rc = trap1.getObjectId().compareTo(trap2.getObjectId());
				break;
			case SnmpTrapEditor.COLUMN_EVENT:
				EventTemplate evt1 = session.findEventTemplateByCode(trap1.getEventCode());
				EventTemplate evt2 = session.findEventTemplateByCode(trap2.getEventCode());
				String name1 = (evt1 != null) ? evt1.getName() : Messages.SnmpTrapComparator_Unknown;
				String name2 = (evt2 != null) ? evt2.getName() : Messages.SnmpTrapComparator_Unknown;
				rc = name1.compareToIgnoreCase(name2);
				break;
			case SnmpTrapEditor.COLUMN_DESCRIPTION:
				rc = trap1.getDescription().compareToIgnoreCase(trap2.getDescription());
				break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TableViewer)viewer).getTable().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}
}
