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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Column definition for DCI summary table
 */
public class DciSummaryTableColumn
{
	public static int REGEXP_MATCH = 0x0001;
   public static int MULTIVALUED = 0x0002;
   public static int DESCRIPTION_MATCH = 0x0004;

	private String name;
	private String dciName;
	private int flags;
	private String separator;

	/**
	 * @param name The column name
	 * @param dciName The dci name
	 * @param flags The flags
	 * @param separator Separator for multivalued columns
	 */
	public DciSummaryTableColumn(String name, String dciName, int flags, String separator)
	{
		this.name = name;
		this.dciName = dciName;
		this.flags = flags;
		this.separator = separator;
	}
	
   /**
    * @param name The column name
    * @param dciName The dci name
    * @param flags The flags
    */
   public DciSummaryTableColumn(String name, String dciName, int flags)
   {
      this(name, dciName, flags, ";");
   }
   
   /**
    * @param name The column name
    * @param dciName The dci name
    */
   public DciSummaryTableColumn(String name, String dciName)
   {
      this(name, dciName, 0, ";");
   }
   
	/**
	 * Copy constructor.
	 * 
	 * @param src The source object
	 */
	public DciSummaryTableColumn(DciSummaryTableColumn src)
	{
		name = src.name;
		dciName = src.dciName;
		flags = src.flags;
		separator = src.separator;
	}
	
	/**
	 * @param msg The NXCPMessage
	 * @param baseId The base ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
	   msg.setField(baseId, name);
      msg.setField(baseId + 1, dciName);
      msg.setFieldInt32(baseId + 2, flags);
      msg.setField(baseId + 3, separator);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the dciName
	 */
	public String getDciName()
	{
		return dciName;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @param dciName the dciName to set
	 */
	public void setDciName(String dciName)
	{
		this.dciName = dciName;
	}
	
	/**
    * Get separator for multivalued column
    * 
    * @return separator for multivalued column
    */
   public String getSeparator()
   {
      return separator;
   }

   /**
    * Set separator for multivalued column
    * 
    * @param separator new separator for multivalued column
    */
   public void setSeparator(String separator)
   {
      this.separator = ((separator != null) && !separator.isEmpty()) ? separator : ";";
   }

   /**
	 * @return true if match
	 */
   public boolean isDescriptionMatch()
	{
      return (flags & DESCRIPTION_MATCH) != 0;
	}
	
	/**
	 * @param enable true to enable
	 */
   public void setDescriptionMatch(boolean enable)
	{
		if (enable)
         flags |= DESCRIPTION_MATCH;
		else
         flags &= ~DESCRIPTION_MATCH;
	}

   /**
    * @return true if match
    */
   public boolean isRegexpMatch()
   {
      return (flags & REGEXP_MATCH) != 0;
   }

   /**
    * @param enable true to enable
    */
   public void setRegexpMatch(boolean enable)
   {
      if (enable)
         flags |= REGEXP_MATCH;
      else
         flags &= ~REGEXP_MATCH;
   }

   /**
    * @return true if column is multivalued
    */
   public boolean isMultivalued()
   {
      return (flags & MULTIVALUED) != 0;
   }
   
   /**
    * @param enable true to set column as multivalued
    */
   public void setMultivalued(boolean enable)
   {
      if (enable)
         flags |= MULTIVALUED;
      else
         flags &= ~MULTIVALUED;
   }

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "DciSummaryTableColumn [name=" + name + ", dciName=" + dciName + ", flags=" + flags + ", separator=" + separator + "]";
   }
}
