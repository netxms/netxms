/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logwatch.widgets.helpers;

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Text;

/**
 * Context in log parser rule
 */
@Root(name="context", strict=false)
public class LogParserPushDci
{
	@Attribute(required=false)
	private Integer group = 1;
	
	@Text(required=false)
	private String data = ""; //$NON-NLS-1$
		

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}

   /**
    * @return the group
    */
   public Integer getGroup()
   {
      return group;
   }

   /**
    * @param group the group to set
    */
   public void setGroup(Integer group)
   {
      this.group = group;
   }
}
