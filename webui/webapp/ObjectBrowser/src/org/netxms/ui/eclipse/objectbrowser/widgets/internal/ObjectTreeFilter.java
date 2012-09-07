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
package org.netxms.ui.eclipse.objectbrowser.widgets.internal;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.ServiceCheck;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Filter for object tree
 */
public class ObjectTreeFilter extends ViewerFilter
{
	private static final long serialVersionUID = 1L;
	
	private static final int NONE = 0;
	private static final int NAME = 1;
	private static final int COMMENTS = 2;
	private static final int IP_ADDRESS = 3;
	
	private String filterString = null;
	private boolean hideUnmanaged = false;
	private boolean hideTemplateChecks = false;
	private Map<Long, GenericObject> objectList = null;
	private GenericObject lastMatch = null;
	private long[] rootObjects = null;
	private Set<Integer> classFilter = null;
	private boolean usePatternMatching = false;
	private int mode = NAME;
	
	/**
	 * Constructor
	 */
	public ObjectTreeFilter(long[] rootObjects, Set<Integer> classFilter)
	{
		if (rootObjects != null)
		{
			this.rootObjects = new long[rootObjects.length];
			System.arraycopy(rootObjects, 0, this.rootObjects, 0, rootObjects.length);
		}
		this.classFilter = classFilter;
	}
	
	/**
	 * Match given value to current filter string
	 * 
	 * @param object object to match
	 * @return true if object matched to current filter string
	 */
	private boolean matchFilterString(GenericObject object)
	{
		if ((mode == NONE) || (filterString == null))
			return true;
		
		switch(mode)
		{
			case NAME:
				return usePatternMatching ? Glob.matchIgnoreCase(filterString, object.getObjectName()) : object.getObjectName().toLowerCase().startsWith(filterString);
			case IP_ADDRESS:
				return object.getPrimaryIP().getHostAddress().startsWith(filterString);
			case COMMENTS:
				return object.getComments().toLowerCase().contains(filterString);
		}
		
		return false;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
	 */
	@Override
	public boolean select(Viewer viewer, Object parentElement, Object element)
	{
		if (classFilter != null)
		{
			if (!classFilter.contains(((GenericObject)element).getObjectClass()))
				return false;
		}
		
		if (hideUnmanaged && (((GenericObject)element).getStatus() == Severity.UNMANAGED))
			return false;
		
		if (hideTemplateChecks && (element instanceof ServiceCheck) && ((ServiceCheck)element).isTemplate())
			return false;
		
		if (objectList == null)
			return true;

		boolean pass = objectList.containsKey(((GenericObject)element).getObjectId());
		if (!pass && (((GenericObject)element).getNumberOfChilds() > 0))
		{
			Iterator<GenericObject> it = objectList.values().iterator();
			while(it.hasNext())
			{
				GenericObject obj = it.next();
				if (obj.isChildOf(((GenericObject)element).getObjectId()))
				{
					pass = true;
					break;
				}
			}
		}
		return pass;
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
			if (filterString.charAt(0) == '/')
			{
				mode = COMMENTS;
				this.filterString = filterString.substring(1);
			}
			else if (filterString.charAt(0) == '>')
			{
				mode = IP_ADDRESS;
				this.filterString = filterString.substring(1);
			}
			else
			{
				mode = NAME;
				usePatternMatching = filterString.contains("*") || filterString.contains("?");
				if (usePatternMatching)
				{
					this.filterString = filterString.toLowerCase() + "*";
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
		}
		else
		{
			this.filterString = null;
			usePatternMatching = false;
			mode = NONE;
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
				GenericObject[] fullList = ((NXCSession)ConsoleSharedData.getSession()).getAllObjects();
				objectList = new HashMap<Long, GenericObject>();
				for(int i = 0; i < fullList.length; i++)
					if (matchFilterString(fullList[i]) &&
					    ((rootObjects == null) || fullList[i].isChildOf(rootObjects)))
					{
						objectList.put(fullList[i].getObjectId(), fullList[i]);
						lastMatch = fullList[i];
					}
			}
			else
			{
				lastMatch = null;
				Iterator<GenericObject> it = objectList.values().iterator();
				while(it.hasNext())
				{
					GenericObject obj = it.next();
					if (!matchFilterString(obj))
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
	public final GenericObject getLastMatch()
	{
		return lastMatch;
	}
	
	/**
	 * Get parent for given object
	 */
	public GenericObject getParent(final GenericObject childObject)
	{
		GenericObject[] parents = childObject.getParentsAsArray();
		for(GenericObject object : parents)
		{
			if (object != null)
			{
				if ((rootObjects == null) || object.isChildOf(rootObjects))
					return object;
			}
		}
		return null;
	}

	/**
	 * @return the hideUnmanaged
	 */
	public boolean isHideUnmanaged()
	{
		return hideUnmanaged;
	}

	/**
	 * @param hideUnmanaged the hideUnmanaged to set
	 */
	public void setHideUnmanaged(boolean hideUnmanaged)
	{
		this.hideUnmanaged = hideUnmanaged;
	}

	/**
	 * @return the hideTemplateChecks
	 */
	public boolean isHideTemplateChecks()
	{
		return hideTemplateChecks;
	}

	/**
	 * @param hideTemplateChecks the hideTemplateChecks to set
	 */
	public void setHideTemplateChecks(boolean hideTemplateChecks)
	{
		this.hideTemplateChecks = hideTemplateChecks;
	}

	/**
	 * @param rootObjects the rootObjects to set
	 */
	public void setRootObjects(long[] rootObjects)
	{
		if (rootObjects != null)
		{
			this.rootObjects = new long[rootObjects.length];
			System.arraycopy(rootObjects, 0, this.rootObjects, 0, rootObjects.length);
		}
		else
		{
			this.rootObjects = null;
		}
	}
}
