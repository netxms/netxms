/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import java.io.StringWriter;
import java.io.Writer;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Text box element config
 */
@Root(name="textBoxConfig")
public class TextBoxConfig
{
   @Element(required=false)
   private String text;
   
   @Element(required=false)
   private int backgroundColor;
   
   @Element(required=false)
   private int textColor;
   
   @Element(required=false)
   private int borderColor;
   
   @Element(required=false)
   private int fontSize;

   @Element(required=false)
   private long drillDownObjectId;
   
   @Element(required=false)
   private boolean borderRequired;
   
   /**
    * Create text box element from xml document
    * 
    * @param xml
    * @return
    * @throws Exception
    */
   public static TextBoxConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(TextBoxConfig.class, xml);
   }
   
   /**
    * Create XML from configuration
    * 
    * @return XML document
    * @throws Exception if the schema for the object is not valid
    */
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }
   
   public String getText()
   {
      return text;
   }
   
   public void setText(String text)
   {
      this.text = text;
   }
   
   /**
    * @return the backgroundColor
    */
   public int getBackgroundColor()
   {
      return backgroundColor;
   }

   /**
    * @param backgroundColor the backgroundColor to set
    */
   public void setBackgroundColor(int backgroundColor)
   {
      this.backgroundColor = backgroundColor;
   }

   /**
    * @return the textColor
    */
   public int getTextColor()
   {
      return textColor;
   }

   /**
    * @param textColor the textColor to set
    */
   public void setTextColor(int textColor)
   {
      this.textColor = textColor;
   }

   /**
    * @return the borderColor
    */
   public int getBorderColor()
   {
      return borderColor;
   }

   /**
    * @param borderColor the borderColor to set
    */
   public void setBorderColor(int borderColor)
   {
      this.borderColor = borderColor;
   }

   /**
    * @return the borderRequired
    */
   public boolean isBorderRequired()
   {
      return borderRequired;
   }

   /**
    * @param borderRequired the borderRequired to set
    */
   public void setBorderRequired(boolean borderRequired)
   {
      this.borderRequired = borderRequired;
   }
   
   /**
    * @return font size
    */
   public int getFontSize()
   {
      return fontSize;
   }
   
   /**
    * @param fontSize to set
    */
   public void setFontSize(int fontSize)
   {
      this.fontSize = fontSize;
   }
   
   /**
    * @return drill down object id
    */
   public long getDrillDownObjectId()
   {
      return drillDownObjectId;
   }
   
   /**
    * @param drillDownObjectId to set
    */
   public void setDrillDownObjectId(long drillDownObjectId)
   {
      this.drillDownObjectId = drillDownObjectId;
   }
}
