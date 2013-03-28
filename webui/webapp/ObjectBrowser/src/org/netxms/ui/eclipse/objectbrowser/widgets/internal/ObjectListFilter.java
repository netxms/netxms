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
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Filter for object list
 *
 */
public class ObjectListFilter extends ViewerFilter
{
	private static final long serialVersionUID = 1L;

	private String filterString = null;
	private Set<Integer> classFilter = null;
	private AbstractObject[] sourceObjects = null;
	private Map<Long, AbstractObject> objectList = null;
	private AbstractObject lastMatch = null;
	private boolean usePatternMatching = false;

	/**
	 * Default constructor, creates object list filter without class filter
	 */
	public ObjectListFilter()
	{
	}
	
	/**
	 * Create object list filter with class filter and custom source object list
	 * 
	 * @param sourceObjects Custom source objects list. If null, all known objects used as source.
	 * @param classFilter Class filter
	 */
	public ObjectListFilter(AbstractObject[] sourceObjects, Set<Integer> classFilter)
	{
		this.sourceObjects = sourceObjects;
		this.classFilter = classFilter;
	}

	/**
	 * Match given value to curtrent filter string
	 * 
	 * @param value value to match
	 * @return true if value matched to current filter string
	 */
	private boolean matchFilterString(String value)
	{
		if (filterString == null)
			return true;

		return usePatternMatching ? Glob.matchIgnoreCase(filterString, value) : value.toLowerCase().startsWith(filterString);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if (classFilter != null)
		{
			if (!classFilter.contains(((AbstractObject)element).getObjectClass()))
				return false;
		}
		
		return matchFilterString(((AbstractObject)element).getObjectName());
	}
		
	/**
	 * Set filter string
	 * 
	 * @param filterString new filter string
	 */
	public void setFilterString(final String filterString)
	{
		boolean fullSearch = true;
		if ((filterString != null) && !filterString.isEmpty())
		{
			usePatternMatching = filterString.contains("*") || filterString.contains("?"); //$NON-NLS-1$ //$NON-NLS-2$
			if (usePatternMatching)
			{
				this.filterString = filterString.toLowerCase() + "*"; //$NON-NLS-1$
			}
			else
			{
				String newFilterString = filterString.toLowerCase();
				if (this.filterString != null)
					if (newFilterString.startsWith(this.filterString))
						fullSearch = false;
				this.filterString = newFilterString;
			}
		}
		else
		{
			this.filterString = null;
			usePatternMatching = false;
		}
		updateObjectList(fullSearch);
	}
	
	/**
	 * Update list of matching objects
	 */
	private void updateObjectList(boolean doFullSearch)
	{
		if (filterString != null)
		{
			if (doFullSearch)
			{
				AbstractObject[] fullList = (sourceObjects != null) ? sourceObjects : ((NXCSession)ConsoleSharedData.getSession()).getAllObjects();
				objectList = new HashMap<Long, AbstractObject>();
				for(int i = 0; i < fullList.length; i++)
					if (matchFilterString(fullList[i].getObjectName()))
					{
						objectList.put(fullList[i].getObjectId(), fullList[i]);
						lastMatch = fullList[i];
					}
			}
			else
			{
				lastMatch = null;
				Iterator<AbstractObject> it = objectList.values().iterator();
				while(it.hasNext())
				{
					AbstractObject obj = it.next();
					if (!matchFilterString(obj.getObjectName()))
						it.remove();
					else
						lastMatch = obj;
				}
			}
		}
		else
		{
			objectList = null;
			lastMatch = null;
		}
	}

	/**
	 * Get last matched object
	 * @return Last matched object
	 */
	public final AbstractObject getLastMatch()
	{
		return lastMatch;
	}
}
