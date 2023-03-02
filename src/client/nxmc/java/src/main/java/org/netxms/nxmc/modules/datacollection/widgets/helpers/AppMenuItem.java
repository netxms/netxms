/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import java.io.ByteArrayInputStream;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;
import org.apache.commons.codec.binary.Base64;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;

/**
 * Application menu item
 */
@Root(name="item")
public class AppMenuItem
{  
   @Element(required=false)
   private String name;
   
   @Element(required=false)
   private String description;
   
   @Element(required=false)
   private String icon;
   
   @Element(required=false)
   private String command;
   
   @ElementList(required=false)
   private List<AppMenuItem> items = null;
   
   private AppMenuItem parent = null;
   
   /**
    * Create command item
    * 
    * @param name Name
    * @param description Description
    * @param command Command
    * @param parent Parent item
    */
   public AppMenuItem(String name, String description, String command, AppMenuItem parent)
   {
      this.name = name;
      this.description = description;
      this.command = command;
      this.parent = parent;
   }

   /**
    * Create sub-menu item
    * 
    * @param name Name
    * @param description Description
    * @param parent Parent item
    */
   public AppMenuItem(String name, String description, AppMenuItem parent)
   {
      this.name = name;
      this.description = description;
      this.command = null;
      this.parent = parent;
      this.items = new ArrayList<AppMenuItem>(0);
   }
   
   /**
    * Create empty menu item
    */
   public AppMenuItem()
   {
      this.name = null;
      this.description = null;
      this.command = null;
      this.parent = null;
   }

   /**
    * Create empty sub-menu or command item
    * 
    * @param subMenu true to create sub-menu item
    */
   public AppMenuItem(boolean subMenu)
   {
      this.name = null;
      this.description = null;
      this.command = null;
      this.parent = null;
      if (subMenu)
         items = new ArrayList<AppMenuItem>(0);
   }
   
   /**
    * Check if this item is a sub-menu
    * 
    * @return true if this item is a sub-menu
    */
   public boolean isSubMenu()
   {
      return items != null;
   }
   
   public String getCommand()
   {
      return command;
   }

   public boolean hasChildren()
   {
      return (items != null) && !items.isEmpty();
   }

   public AppMenuItem[] getChildren()
   {
      return (items != null) ? items.toArray(new AppMenuItem[items.size()]) : null;
   }

   /**
    * Get parent menu item
    * 
    * @return parent menu item
    */
   public AppMenuItem getParent()
   {
      return parent;
   }

   public void setIcon(byte[] bs)
   {
      try
      {
         icon = new String(Base64.encodeBase64(bs, false), "ISO-8859-1");
      }
      catch(UnsupportedEncodingException e)
      {
         icon = "";
      }
   }
   
   public void updateParents(AppMenuItem parent) 
   {
      this.parent = parent;
      if (items != null)
      {
         for(AppMenuItem child : items)
            child.updateParents(this);
      }
   }
   
   public boolean delete()
   {
      if (parent != null)
      {
         parent.removeSubItem(this);
         clearChildren();
         return true;
      }
      return false;
   }
   
   private void clearChildren()
   {
      if (items == null)
         return;
      
      for(AppMenuItem obj : items)
      {
         obj.clearChildren();
      }
      items = null;
   }

   private void removeSubItem(AppMenuItem item)
   {
      if (items != null)
         items.remove(item);
   }
   
   public void addSubItem(AppMenuItem item)
   {
      if (items != null)
         items.add(item);
   }
   
   @SuppressWarnings("deprecation")
   public Image getIcon()
   {
      if (icon == null)
         return null;
      
      try
      {
         byte[] tmp = Base64.decodeBase64(icon.getBytes("ISO-8859-1"));
         ByteArrayInputStream input = new ByteArrayInputStream(tmp);
         ImageDescriptor d = ImageDescriptor.createFromImageData(new ImageData(input));
         return d.createImage();
      }
      catch(Exception e)
      {
         return null;
      }
   }

   public String getName()
   {
      return name;
   }
   
   public String getDescription()
   {
      return description;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @param command the command to set
    */
   public void setCommand(String command)
   {
      this.command = command;
   }
}
