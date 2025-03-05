/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.config;

import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.tools.ColorConverter;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * Configuration for label
 */
@Root(name = "element", strict = false)
public class LabelConfig extends DashboardElementConfig
{
   private static final Logger logger = LoggerFactory.getLogger(LabelConfig.class);

	@Element(required=false)
   private String foreground = null;

	@Element(required=false)
   private String background = null;

	/**
	 * Create label configuration object from XML document
	 * 
	 * @param data XML or JSON document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static LabelConfig createFromXmlOrJson(final String data) throws Exception
	{
	   LabelConfig config;
      if (data.trim().startsWith("<"))
      {
         config = XMLTools.createFromXml(LabelConfig.class, data);
      }
      else
      {
         config = new Gson().fromJson(data, LabelConfig.class);
      }

      // Update from old versions
      if ((config.foreground != null) && config.foreground.startsWith("0x"))
      {
         try
         {
            config.setTitleForeground(ColorConverter.rgbToCss(ColorConverter.rgbFromInt(Integer.parseInt(config.foreground.substring(2), 16))));
         }
         catch(NumberFormatException e)
         {
            logger.warn("Cannot convert label foreground color \"" + config.foreground + "\"", e);
         }
         config.foreground = null;
      }
      if ((config.background != null) && config.background.startsWith("0x"))
      {
         try
         {
            config.setTitleBackground(ColorConverter.rgbToCss(ColorConverter.rgbFromInt(Integer.parseInt(config.background.substring(2), 16))));
         }
         catch(NumberFormatException e)
         {
            logger.warn("Cannot convert label background color \"" + config.background + "\"", e);
         }
         config.background = null;
      }

      return config;
	}
}
