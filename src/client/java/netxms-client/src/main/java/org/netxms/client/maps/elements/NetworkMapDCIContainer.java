/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.client.maps.elements;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.DciContainerConfiguration;
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.client.xml.XMLTools;

/**
 * Network map element representing NetXMS DCI container.
 */
public class NetworkMapDCIContainer extends NetworkMapElement
{
   private int backgroundColor;
   private int textColor;
   private int borderColor;
   private boolean borderRequired;
	private SingleDciConfig[] dciArray;

	/**
	 * Create DCI container from NXCP message.
	 *
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	protected NetworkMapDCIContainer(NXCPMessage msg, long baseId)
	{
		super(msg, baseId);
      String xml = msg.getFieldAsString(baseId + 10);
		try
      {
         DciContainerConfiguration config = XMLTools.createFromXml(DciContainerConfiguration.class, xml);
		   backgroundColor = config.getBackgroundColor();
		   textColor = config.getTextColor();
		   borderColor = config.getBorderColor();
		   borderRequired = config.isBorderRequired();
		   dciArray = config.getDciList();
      }
      catch(Exception e)
      {
         backgroundColor = 0xFFFFFF;
         textColor = 0x202020;
         borderColor = 0x202020;
         borderRequired = false;
         dciArray = null;
      }
	}

	/**
	 * Create new DCI container element.
	 * 
	 * @param id element ID
	 */
	public NetworkMapDCIContainer(long id)
	{
		super(id);
		type = MAP_ELEMENT_DCI_CONTAINER;
		dciArray = null;
      backgroundColor = 0xFFFFFF;
      textColor = 0x202020;
      borderColor = 0x202020;
	}

   /**
    * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
    */
   @Override
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      super.fillMessage(msg, baseId);

      DciContainerConfiguration config = new DciContainerConfiguration();
      config.setDciList(dciArray);
      config.setBackgroundColor(backgroundColor);
      config.setTextColor(textColor);
      config.setBorderColor(borderColor);
      config.setBorderRequired(borderRequired);
      try
      {
         msg.setField(baseId + 10, config.createXml());
      }
      catch(Exception e)
      {
         msg.setField(baseId + 10, "");
      }
   }

	/**
	 * Check if container has any data to display.
	 *
    * @return returns if DCI list is not empty
    */
   public boolean hasDciData()
   {
      return (dciArray != null) && (dciArray.length > 0);
   }

	/**
	 * Get DCI list for this container. Returned array is owned by container and changes to it will be reflected in container.
	 *
	 * @return DCI list for this container
	 */
	public SingleDciConfig[] getObjectDCIArray()
	{
		return dciArray;
	}

	/**
	 * Set DCI list for this container. Container will become owner of the list.
	 *
	 * @param dciArray DCI list for this container
	 */
	public void setObjectDCIArray(SingleDciConfig[] dciArray)
	{
	   this.dciArray = dciArray;
	}

	/**
    * Get configured DCIs as list. Changes to returned list object will not affect container configuration. 
    * 
    * @return configured DCIs as list
    */
   public List<SingleDciConfig> getDciAsList()
   {
      List<SingleDciConfig> dciList = new ArrayList<SingleDciConfig>();
      if(hasDciData())
      {
         for(SingleDciConfig dci : dciArray)
            dciList.add(dci);
      }
      return dciList;
   }

   /**
    * Get background color.
    *
    * @return background color in BGR format
    */
   public int getBackgroundColor()
   {
      return backgroundColor;
   }

   /**
    * Set background color (in BGR format).
    *
    * @param backgroundColor new background color in BGR format
    */
   public void setBackgroundColor(int backgroundColor)
   {
      this.backgroundColor = backgroundColor;
   }

   /**
    * Get text color.
    *
    * @return text color in BGR format
    */
   public int getTextColor()
   {
      return textColor;
   }

   /**
    * Set text color (in BGR format).
    *
    * @param textColor new text color in BGR format
    */
   public void setTextColor(int textColor)
   {
      this.textColor = textColor;
   }

   /**
    * Get border color.
    *
    * @return border color in BGR format
    */
   public int getBorderColor()
   {
      return borderColor;
   }

   /**
    * Set border color (in BGR format).
    *
    * @param borderColor new border color in BGR format
    */
   public void setBorderColor(int borderColor)
   {
      this.borderColor = borderColor;
   }

   /**
    * Check if "show border" flag is set for this element.
    *
    * @return true if "show border" flag is set
    */
   public boolean isBorderRequired()
   {
      return borderRequired;
   }

   /**
    * Set "show border" flag.
    *
    * @param borderRequired true if border should be shown
    */
   public void setBorderRequired(boolean borderRequired)
   {
      this.borderRequired = borderRequired;
   }
}
