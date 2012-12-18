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

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Filter for alarm list
 */
public class AlarmListFilter extends ViewerFilter
{
	private static final long serialVersionUID = 1L;

	private long rootObject = 0;
	private int stateFilter = -1;
	
	/**
	 * 
	 */
	public AlarmListFilter()
	{
		rootObject = 0;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if ((stateFilter != -1) && (((Alarm)element).getState() != stateFilter))
			return false;
				
		if ((rootObject == 0) || (rootObject == ((Alarm)element).getSourceObjectId()))
			return true;	// No filtering by object ID or root object is a source
		
		GenericObject object = ((NXCSession)ConsoleSharedData.getSession()).findObjectById(((Alarm)element).getSourceObjectId());
		if (object != null)
			return object.isChildOf(rootObject);
		return false;
	}

	/**
	 * @param rootObject the rootObject to set
	 */
	public final void setRootObject(long rootObject)
	{
		this.rootObject = rootObject;
	}

	/**
	 * @param stateFilter the stateFilter to set
	 */
	public void setStateFilter(int stateFilter)
	{
		this.stateFilter = stateFilter;
	}
}
