/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.views.helpers;

import java.util.List;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.MapLinkDataSource;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * Utility class for editing map links 
 */
public class LinkEditor
{
	private NetworkMapLink link;
	private String name;
	private int type;
   private long element1;
   private long element2;
   private long interfaceId1;
   private long interfaceId2;
	private String connectorName1;
	private String connectorName2;
	private int color;
	private int colorSource;
   private String colorProvider;
	private List<Long> statusObjects;
	private int routingAlgorithm;
	private boolean modified = false;
	private List<MapLinkDataSource> dciList;
	private boolean useActiveThresholds;
   private boolean useInterfaceUtilization;
	private int labelPosition;
	private boolean disableLinkTextAutoUpdate;
   private boolean excludeFromAutomaticUpdates;
	private int lineStyle;
	private int lineWidth;

	/**
	 * @param link
	 * @param mapPage
	 */
	public LinkEditor(NetworkMapLink link, NetworkMapPage mapPage, boolean disableLinkTextAutoUpdate)
	{
		this.link = link;
		name = link.getName();
		type = link.getType();
      interfaceId1 = link.getInterfaceId1();
      interfaceId2 = link.getInterfaceId2();
      element1 = ((NetworkMapObject)mapPage.getElement(link.getElement1(), NetworkMapObject.class)).getObjectId();
      element2 = ((NetworkMapObject)mapPage.getElement(link.getElement2(), NetworkMapObject.class)).getObjectId();
		connectorName1 = link.getConnectorName1();
		connectorName2 = link.getConnectorName2();
		color = link.getColor();
		colorSource = link.getColorSource();
      colorProvider = link.getColorProvider();
		statusObjects = link.getStatusObjects();
		routingAlgorithm = link.getRouting();
		dciList = link.getDciAsList();
		useActiveThresholds = link.getConfig().isUseActiveThresholds();
      useInterfaceUtilization = link.getConfig().isUseInterfaceUtilization();
		labelPosition = link.getConfig().getLabelPosition();
		lineStyle = link.getConfig().getStyle();
		lineWidth = link.getConfig().getWidth();
		this.disableLinkTextAutoUpdate = disableLinkTextAutoUpdate;
		excludeFromAutomaticUpdates = link.isExcludedFromAutomaticUpdate();
	}

	/**
	 * Update network map link
	 * @param mapPage 
	 */
	public boolean update(NetworkMapPage mapPage)
	{
	   NetworkMapLink currentLink = null;
	   for (NetworkMapLink l : mapPage.getLinks())
	   {
	      if (l.getId() == link.getId())
	      {
	         currentLink = l;
	      }
	   }
	   if (currentLink == null)
	   {
	      return false;
	   }

		long[] bp = currentLink.getBendPoints();
		mapPage.removeLink(link.getId());
      link = new NetworkMapLink(link.getId(), name, type, currentLink.getElement1(), interfaceId1, currentLink.getElement2(), interfaceId2, connectorName1,
            connectorName2, (dciList != null) ? dciList.toArray(new MapLinkDataSource[dciList.size()]) : new MapLinkDataSource[0], currentLink.getFlags());
		link.setColor(color);
		link.setColorSource(colorSource);
      link.setColorProvider(colorProvider);
		link.setStatusObjects(statusObjects);
		link.setRouting(routingAlgorithm);
		link.setBendPoints(bp);
		link.getConfig().setUseActiveThresholds(useActiveThresholds);
      link.getConfig().setUseInterfaceUtilization(useInterfaceUtilization);
		link.getConfig().setLabelPosition(labelPosition);
		link.getConfig().setStyle(lineStyle);
      link.getConfig().setWidth(lineWidth);
      link.setExcludedFromAutomaticUpdate(excludeFromAutomaticUpdates);
		mapPage.addLink(link);
		modified = true;
		return true;
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
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
    * @return the interfaceId1
    */
   public long getInterfaceId1()
   {
      return interfaceId1;
   }

   /**
    * @param interfaceId1 the interfaceId1 to set
    */
   public void setInterfaceId1(long interfaceId1)
   {
      this.interfaceId1 = interfaceId1;
   }

   /**
    * @return the interfaceId2
    */
   public long getInterfaceId2()
   {
      return interfaceId2;
   }

   /**
    * @param interfaceId2 the interfaceId2 to set
    */
   public void setInterfaceId2(long interfaceId2)
   {
      this.interfaceId2 = interfaceId2;
   }

   /**
    * @return the connectorName1
    */
	public String getConnectorName1()
	{
		return connectorName1;
	}

	/**
	 * @param connectorName1 the connectorName1 to set
	 */
	public void setConnectorName1(String connectorName1)
	{
		this.connectorName1 = connectorName1;
	}

	/**
	 * @return the connectorName2
	 */
	public String getConnectorName2()
	{
		return connectorName2;
	}

	/**
	 * @param connectorName2 the connectorName2 to set
	 */
	public void setConnectorName2(String connectorName2)
	{
		this.connectorName2 = connectorName2;
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return color;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
		this.color = color;
	}

	/**
    * @return the colorSource
    */
   public int getColorSource()
   {
      return colorSource;
   }

   /**
    * @param colorSource the colorSource to set
    */
   public void setColorSource(int colorSource)
   {
      this.colorSource = colorSource;
   }

   /**
    * @return the colorProvider
    */
   public String getColorProvider()
   {
      return colorProvider;
   }

   /**
    * @param colorProvider the colorProvider to set
    */
   public void setColorProvider(String colorProvider)
   {
      this.colorProvider = colorProvider;
   }

   /**
    * @return the statusObject
    */
	public List<Long> getStatusObjects()
	{
		return statusObjects;
	}

	/**
	 * @param statusObject the statusObject to set
	 */
	public void setStatusObjects(List<Long> statusObject)
	{
		this.statusObjects = statusObject;
	}
	
	/**
    * @param id the id of status object to be added
    */
	public void addStatusObject(Long id)
	{
	   statusObjects.add(id);
	}
	
	/**
    * @param index the index of status the object to be removed
    */
	public void removeStatusObjectByIndex(int index)
	{
	   statusObjects.remove(index);
	}

	/**
	 * @return the routingAlgorithm
	 */
	public int getRoutingAlgorithm()
	{
		return routingAlgorithm;
	}

	/**
	 * @param routingAlgorithm the routingAlgorithm to set
	 */
	public void setRoutingAlgorithm(int routingAlgorithm)
	{
		this.routingAlgorithm = routingAlgorithm;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

   /**
    * @return the modified
    */
   public void setModified()
   {
      modified = true;
   }

   /**
    * @return the config
    */
   public List<MapLinkDataSource> getDciList()
   {
      return dciList;
   }

   /**
    * @param config the config to set
    */
   public void setDciList(List<MapLinkDataSource> dciList)
   {
      this.dciList = dciList;
   }
   
   /**
    * @return are active thresholds used
    */
   public boolean isUseActiveThresholds()
   {
      return useActiveThresholds;
   }
   
   /**
    * @param useActiveThresholds set to use active thresholds
    */
   public void setUseActiveThresholds(boolean useActiveThresholds)
   {
      this.useActiveThresholds = useActiveThresholds;
   }

   /**
    * @return the useInterfaceUtilization
    */
   public boolean isUseInterfaceUtilization()
   {
      return useInterfaceUtilization;
   }

   /**
    * @param useInterfaceUtilization the useInterfaceUtilization to set
    */
   public void setUseInterfaceUtilization(boolean useInterfaceUtilization)
   {
      this.useInterfaceUtilization = useInterfaceUtilization;
   }

   /**
    * Get updated link object
    * 
    * @return updated link object
    */
   public NetworkMapLink getLink()
   {
      return link;
   }

   /**
    * @return the labelPosition
    */
   public int getLabelPosition()
   {
      return labelPosition;
   }

   /**
    * @param labelPosition the labelPosition to set
    */
   public void setLabelPosition(int labelPosition)
   {
      this.labelPosition = labelPosition;
   }

   /**
    * @return if link text auto update is disabled
    */
   public boolean isLinkTextUpdateDisabled()
   {
      return disableLinkTextAutoUpdate;
   }

   /**
    * @return the lineStyle
    */
   public int getLineStyle()
   {
      return lineStyle;
   }

   /**
    * @param lineStyle the lineStyle to set
    */
   public void setLineStyle(int lineStyle)
   {
      this.lineStyle = lineStyle;
   }

   /**
    * @return the lineWidth
    */
   public int getLineWidth()
   {
      return lineWidth;
   }

   /**
    * @param lineWidth the lineWidth to set
    */
   public void setLineWidth(int lineWidth)
   {
      this.lineWidth = lineWidth;
   }

   public boolean isExcludeFromAutomaticUpdates()
   {
      return excludeFromAutomaticUpdates;
   }

   public void setExcludeFromAutomaticUpdates(boolean excludeFromAutomaticUpdates)
   {
      this.excludeFromAutomaticUpdates = excludeFromAutomaticUpdates;
   }

   /**
    * Get element 1 object id
    * 
    * @return object id of element 1
    */
   public long getElement1()
   {
      return element1;
   }

   /**
    * Get element 2 object id
    * 
    * @return object id of element 2
    */
   public long getElement2()
   {
      return element2;
   }
}
