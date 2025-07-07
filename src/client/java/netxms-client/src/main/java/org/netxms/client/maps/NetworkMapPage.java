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
import java.util.Map.Entry;
import java.util.Set;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * Network map object representation used by visualisation tools
 *
 */
public class NetworkMapPage
{
	private String id;
	private long nextElementId;
   private long nextLinkId;
   private long mapObjectId;
	private Map<Long, NetworkMapElement> elements = new HashMap<Long, NetworkMapElement>(0);
   private Map<Long, NetworkMapLink> links = new HashMap<Long, NetworkMapLink>(0);

	/**
	 * Create empty named page.
	 * 
	 * @param id page ID
	 */
	public NetworkMapPage(final String id, final long mapObjectId)
	{
		this.id = id;
		this.mapObjectId = mapObjectId;
		nextElementId = 1;
      nextLinkId = 1;
	}

   /**
    * Create empty named page.
    * 
    * @param id page ID
    */
   public NetworkMapPage(final String id)
   {
      this.id = id;
      this.mapObjectId = 0;
      nextElementId = 1;
      nextLinkId = 1;
   }
	
	/**
	 * Add element to map
	 * 
	 * @param element element to add
	 */
	public void addElement(final NetworkMapElement element)
	{
		elements.put(element.getId(), element);
		if (element.getId() >= nextElementId)
			nextElementId = element.getId() + 1;
	}

	/**
	 * Add all elements from given collection.
	 *
	 * @param set set of elements to add
	 */
	public void addAllElements(Collection<NetworkMapElement> set)
	{
		for(NetworkMapElement e : set)
			addElement(e);
	}

	/**
	 * Add link between elements to map
	 * 
	 * @param link link to add
	 */
	public void addLink(final NetworkMapLink link)
	{
	   link.resetPosition();
      for(NetworkMapLink l : links.values())
	   {
	      if (((l.getElement1() == link.getElement1() && l.getElement2() == link.getElement2()) ||
	           (l.getElement1() == link.getElement2() && l.getElement2() == link.getElement1())) &&
	          (l.getId() != link.getId()))
	      {
	         l.updatePosition();
	         link.setCommonFirstElement(l.getCommonFirstElement());
	         link.setDuplicateCount(l.getDuplicateCount());
	      }
	   }
      links.put(link.getId(), link);
      if (link.getId() >= nextLinkId)
         nextLinkId = link.getId() + 1;
	}

	/**
	 * Add all links from given collection
	 * 
	 * @param set set of links to add
	 */
	public void addAllLinks(Collection<NetworkMapLink> set)
	{
	   for(NetworkMapLink l : set)
	      addLink(l);
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

      Iterator<Entry<Long, NetworkMapLink>> it = links.entrySet().iterator();
		while(it.hasNext())
		{
         NetworkMapLink l = it.next().getValue();
			if ((l.getElement1() == elementId) || (l.getElement2() == elementId))
			{
				it.remove();
			}
		}
	}

	/**
	 * Remove map element representing NetXMS object by NetXMS object ID.
	 * 
	 * @param objectId NetXMS object ID
	 */
	public void removeObjectElement(long objectId)
	{
		long elementId = -1;
		for(NetworkMapElement element : elements.values())
		{
			if (element instanceof NetworkMapObject)
			{
				if (((NetworkMapObject)element).getObjectId() == objectId)
				{
					elementId = element.getId();
					break;
				}
			}
		}

		if (elementId != -1)
		{
			removeElement(elementId);
		}
	}

	/**
	 * Remove link between objects
	 * 
	 * @param link link to be removed
	 */
	public void removeLink(NetworkMapLink link)
	{
      links.remove(link.getId());
	}

   /**
    * Remove link between objects
    * 
    * @param id link ID
    */
   public void removeLink(long id)
   {
      links.remove(id);
   }

	/**
	 * Get page ID.
	 *
	 * @return page ID
	 */
	public String getId()
	{
		return id;
	}

	/**
	 * Set page ID.
	 *
	 * @param id new page ID
	 */
	public void setId(String id)
	{
		this.id = id;
	}

	/**
	 * Get map elements.
	 *
	 * @return map elements
	 */
	public Collection<NetworkMapElement> getElements()
	{
		return elements.values();
	}

	/**
	 * Get links between elements.
	 *
	 * @return links between elements
	 */
	public Collection<NetworkMapLink> getLinks()
	{
      return links.values();
	}

	/**
	 * Get IDs of all objects on map
	 * 
	 * @return IDs of all objects on map
	 */
	public List<Long> getObjectIds()
	{
	   List<Long> objects = new ArrayList<Long>();
	   for(NetworkMapElement e : elements.values())
	   {
	      if (e.getType() == NetworkMapElement.MAP_ELEMENT_OBJECT)
	         objects.add(((NetworkMapObject)e).getObjectId());
	   }
	   return objects;
	}

   /**
    * Get all object elements
    * 
    * @return all object elements
    */
   public List<NetworkMapElement> getObjectElements()
   {
      List<NetworkMapElement> objects = new ArrayList<NetworkMapElement>();
      for(NetworkMapElement e : elements.values())
      {
         if (e.getType() == NetworkMapElement.MAP_ELEMENT_OBJECT)
            objects.add(e);
      }
      return objects;
   }
   
	/**
	 * Create new unique element ID
	 * 
	 * @return new unique element ID
	 */
	public long createElementId()
	{
		return nextElementId++;
	}
	
   /**
    * Create new unique link ID
    * 
    * @return new unique link ID
    */
   public long createLinkId()
   {
      return nextLinkId++;
   }

	/**
    * Find object element by NetXMS object ID.
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
	 * Find links from source to destination
	 * 
	 * @param source source element
	 * @param destination destination element
	 * @return link between source and destination or null if there are no such link 
	 */
	public List<NetworkMapLink> findLinks(NetworkMapElement source, NetworkMapElement destination)
	{
	   List<NetworkMapLink> result = new ArrayList<NetworkMapLink>();
      for(NetworkMapLink l : links.values())
			if ((l.getElement1() == source.getId()) && (l.getElement2() == destination.getId()))
				result.add(l);
		return result;
	}

	/**
	 * Find all links using given object as status source
	 * 
	 * @param objectId status source object id
	 * @return list of link using this object
	 */
	public List<NetworkMapLink> findLinksWithStatusObject(long objectId)
	{
		List<NetworkMapLink> list = null;
      for(NetworkMapLink l : links.values())
		{
		   for(Long obj : l.getStatusObjects())
			if (obj == objectId)
			{
				if (list == null)
					list = new ArrayList<NetworkMapLink>();
				list.add(l);
			}
		}
		return list;
	}

   /**
    * Get all object used as status source for links and as an utilization source
    */
   public void getAllLinkStatusAndUtilizationObjects(Set<Long> objects, Set<Long> utilizationObjects)
   {
      for(NetworkMapLink l : links.values())
      {
         for(Long obj : l.getStatusObjects())
            objects.add(obj);
         if (l.getInterfaceId1() > 0)
         {
            objects.add(l.getInterfaceId1());
            if (l.getColorSource() == NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION || l.getConfig().isUseInterfaceUtilization())
            {
               utilizationObjects.add(l.getInterfaceId1());
            }
         }
         if (l.getInterfaceId2() > 0)
         {
            objects.add(l.getInterfaceId2());
            if (l.getColorSource() == NetworkMapLink.COLOR_SOURCE_LINK_UTILIZATION || l.getConfig().isUseInterfaceUtilization())
            {
               utilizationObjects.add(l.getInterfaceId2());
            }
         }
      }
   }

   /**
    * Get all object used as status source for links
    * 
    * @return set of status source objects
    */
   public Set<Long> getAllLinkStatusObjects()
   {
      Set<Long> objects = new HashSet<Long>();
      for(NetworkMapLink l : links.values())
      {
         for(Long obj : l.getStatusObjects())
            objects.add(obj);
         if (l.getInterfaceId1() > 0)
            objects.add(l.getInterfaceId1());
         if (l.getInterfaceId2() > 0)
            objects.add(l.getInterfaceId2());
      }
      return objects;
   }

	/**
	 * Checks if two objects are connected
	 * 
	 * @param elementId1 ID of first map element
	 * @param elementId2 ID of second map element
	 * @return true if given elements are connected
	 */
	public boolean areObjectsConnected(long elementId1, long elementId2)
	{
      for(NetworkMapLink l : links.values())
			if (((l.getElement1() == elementId1) && (l.getElement2() == elementId2)) ||
			    ((l.getElement1() == elementId2) && (l.getElement2() == elementId1)))
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
      for(NetworkMapLink l : links.values())
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
		
      for(NetworkMapLink link : links.values())
		{
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

   /**
    * Update existing on the map element
    * 
    * @param element element with data to update
    * @return true if element updated, false if element not found
    */
   public boolean updateElement(NetworkMapElement element)
   {
      if (elements.containsKey(element.getId()))
      {
         addElement(element);
         return true;
      }
      return false;
   }

   /**
    * @return the mapObjectId
    */
   public long getMapObjectId()
   {
      return mapObjectId;
   }

   /**
    * @param link
    */
   public NetworkMapLink findLink(NetworkMapLink link)
   {
      for(NetworkMapLink l : links.values())
      {
         if (l.getElement1() == link.getElement1() && l.getElement2() == link.getElement2() &&
               l.getDuplicateCount() == link.getDuplicateCount())
            return l;
      }
      return null;
      
   }
}
