/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.io.StringWriter;
import java.io.Writer;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.DciIdMatchingData;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for table last value widget
 */
public class TableValueConfig extends DashboardElementConfig
{
	@Element(required=true)
	private long objectId = 0;

	@Element(required=true)
	private long dciId = 0;

	@Element(required=false)
	private String title = ""; //$NON-NLS-1$
	
	@Element(required = false)
	private int refreshRate = 30;

	/**
	 * Create line chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static TableValueConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(TableValueConfig.class, xml);
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#createXml()
	 */
	@Override
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getObjects()
	 */
	@Override
	public Set<Long> getObjects()
	{
		Set<Long> objects = super.getObjects();
		objects.add(objectId);
		return objects;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapObjects(java.util.Map)
	 */
	@Override
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
		super.remapObjects(remapData);
		ObjectIdMatchingData md = remapData.get(objectId);
		if (md != null)
			objectId = md.dstId;
	}

	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getDataCollectionItems()
    */
   @Override
   public Map<Long, Long> getDataCollectionItems()
   {
      Map<Long, Long> dcis = new HashMap<Long, Long>();
      dcis.put(dciId,  objectId);
      return dcis;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapDataCollectionItems(java.util.Map)
    */
   @Override
   public void remapDataCollectionItems(Map<Long, DciIdMatchingData> remapData)
   {
      super.remapDataCollectionItems(remapData);
      DciIdMatchingData md = remapData.get(dciId);
      if (md != null)
      {
         objectId = md.dstNodeId;
         dciId = md.dstDciId;
      }
   }

   /**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @param objectId the objectId to set
	 */
	public void setObjectId(long objectId)
	{
		this.objectId = objectId;
	}

	/**
	 * @return
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @param dciId
	 */
	public void setDciId(long dciId)
	{
		this.dciId = dciId;
	}

	/**
	 * @return
	 */
	public int getRefreshRate()
	{
		return refreshRate;
	}

	/**
	 * @param refreshRate
	 */
	public void setRefreshRate(int refreshRate)
	{
		this.refreshRate = refreshRate;
	}
}
