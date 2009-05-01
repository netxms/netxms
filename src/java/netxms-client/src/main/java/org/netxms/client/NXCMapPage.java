/**
 * 
 */
package org.netxms.client;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

/**
 * Represents single map page (submap)
 * @author Victor
 *
 */
public class NXCMapPage
{
	private String name;
	private Set<NXCMapObjectData> objects = new HashSet<NXCMapObjectData>(0);
	private Set<NXCMapObjectLink> links = new HashSet<NXCMapObjectLink>(0);

	/**
	 * Create empty unnamed page
	 */
	public NXCMapPage()
	{
		name = "";
	}

	/**
	 * Create empty named page
	 */
	public NXCMapPage(final String name)
	{
		this.name = name;
	}
	
	/**
	 * Add object to map
	 */
	public void addObject(final NXCMapObjectData object)
	{
		objects.add(object);
	}

	/**
	 * Add link between objects to map
	 */
	public void addLink(final NXCMapObjectLink link)
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
	public Set<NXCMapObjectData> getObjects()
	{
		return objects;
	}

	/**
	 * @return the links
	 */
	public Set<NXCMapObjectLink> getLinks()
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
		
		Iterator<NXCMapObjectData> oit = objects.iterator();
		for(int i = 0; oit.hasNext(); i++)
		{
			NXCMapObjectData data = oit.next();
			long id = data.getObjectId();
			list[i] = session.findObjectById(id);
		}
		
		Iterator<NXCMapObjectLink> lit = links.iterator();
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
	public NXCObject[] getResolvedObjects(NXCSession session)
	{
		NXCObject[] list = new NXCObject[objects.size()];
		
		Iterator<NXCMapObjectData> it = objects.iterator();
		for(int i = 0; it.hasNext(); i++)
		{
			NXCMapObjectData data = it.next();
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
	public NXCObject[] getConnectedObjects(long root, NXCSession session)
	{
		Set<NXCObject> result = new HashSet<NXCObject>(0);
		
		Iterator<NXCMapObjectLink> it = links.iterator();
		while(it.hasNext())
		{
			NXCMapObjectLink link = it.next();
			if (link.getObject1() == root)
			{
				long id = link.getObject2();
				NXCObject object = session.findObjectById(id);
				if (object != null)
					result.add(object);
			}
			else if (link.getObject1() == root)
			{
				long id = link.getObject1();
				NXCObject object = session.findObjectById(id);
				if (object != null)
					result.add(object);
			}
		}
		return result.toArray(new NXCObject[result.size()]);
	}
}
