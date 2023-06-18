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
package org.netxms.nxmc.tools;

import org.eclipse.swt.graphics.RGB;
import org.simpleframework.xml.convert.Converter;
import org.simpleframework.xml.stream.InputNode;
import org.simpleframework.xml.stream.OutputNode;

/**
 * SimpleXML converter for UUID class
 */
public class RGBConverter implements Converter<RGB>
{
   /**
    * @see org.simpleframework.xml.convert.Converter#read(org.simpleframework.xml.stream.InputNode)
    */
   @Override
   public RGB read(InputNode n) throws Exception
   {
      return ColorConverter.parseColorDefinition(n.getValue());
   }

   /**
    * @see org.simpleframework.xml.convert.Converter#write(org.simpleframework.xml.stream.OutputNode, java.lang.Object)
    */
   @Override
   public void write(OutputNode n, RGB rgb) throws Exception
   {
      n.setValue(ColorConverter.rgbToCss(rgb));
   }
}
