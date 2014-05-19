/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.UUID;
import org.netxms.base.NXCommon;
import org.netxms.client.datacollection.DataCollectionObject;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for map line
 */
@Root(name="rule")
public class DCIImageRule
{	
	public static final int ITEM = DataCollectionObject.DCO_TYPE_ITEM;
	public static final int TABLE = DataCollectionObject.DCO_TYPE_TABLE;
	
	
	@Element(required=true)
	private int comparisonType;

	@Element(required=true)
	private UUID image;

	@Element(required=true)
	private String compareValue;
	
	@Element(required=false)
	private String comment;
	
	/**
	 * Default constructor
	 */
	public DCIImageRule()
	{
	   comparisonType = -1;
	   image = NXCommon.EMPTY_GUID;
	   compareValue = "";
	}

	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public DCIImageRule(DCIImageRule src)
	{
	   comparisonType = src.getComparisonType();
	   image = src.getImage();
	   compareValue = src.getCompareValue();
	}	 

   /**
    * @return the comparisonType
    */
   public int getComparisonType()
   {
      return comparisonType;
   }

   /**
    * @param comparisonType the comparisonType to set
    */
   public void setComparisonType(int comparisonType)
   {
      this.comparisonType = comparisonType;
   }

   /**
    * @return the image
    */
   public UUID getImage()
   {
      return image;
   }

   /**
    * @param image the image to set
    */
   public void setImage(UUID image)
   {
      this.image = image;
   }

   /**
    * @return the compareValue
    */
   public String getCompareValue()
   {
      return compareValue == null ? "" : compareValue;
   }

   /**
    * @param compareValue the compareValue to set
    */
   public void setCompareValue(String compareValue)
   {
      this.compareValue = compareValue;
   }

   /**
    * @return the comment
    */
   public String getComment()
   {
      return comment == null ? "" : comment;
   }

   /**
    * @param comment the comment to set
    */
   public void setComment(String comment)
   {
      this.comment = comment;
   }
}
