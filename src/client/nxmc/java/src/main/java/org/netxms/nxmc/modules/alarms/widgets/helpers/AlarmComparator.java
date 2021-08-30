/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.alarms.widgets.helpers;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.TreeColumn;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmHandle;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.interfaces.ZoneMember;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.widgets.AlarmList;
import org.xnap.commons.i18n.I18n;

/**
 * Comparator for alarm list
 */
public class AlarmComparator extends ViewerComparator
{
   private I18n i18n = LocalizationHelper.getI18n(AlarmComparator.class);

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
		TreeColumn sortColumn = ((TreeViewer)viewer).getTree().getSortColumn();
		if (sortColumn == null)
			return 0;

		int rc;
		switch((Integer)sortColumn.getData("ID")) //$NON-NLS-1$
		{
			case AlarmList.COLUMN_SEVERITY:
            rc = ((AlarmHandle)e1).alarm.getCurrentSeverity().compareTo(((AlarmHandle)e2).alarm.getCurrentSeverity());
				break;
			case AlarmList.COLUMN_STATE:
            rc = Integer.signum(((AlarmHandle)e1).alarm.getState() - ((AlarmHandle)e2).alarm.getState());
				break;
			case AlarmList.COLUMN_SOURCE:
            AbstractObject obj1 = Registry.getSession().findObjectById(((AlarmHandle)e1).alarm.getSourceObjectId());
            AbstractObject obj2 = Registry.getSession().findObjectById(((AlarmHandle)e2).alarm.getSourceObjectId());
            String name1 = (obj1 != null) ? obj1.getObjectName() : i18n.tr("Unknown");
            String name2 = (obj2 != null) ? obj2.getObjectName() : i18n.tr("Unknown");
				rc = name1.compareToIgnoreCase(name2);
				break;
			case AlarmList.COLUMN_MESSAGE:
            rc = ((AlarmHandle)e1).alarm.getMessage().compareToIgnoreCase(((AlarmHandle)e2).alarm.getMessage());
				break;
			case AlarmList.COLUMN_COUNT:
            rc = Integer.signum(((AlarmHandle)e1).alarm.getRepeatCount() - ((AlarmHandle)e2).alarm.getRepeatCount());
				break;
			case AlarmList.COLUMN_CREATED:
            rc = ((AlarmHandle)e1).alarm.getCreationTime().compareTo(((AlarmHandle)e2).alarm.getCreationTime());
				break;
			case AlarmList.COLUMN_LASTCHANGE:
            rc = ((AlarmHandle)e1).alarm.getLastChangeTime().compareTo(((AlarmHandle)e2).alarm.getLastChangeTime());
				break;
         case AlarmList.COLUMN_ZONE:
            NXCSession session = Registry.getSession();
            if (session.isZoningEnabled())
            {
               ZoneMember o1 = session.findObjectById(((AlarmHandle)e1).alarm.getSourceObjectId(), ZoneMember.class);
               ZoneMember o2 = session.findObjectById(((AlarmHandle)e2).alarm.getSourceObjectId(), ZoneMember.class);
               String n1 = (o1 != null) ? o1.getZoneName() : "";
               String n2 = (o2 != null) ? o2.getZoneName() : "";
               rc = n1.compareToIgnoreCase(n2);
            }
            else
            {
               rc = 0;
            }
            break;
			default:
				rc = 0;
				break;
		}
		int dir = ((TreeViewer)viewer).getTree().getSortDirection();
		return (dir == SWT.UP) ? rc : -rc;
	}

   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#isSorterProperty(java.lang.Object, java.lang.String)
    */
   @Override
   public boolean isSorterProperty(Object element, String property)
   {
      return true;
   }
}
