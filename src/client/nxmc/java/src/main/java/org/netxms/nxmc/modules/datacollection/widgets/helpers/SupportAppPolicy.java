/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.io.UnsupportedEncodingException;
import org.apache.commons.codec.binary.Base64;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * User support application policy
 */
@Root(name="SupportAppPolicy", strict=false)
public class SupportAppPolicy
{
   @Element(required=false)
   public boolean closeOnDeactivate = false;

   @Element(required=false)
   public Integer mainWindowPosition = null;
   
   @Element(required=false)
   public Integer notificationTimeout = null;

   @Element(required = false)
   public boolean customColorSchema = false;

   @Element(required=false)
   public Integer backgroundColor = 0x303030;
   
   @Element(required=false)
   public Integer textColor = 0xC0C0C0;
   
   @Element(required=false)
   public Integer highlightColor = 0x2020C0;
   
   @Element(required=false)
   public Integer borderColor = 0xC0C0C0;
   
   @Element(required=false)
   public Integer menuBackgroundColor = 0x303030;

   @Element(required=false)
   public Integer menuHighligtColor = 0x505050;

   @Element(required=false)
   public Integer menuSelectionColor = 0x0069D2;

   @Element(required=false)
   public Integer menuTextColor = 0xF0F0F0;

   @Element(required=false)
   public Integer notificationBackgroundColor = 0xF0F0F0;

   @Element(required = false)
   public Integer notificationHighligtColor = 0xC0C0C0;

   @Element(required = false)
   public Integer notificationSelectionColor = 0x007FFF;

   @Element(required = false)
   public Integer notificationTextColor = 0x202020;

   @Element(required = false)
   public String icon;

   @Element(required=false)
   public String welcomeMessage;

   @Element(required=false)
   public String tooltipMessage;

   @Element(required = false)
   public String desktopWallpaper;

   @Element(required=false)
   public AppMenuItem menu = new AppMenuItem("[root]", "", null);

   /**
    * Create support application policy from XML
    * 
    * @param xml configuration
    * @return attribute support application policy
    * @throws Exception if deserialization fails
    */
   public static SupportAppPolicy createFromXml(final String xml) throws Exception
   {
      SupportAppPolicy policy = XMLTools.createFromXml(SupportAppPolicy.class, xml);
      policy.updateMenuParents();
      return policy;
   }
   
   /**
    * Create XML from configuration entry
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      return XMLTools.serialize(this);
   }
   
   /**
    * Set application icon
    * 
    * @param content application icon file content (in ICO format)
    */
   public void setIcon(byte[] content)
   {
      if (content != null)
      {
         try
         {
            icon = new String(Base64.encodeBase64(content), "ISO-8859-1");
         }
         catch(UnsupportedEncodingException e)
         {
            icon = null;
         }
      }
      else
      {
         icon = null;
      }
   }
	
   /**
    * Get application icon file content (in ICO format)
    * 
    * @return application icon file content (in ICO format)
    */
   public byte[] getIcon() 
   {
      if (icon == null)
         return null;
      
      try
      {
         return Base64.decodeBase64(icon.getBytes("ISO-8859-1"));
      }
      catch(UnsupportedEncodingException e)
      {
         return null;
      }
   }

   /**
    * Update parent references in menu items
    */
   public void updateMenuParents()
   {
      menu.updateParents(null);
   }
}
