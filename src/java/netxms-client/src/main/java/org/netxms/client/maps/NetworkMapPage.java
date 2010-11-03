/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client.maps;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.GenericObject;

/**
 * Represents single map page (submap)
 * @author Victor
 *
 */
public class NetworkMapPage
{
	private String name;
	private Set<NetworkMapObjectData> objects = new HashSet<NetworkMapObjectData>(0);
	private Set<NetworkMapObjectLink> links = new HashSet<NetworkMapObjectLink>(0);

	/**
	 * Create empty unnamed page
	 */
	public NetworkMapPage()
	{
		name = "";
	}

	/**
	 * Create empty named page
	 */
	public NetworkMapPage(final String name)
	{
		this.name = name;
	}
	
	/**
	 * Add object to map
	 */
	public void addObject(final NetworkMapObjectData object)
	{
		objects.add(object);
	}

	/**
	 * Add link between objects to map
	 */
	public void addLink(final NetworkMapObjectLink link)
	{
		links.add(link);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the objects
	 */
	public Set<NetworkMapObjectData> getObjects()
	{
		return objects;
	}

	/**
	 * @return the links
	 */
	public Set<NetworkMapObjectLink> getLinks()
	{
		return links;
	}
	
	/**
	 * Get objects and links in one array - objects as NXCObject and subclasses,
	 * links as NXCMapObjectLink
	 * @param session Client session which should be used for object lookup
	 * @return Objects and links in one array
	 */
	public Object[] getObjectsAndLinks(NXCSession session)
	{
		Object[] list = new Object[objects.size() + links.size()];
		
		Iterator<NetworkMapObjectData> oit = objects.iterator();
		for(int i = 0; oit.hasNext(); i++)
		{
			NetworkMapObjectData data = oit.next();
			long id = data.getObjectId();
			list[i] = session.findObjectById(id);
		}
		
		Iterator<NetworkMapObjectLink> lit = links.iterator();
		for(int i = objects.size(); lit.hasNext(); i++)
		{
			list[i] = lit.next();
		}

		return list;
	}

	/**
	 * Get objects as NXCObject and subclasses
	 * @param session Client session which should be used for object lookup
	 * @return NetXMS objects
	 */
	public GenericObject[] getResolvedObjects(NXCSession session)
	{
		GenericObject[] list = new GenericObject[objects.size()];
		
		Iterator<NetworkMapObjectData> it = objects.iterator();
		for(int i = 0; it.hasNext(); i++)
		{
			NetworkMapObjectData data = it.next();
			long id = data.getObjectId();
			list[i] = session.findObjectById(id);
		}

		return list;
	}
	
	/**
	 * Get all objects connected to given object
	 * @param root Root object id
	 * @param session Client session which should be used for object lookup
	 * @return All objects connected to given object
	 */
	public GenericObject[] getConnectedObjects(long root, NXCSession session)
	{
		Set<GenericObject> result = new HashSet<GenericObject>(0);
		
		Iterator<NetworkMapObjectLink> it = links.iterator();
		while(it.hasNext())
		{
			NetworkMapObjectLink link = it.next();
			if (link.getObject1() == root)
			{
				long id = link.getObject2();
				GenericObject object = session.findObjectById(id);
				if (object != null)
					result.add(object);
			}
/*			else if (link.getObject2() == root)
			{
				long id = link.getObject1();
				GenericObject object = session.findObjectById(id);
				if (object != null)
					result.add(object);
			}*/
		}
		return result.toArray(new GenericObject[result.size()]);
	}
}
