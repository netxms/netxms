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
package org.netxms.nxmc.modules.dashboards.config;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.DciIdMatchingData;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import com.google.gson.Gson;

/**
 * Abstract base class for all dashboard element configs
 */
@Root(name = "element", strict = false)
public class DashboardElementConfig
{
	private DashboardElementLayout layout;

   @Element(required = false)
   private String title = ""; //$NON-NLS-1$

   @Element(required = false)
   private String titleForeground = null;

   @Element(required = false)
   private String titleBackground = null;

   @Element(required = false)
   private int titleFontSize = 0; // Adjustment from standard size

   @Element(required = false)
   private String titleFontName = null;

	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
    */
	public String createXml() throws Exception
	{
      return XMLTools.serialize(this);
	}

   /**
    * Create Json form object
    * 
    * @return json string
    */
   public String createJson()
   {
      return new Gson().toJson(this);
   }

	/**
	 * @return the layout
	 */
	public DashboardElementLayout getLayout()
	{
		return layout;
	}

	/**
	 * @param layout the layout to set
	 */
	public void setLayout(DashboardElementLayout layout)
	{
		this.layout = layout;
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
    * @return the titleForeground
    */
   public String getTitleForeground()
   {
      return titleForeground;
   }

   /**
    * @param titleForeground the titleForeground to set
    */
   public void setTitleForeground(String titleForeground)
   {
      this.titleForeground = titleForeground;
   }

   /**
    * @return the titleBackground
    */
   public String getTitleBackground()
   {
      return titleBackground;
   }

   /**
    * @param titleBackground the titleBackground to set
    */
   public void setTitleBackground(String titleBackground)
   {
      this.titleBackground = titleBackground;
   }

   /**
    * @return the titleFontSize
    */
   public int getTitleFontSize()
   {
      return titleFontSize;
   }

   /**
    * @param titleFontSize the titleFontSize to set
    */
   public void setTitleFontSize(int titleFontSize)
   {
      this.titleFontSize = titleFontSize;
   }

   /**
    * @return the titleFontName
    */
   public String getTitleFontName()
   {
      return titleFontName;
   }

   /**
    * @param titleFontName the titleFontName to set
    */
   public void setTitleFontName(String titleFontName)
   {
      this.titleFontName = (titleFontName != null) && titleFontName.isBlank() ? null : titleFontName;
   }

   /**
    * Get list of referenced object IDs
    * 
    * @return
    */
	public Set<Long> getObjects()
	{
		return new HashSet<Long>(0);
	}

	/**
	 * Get list of referenced DCI IDs. Key is DCI iD and value is node ID.
	 * 
	 * @return
	 */
	public Map<Long, Long> getDataCollectionItems()
	{
		return new HashMap<Long, Long>(0);
	}

	/**
	 * Remap object identifiers. Remap data has source object ID as a key.
	 * 
	 * @param remapData
	 */
	public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
	{
	}

	/**
	 * Remap DCI identifiers. Remap data has source DCI ID as a key.
	 * 
	 * @param remapData
	 */
	public void remapDataCollectionItems(Map<Long, DciIdMatchingData> remapData)
	{
	}
}
