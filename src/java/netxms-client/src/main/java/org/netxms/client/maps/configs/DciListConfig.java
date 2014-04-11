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
package org.netxms.client.maps.configs;

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Base class for all DCI configuration for line
 */
@Root(name="config")
public class DciListConfig
{
   /**
    * Create DCI list object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static DciListConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(DciListConfig.class, xml);
   }
   
   /**
    * Create XML from configuration.
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
	@ElementArray(required = true)
	private SingleDciConfig[] dciList = new SingleDciConfig[0];
	
	@Element(required=false)
	private int backgroundColor;
	
	@Element(required=false)
   private int textColor;
	
	@Element(required=false)
   private int borderColor;
	
	@Element(required=false)
   private boolean borderRequired;

	/**
	 * @return the dciList
	 */
	public SingleDciConfig[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(SingleDciConfig[] dciList)
	{
		this.dciList = dciList;
	}

   /**
    * @return the backgroundColor
    */
   public int getBackgroundColor()
   {
      return backgroundColor;
   }

   /**
    * @param backgroundColor the backgroundColor to set
    */
   public void setBackgroundColor(int backgroundColor)
   {
      this.backgroundColor = backgroundColor;
   }

   /**
    * @return the textColor
    */
   public int getTextColor()
   {
      return textColor;
   }

   /**
    * @param textColor the textColor to set
    */
   public void setTextColor(int textColor)
   {
      this.textColor = textColor;
   }

   /**
    * @return the borderColor
    */
   public int getBorderColor()
   {
      return borderColor;
   }

   /**
    * @param borderColor the borderColor to set
    */
   public void setBorderColor(int borderColor)
   {
      this.borderColor = borderColor;
   }

   /**
    * @return the borderRequired
    */
   public boolean isBorderRequired()
   {
      return borderRequired;
   }

   /**
    * @param borderRequired the borderRequired to set
    */
   public void setBorderRequired(boolean borderRequired)
   {
      this.borderRequired = borderRequired;
   }
}
