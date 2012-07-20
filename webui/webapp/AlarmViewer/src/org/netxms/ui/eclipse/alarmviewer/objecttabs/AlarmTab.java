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
package org.netxms.ui.eclipse.alarmviewer.objecttabs;

import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.EntireNetwork;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmList;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;

/**
 * Alarm tab
 */
public class AlarmTab extends ObjectTab
{
	private AlarmList alarmList;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		alarmList = new AlarmList(getViewPart(), parent, SWT.NONE, "AlarmTab"); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public void objectChanged(GenericObject object)
	{
		alarmList.setRootObject(object.getObjectId());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		alarmList.refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
	 */
	@Override
	public ISelectionProvider getSelectionProvider()
	{
		return alarmList.getSelectionProvider();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.GenericObject)
	 */
	@Override
	public boolean showForObject(GenericObject object)
	{
		return (object instanceof Node) || (object instanceof Subnet) ||
		       (object instanceof EntireNetwork) || (object instanceof Container) ||
		       (object instanceof Cluster) || (object instanceof ServiceRoot);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#dispose()
	 */
	@Override
	public void dispose()
	{
		if (alarmList != null)
			alarmList.dispose();
		super.dispose();
	}
}
