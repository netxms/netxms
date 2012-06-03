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
package org.netxms.client.maps;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * Network map object representation used by visualisation tools
 *
 */
public class NetworkMapPage
{
	private String name;
	private long nextElementId;
	private Map<Long, NetworkMapElement> elements = new HashMap<Long, NetworkMapElement>(0);
	private Set<NetworkMapLink> links = new HashSet<NetworkMapLink>(0);

	/**
	 * Create empty unnamed page
	 */
	public NetworkMapPage()
	{
		name = "";
		nextElementId = 1;
	}

	/**
	 * Create empty named page
	 */
	public NetworkMapPage(final String name)
	{
		this.name = name;
		nextElementId = 1;
	}
	
	/**
	 * Add element to map
	 */
	public void addElement(final NetworkMapElement element)
	{
		elements.put(element.getId(), element);
		if (element.getId() >= nextElementId)
			nextElementId = element.getId() + 1;
	}
	
	/**
	 * Add all elements from given collection
	 * @param set
	 */
	public void addAllElements(Collection<NetworkMapElement> set)
	{
		for(NetworkMapElement e : set)
			addElement(e);
	}

	/**
	 * Add link between elements to map
	 */
	public void addLink(final NetworkMapLink link)
	{
		links.add(link);
	}
	
	/**
	 * Add all links from given collection
	 * 
	 * @param set
	 */
	public void addAllLinks(Collection<NetworkMapLink> set)
	{
		links.addAll(set);
	}
	
	/**
	 * Get map element by element ID.
	 * 
	 * @param elementId element ID
	 * @param requiredClass optional class filter (set to null to disable filtering)
	 * @return map element or null
	 */
	public NetworkMapElement getElement(long elementId, Class<? extends NetworkMapElement> requiredClass)
	{
		NetworkMapElement e = elements.get(elementId);
		if ((e == null) || (requiredClass == null))
			return e;
		return requiredClass.isInstance(e) ? e : null;
	}
	
	/**
	 * Remove element from map
	 * 
	 * @param elementId map element ID
	 */
	public void removeElement(long elementId)
	{
		elements.remove(elementId);
	}
	
	/**
	 * Remove map element representing NetXMS object by NetXMS object ID.
	 * 
	 * @param objectId NetXMS object ID
	 */
	public void removeObjectElement(long objectId)
	{
		for(NetworkMapElement element : elements.values())
		{
			if (element instanceof NetworkMapObject)
			{
				if (((NetworkMapObject)element).getObjectId() == objectId)
				{
					elements.remove(element.getId());
					break;
				}
			}
		}
	}
	
	/**
	 * Remove link between objects
	 * 
	 * @param link
	 */
	public void removeLink(NetworkMapLink link)
	{
		links.remove(link);
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
	public Collection<NetworkMapElement> getElements()
	{
		return elements.values();
	}

	/**
	 * @return the links
	 */
	public Collection<NetworkMapLink> getLinks()
	{
		return links;
	}
	
	/**
	 * Create new unique element ID
	 * @return
	 */
	public long createElementId()
	{
		return nextElementId++;
	}
	
	/**
	 * Find object element by NeTXMS object ID.
	 * 
	 * @param objectId NetXMS object ID
	 * @return object element or null
	 */
	public NetworkMapObject findObjectElement(long objectId)
	{
		for(NetworkMapElement e : elements.values())
			if ((e instanceof NetworkMapObject) && (((NetworkMapObject)e).getObjectId() == objectId))
				return (NetworkMapObject)e;
		return null;
	}
	
	/**
	 * Find link from source to destination
	 * 
	 * @param source source element
	 * @param destination destination element
	 * @return link between source and destination or null if there are no such link 
	 */
	public NetworkMapLink findLink(NetworkMapElement source, NetworkMapElement destination)
	{
		for(NetworkMapLink l : links)
			if ((l.getElement1() == source.getId()) && (l.getElement2() == destination.getId()))
				return l;
		return null;
	}
	
	/**
	 * Find all links using given object as status source
	 * 
	 * @param source source element
	 * @param destination destination element
	 * @return link between source and destination or null if there are no such link 
	 */
	public List<NetworkMapLink> findLinksWithStatusObject(long objectId)
	{
		List<NetworkMapLink> list = null;
		for(NetworkMapLink l : links)
		{
			if (l.getStatusObject() == objectId)
			{
				if (list == null)
					list = new ArrayList<NetworkMapLink>();
				list.add(l);
			}
		}
		return list;
	}
	
	/**
	 * Checks if two objects are connected
	 * 
	 * @param objectId1 ID of first map object
	 * @param objectId2 ID of second map object
	 * @return true if given objects are connected
	 */
	public boolean areObjectsConnected(long objectId1, long objectId2)
	{
		for(NetworkMapLink l : links)
			if (((l.getElement1() == objectId1) && (l.getElement2() == objectId2)) ||
			    ((l.getElement1() == objectId2) && (l.getElement2() == objectId1)))
				return true;
		return false;
	}
	
	/**
	 * Get objects and links in one array
	 * @return Objects and links in one array
	 */
	public Object[] getElementsAndLinks()
	{
		Object[] list = new Object[elements.size() + links.size()];
		int i = 0;
		
		for(NetworkMapElement e : elements.values())
			list[i++] = e;
		for(NetworkMapLink l : links)
			list[i++] = l;

		return list;
	}

	/**
	 * Get all elements connected to given element
	 * @param root Root element id
	 * @return All elements connected to given element
	 */
	public NetworkMapElement[] getConnectedElements(long root)
	{
		Set<NetworkMapElement> result = new HashSet<NetworkMapElement>(0);
		
		Iterator<NetworkMapLink> it = links.iterator();
		while(it.hasNext())
		{
			NetworkMapLink link = it.next();
			if (link.getElement1() == root)
			{
				long id = link.getElement2();
				NetworkMapElement e = elements.get(id);
				if (e != null)
					result.add(e);
			}
		}
		return result.toArray(new NetworkMapElement[result.size()]);
	}
}
