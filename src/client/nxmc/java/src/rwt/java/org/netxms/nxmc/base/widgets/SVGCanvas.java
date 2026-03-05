/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.remote.RemoteObject;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.resources.ResourceManager;

/**
 * Canvas that displays SVG image using native browser SVG rendering via RAP custom widget.
 */
public class SVGCanvas extends Composite
{
   private RemoteObject remoteObject;
   private Color defaultColor = null;

   /**
    * @param parent
    * @param style
    */
   public SVGCanvas(Composite parent, int style)
   {
      super(parent, style);
      setData(RWT.CUSTOM_VARIANT, "SVGCanvas");
      remoteObject = RWT.getUISession().getConnection().createRemoteObject("netxms.SVGCanvas");
      remoteObject.set("parent", WidgetUtil.getId(this));
   }

   /**
    * Set SVG content directly.
    *
    * @param svgContent raw SVG markup string
    */
   public void setImage(String svgContent)
   {
      if (svgContent != null)
         remoteObject.set("svgContent", svgContent);
   }

   /**
    * Set image from resource path.
    *
    * @param path resource path to SVG image
    */
   public void setImageResource(String path)
   {
      String content = ResourceManager.getResourceAsString(path);
      if (content != null)
         setImage(content);
   }

   /**
    * @return the defaultColor
    */
   public Color getDefaultColor()
   {
      return defaultColor;
   }

   /**
    * @param defaultColor the defaultColor to set
    */
   public void setDefaultColor(Color defaultColor)
   {
      this.defaultColor = defaultColor;
      if (defaultColor != null)
      {
         String cssColor = String.format("#%02x%02x%02x", defaultColor.getRed(), defaultColor.getGreen(), defaultColor.getBlue());
         remoteObject.set("svgColor", cssColor);
      }
   }

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      if (remoteObject != null)
         remoteObject.destroy();
      super.dispose();
   }
}
