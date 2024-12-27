/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Reden Solutions
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

/**
 * Pair of name and description
 */
public class DciInfo
{
   public String metric;
   public String displayName;
   public String userTag;
   
   /**
    * Constructor
    * 
    * @param name DCI name
    * @param displayName DCI description
    * @param userTag user-assigned tag
    */
   public DciInfo(String name, String displayName, String userTag)
   {
      this.metric = name;
      this.displayName = displayName;
      this.userTag = userTag;
   }
}
