/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.client.objecttools;

import java.util.HashMap;
import org.netxms.base.annotations.Internal;

/**
 * Object tools folder
 */
public class ObjectToolFolder
{
   @Internal
   private ObjectToolFolder parent;
   
   private String name;
   private String displayName;
   private HashMap<String, ObjectToolFolder> subfolders;
   private HashMap<Long, ObjectTool> tools;
   
   /**
    * Create new object tool folder with given name
    * 
    * @param name folder's name
    */
   public ObjectToolFolder(String name)
   {
      this.name = name;
      displayName = name.replace("&", "");
      this.parent = null;
      this.subfolders = new HashMap<String, ObjectToolFolder>();
      this.tools = new HashMap<Long, ObjectTool>();
   }
   
   /**
    * Add tool to folder
    * 
    * @param t tool to add
    */
   public void addTool(ObjectTool t)
   {
      tools.put(t.getId(), t);
   }
   
   /**
    * Add sub-folder
    * 
    * @param f sub-folder
    */
   public void addFolder(ObjectToolFolder f)
   {
      f.setParent(this);
      subfolders.put(f.getDisplayName(), f);
   }
   
   /**
    * Get folder name
    * 
    * @return folder name
    */
   public String getName()
   {
      return name;
   }
   
   /**
    * Get folder display name
    * 
    * @return folder display name
    */
   public String getDisplayName()
   {
      return displayName;
   }
   
   /**
    * Set folder's parent
    * 
    * @param f new parent
    */
   public void setParent(ObjectToolFolder f)
   {
      parent = f;
   }
}
