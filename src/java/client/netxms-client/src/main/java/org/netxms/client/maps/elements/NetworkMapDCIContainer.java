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
package org.netxms.client.maps.elements;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.DciContainerConfiguration;
import org.netxms.client.maps.configs.SingleDciConfig;

/**
 * Network map element representing NetXMS DCI container
 *
 */
public class NetworkMapDCIContainer  extends NetworkMapElement
{
   private String DCIListXml = "";
   private int backgroundColor;
   private int textColor;
   private int borderColor;
   private boolean borderRequired;
	private SingleDciConfig[] dciArray;
	
	/**
	 * @param msg
	 * @param baseId
	 */
	protected NetworkMapDCIContainer(NXCPMessage msg, long baseId)
	{
		super(msg, baseId);
		DCIListXml = msg.getVariableAsString(baseId+10);
		try
      {
		   DciContainerConfiguration conf = DciContainerConfiguration.createFromXml(DCIListXml);
		   backgroundColor = conf.getBackgroundColor();
		   textColor = conf.getTextColor();
		   borderColor = conf.getBorderColor();
		   borderRequired = conf.isBorderRequired();
		   dciArray = conf.getDciList();
      }
      catch(Exception e)
      { 
         dciArray = null;
      }
		
	}
	
	/**
	 * Create new DCI container element
	 * 
	 * @param id element ID
	 */
	public NetworkMapDCIContainer(long id)
	{
		super(id);
		type = MAP_ELEMENT_DCI_CONTAINER;
		dciArray = null;
		backgroundColor = Integer.MAX_VALUE;//white
		textColor = 0;//black
		borderColor = 0;//balck
	}
	
	/**
    * @return returns if DCI list is not empty
    */
   public boolean hasDciData()
   {
      if(dciArray != null && dciArray.length > 0)
         return true;
      return false;
   }

	/**
	 * @return the objectDCIList
	 */
	public SingleDciConfig[] getObjectDCIArray()
	{
		return dciArray;
	}
	
	
	/**
	 * Setter for dciList
	 * 
	 * @param dciArray
	 */
	public void setObjectDCIArray(SingleDciConfig[] dciArray)
	{
	   this.dciArray = dciArray;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
	 */
	@Override
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		super.fillMessage(msg, baseId);
		DciContainerConfiguration dciList = new DciContainerConfiguration();
		dciList.setDciList(dciArray);
		dciList.setBackgroundColor(backgroundColor);
      dciList.setTextColor(textColor);
      dciList.setBorderColor(borderColor);
      dciList.setBorderRequired(borderRequired);
		try
      {
		   DCIListXml = dciList.createXml();
      }
      catch(Exception e)
      {
         DCIListXml = "";
      }
      msg.setVariable(baseId + 10, DCIListXml);
	}
	
	/**
    * Returns DCI array as a list
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
